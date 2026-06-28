import { engine } from "../engine/engine.ts";
import { useStore } from "../store/useStore.ts";
import {
  filePicker,
  ARCTRACKER_MODFILE_EXTENSION,
  DESKTOP_TRACKER_MODFILE_EXTENSION,
  TRACKER_MODFILE_EXTENSION,
} from "../filesystem/filePicker.ts";
import { alerting } from "../alerting/alert.ts";
import { editor } from "../editing/editor.ts";

export type { Module, Sample } from "../engine/engine.ts";

export const module = {
  getCurrent: async () => {
    engine
      .getCurrentModule()
      .then((module) => useStore.getState().setModule(module));
  },

  load: async (): Promise<boolean> => {
    const { setLoadingModule, setModule } = useStore.getState();
    setLoadingModule(true);
    try {
      const selected = await filePicker.chooseFileToOpen([
        ARCTRACKER_MODFILE_EXTENSION,
        TRACKER_MODFILE_EXTENSION,
        DESKTOP_TRACKER_MODFILE_EXTENSION,
      ]);
      if (selected) {
        let module = await engine.loadModule(selected);
        if (module) {
          module.fileName = selected;
          setModule(module);
        }
      }
      return true;
    } catch (err) {
      await alerting.showError(err as string);
      return false;
    } finally {
      setLoadingModule(false);
    }
  },

  create: async (numTracks: number): Promise<boolean> => {
    try {
      const newModule = await engine.createModule(numTracks);
      if (newModule) useStore.getState().setModule(newModule);
      return true;
    } catch (err) {
      await alerting.showError(err as string);
      return false;
    }
  },

  editTrackCount: () => editor.setEditMode("trackCount"),

  setTrackCount: async (trackCount: number) => {
    const module = useStore.getState().module;
    const currentTracks = module.numTracks;
    const currentEffectsDisplayed = useStore.getState().editorState.effectsDisplayed;
    let newEffectsDisplayed = [...currentEffectsDisplayed];
    if (trackCount < currentTracks) {
      newEffectsDisplayed.splice(newEffectsDisplayed.length - 1, (currentTracks - trackCount));
    } else {
      for (let i = 0; i < (trackCount - currentTracks); i++)
        newEffectsDisplayed.push(1);
    }
    if (currentTracks !== trackCount) {
      void editor.applyEdit({
        apply: async () => {
          await engine.setNumTracks(trackCount);
          useStore.getState().updateTracks(trackCount);
          const editorState = useStore.getState().editorState;
          useStore.getState().setEditorState({
            ...editorState,
            effectsDisplayed: newEffectsDisplayed,
          });
          return true;
        },
        undo: async () => {
          await engine.setNumTracks(currentTracks);
          useStore.getState().updateTracks(currentTracks);
          const editorState = useStore.getState().editorState;
          useStore.getState().setEditorState({
            ...editorState,
            effectsDisplayed: currentEffectsDisplayed,
          });
        },
      });
    }
    editor.setEditMode("none");
  },
};
