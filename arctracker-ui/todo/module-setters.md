Yes — there is a cleaner way to express this, and the underlying issue is not that the granular setters are wrong. It is that `setModule` is doing two conceptually different jobs:
  
  1. replacing the module data;
  2. resetting the editor session because a different document has been opened or created.
  
  That makes the name `setModule` deceptively innocent. It sounds like a normal state setter, but it is actually a lifecycle operation.
  
  I would separate those concepts explicitly.

For example:
  
  ```ts
  interface AppStore {
module: Module;

replaceModule: (module: Module) => void;
updateModule: (update: (module: Module) => Module) => void;
}
  ```

Where `replaceModule` means:
  
  > A different document has become active. Install it and reset all document-specific UI state.

And `updateModule` means:
  
  > This is still the same document. Apply an edit without resetting anything.

Something like:
  
  ```ts
replaceModule: (module) =>
  set({
  module,
  sequence: module.sequence,
  patternSelection: null,
  selectedInstrument: null,
  currentPattern: initialCurrentPattern(module),
  editorState: initialEditorState(),
  draftInstrument: initialInstrument(),
  draftModuleTitle: null,
  moduleId: nextModuleId(),
  patternRevision: 0,
}),
  ```

Then ordinary edits can use:
  
  ```ts
updateModule: (update) =>
  set((state) => ({
  module: update(state.module),
})),
  ```

For example:
  
  ```ts
  useAppStore.getState().updateModule((module) => ({
  ...module,
masterGain: gain,
}));
  ```
  
  That gives you one general mutation route instead of a setter for every field.

However, I would not necessarily remove every named action. Named actions can be useful when they express domain meaning:
  
  ```ts
  setMasterGain(gain)
  setModuleTitle(name, author)
  resizePatterns(count, lengths)
  setTrackCount(count)
  ```
  
  Those are better than exposing a generic `setModuleField`, because they tell you what operation is happening and provide one place for validation and associated side effects.
  
  So I would distinguish between two kinds of store action.

Document lifecycle actions:
  
  ```ts
  createModule(...)
  loadModule(...)
  replaceModule(...)
  ```
  
  These may reset editor state, selection, drafts, revisions and so forth.

Document edit actions:
  
  ```ts
  setMasterGain(...)
  renameModule(...)
  resizePatterns(...)
  setSequence(...)
  ```
  
  These preserve the current editing session.
  
  That separation is more important than reducing the raw number of functions.

A possible interface might look like:
  
  ```ts
  interface AppStore {
module: Module;

loadModule: (module: Module) => void;
createModule: (module: Module) => void;

setMasterGain: (gain: number) => void;
renameModule: (name: string, author: string) => void;
resizeTracks: (numTracks: number) => void;
resizePatterns: (
  numPatterns: number,
  patternLengths: number[],
  ) => void;
}
  ```

Internally, both `loadModule` and `createModule` could call a shared helper:
  
  ```ts
  function stateForNewlyOpenedModule(
module: Module,
previous: AppStore,
): Partial<AppStore> {
  return {
  module,
  sequence: module.sequence,
  patternSelection: null,
  selectedInstrument: null,
  currentPattern: initialCurrentPattern(module),
  editorState: initialEditorState(),
  draftInstrument: initialInstrument(),
  draftModuleTitle: null,
  moduleId: previous.moduleId + 1,
  patternRevision: 0,
};
}
  ```

Then:
  
  ```ts
loadModule: (module) =>
  set((state) => stateForNewlyOpenedModule(module, state)),
  ```
  
  This makes the reset behaviour visible and testable without attaching surprising semantics to `setModule`.

There is another option: separate the store into conceptual slices.
  
  ```ts
  type ModuleSlice = {
module: Module;
replaceModule: (module: Module) => void;
setMasterGain: (gain: number) => void;
renameModule: (name: string, author: string) => void;
};
  
  type EditorSlice = {
editorState: EditorState;
patternSelection: PatternSelection | null;
resetEditorForModule: (module: Module) => void;
};
  ```

Then a higher-level controller operation coordinates them:
  
  ```ts
function installModule(module: Module) {
  const store = useAppStore.getState();

  store.replaceModule(module);
  store.resetEditorForModule(module);
}
  ```
  
  That is architecturally neat, but it introduces the possibility of observing an intermediate state between those two updates unless they are performed in one Zustand `set`. For Arctracker, I think keeping the replacement and reset atomic inside one store action is safer.

So my preferred shape would be:
  
  ```ts
  replaceCurrentModule(module)
  ```
  
  for loading and creating, with all the reset semantics, plus domain-specific edit actions for changing the current module.

I would specifically rename:
  
  ```ts
  setModule
  ```

to something like:
  
  ```ts
  replaceModule
  installModule
  openModule
  initialiseModuleState
  ```
  
  Of those, I think `replaceModule` is the clearest mechanically, while `openModule` is clearest from the application’s point of view.
  
  On `effectsDisplayed`, your reasoning makes sense. It began as editor-view state, but once you decide it should survive save and reload, it becomes part of the document’s persisted presentation state.

There is a useful distinction here:
  
  ```text
Musical document state:
  patterns, instruments, sequence, samples

Persisted document presentation:
  visible effects columns, perhaps track widths or names

Transient editor state:
  cursor position, selection, text-entry mode
  ```
  
  All three may affect the UI, but only the first two belong in the file.

So `effectsDisplayed` can live in `module` without suggesting that it is musical data. It is simply persisted per-document metadata. I might even group that kind of thing explicitly:
  
  ```ts
  type Module = {
  // Musical content
patterns: Pattern[];
sequence: number[];
instruments: Instrument[];

  // Saved editor presentation
editorLayout: {
  effectsDisplayed: number[];
};
};
  ```
  
  That keeps the distinction visible and leaves room for future persisted settings without gradually sprinkling UI concerns throughout the musical model.

In summary: your individual module-edit functions are defensible. The awkward part is primarily the overloaded meaning of `setModule`. Give the document-replacement operation a lifecycle-oriented name, keep it atomic, and retain named edit operations where they express genuine domain behaviour.
