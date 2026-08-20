import { useStore } from "../store/useStore.ts";
import { Cursor, CursorPosition } from "./cursor.ts";
import { alerting } from "../alerting/alert.ts";
import { transport } from "../transport/transport.ts";
import { engine } from "../engine/engine.ts";

export type EditMode =
  | "none"
  | "patternEvents"
  | "instrument"
  | "patternLength"
  | "moduleMetaData"
  | "trackCount"
  | "tempo"
  | "appConfig";

export type EditorState = {
  editMode: EditMode;
  inputtingText: boolean;
  cursorPosition: CursorPosition;
  sequencePosition: number;
};

export type EditCommand = {
  apply: (redoing: boolean) => Promise<boolean>;
  undo: () => Promise<void>;
};

type HistoryEntry = {
  command: EditCommand;
  beforeRevision: number;
  afterRevision: number;
};

const undoStack: HistoryEntry[] = [];
const redoStack: HistoryEntry[] = [];
let nextRevisionId = 1;
let currentRevisionId = 0;
let savedRevisionId = 0;

export const editor = {
  setEditMode: (editMode: EditMode) => {
    const editorState = useStore.getState().editorState;
    useStore.getState().setEditorState({
      ...editorState,
      editMode,
    });
  },

  hasUnsavedChanges: (): boolean => currentRevisionId !== savedRevisionId,

  allChangesSaved: () => {
    savedRevisionId = currentRevisionId;
  },

  inputtingText: () => {
    return useStore.getState().editorState.inputtingText;
  },

  startTextInput: () => {
    const { editorState, setEditorState } = useStore.getState();
    setEditorState({
      ...editorState,
      inputtingText: true,
    });
  },

  stopTextInput: () => {
    const { editorState, setEditorState } = useStore.getState();
    setEditorState({
      ...editorState,
      inputtingText: false,
    });
  },

  togglePatternEdit: () => {
    if (transport.playing()) return;
    const { editorState, setEditorState, setPatternSelection } =
      useStore.getState();
    setEditorState({
      ...editorState,
      editMode: editorState.editMode === "patternEvents" ? "none" : "patternEvents",
    });
    setPatternSelection(null);
  },

  cancelPatternEdit: () => {
    const { editorState, setEditorState } = useStore.getState();
    setEditorState({
      ...editorState,
      editMode: "none",
    });
  },

  increaseEffectsDisplayed: () => {
    const { editorState, effectsDisplayed } = useStore.getState();
    const track = editorState.cursorPosition.track;
    const newEffectsDisplayed = [...effectsDisplayed];
    if (newEffectsDisplayed[track] === 4) return;
    newEffectsDisplayed[track] += 1;
    engine.setEffectsDisplayed(track, newEffectsDisplayed[track]);
  },

  decreaseEffectsDisplayed: () => {
    const { editorState, effectsDisplayed, setEditorState } = useStore.getState();
    const track = editorState.cursorPosition.track;
    let cursor = new Cursor();
    let newEffectsDisplayed = [...effectsDisplayed];
    if (newEffectsDisplayed[track] === 0) return;
    newEffectsDisplayed[track] -= 1;
    cursor.effectsDisplayedDecreased();
    setEditorState({
      ...editorState,
      cursorPosition: cursor.currentPosition(),
    });
    engine.setEffectsDisplayed(track, newEffectsDisplayed[track]);
  },

  applyEdit: async (command: EditCommand) => {
    try {
      const revised = await command.apply(false);
      if (revised) {
        const entry: HistoryEntry = {
          command,
          beforeRevision: currentRevisionId,
          afterRevision: nextRevisionId,
        };
        nextRevisionId += 1;
        currentRevisionId = entry.afterRevision;
        undoStack.push(entry);
        redoStack.length = 0;
      }
    } catch (err) {
      void alerting.showError(err as string);
      throw err;
    }
  },

  undoEdit: async () => {
    const entry = undoStack.pop();
    if (!entry) return;
    try {
      await entry.command.undo();
      currentRevisionId = entry.beforeRevision;
      redoStack.push(entry);
    } catch (err) {
      undoStack.push(entry);
      void alerting.showError(err as string);
      throw err;
    }
  },

  redoEdit: async () => {
    const entry = redoStack.pop();
    if (!entry) return;
    try {
      await entry.command.apply(true);
      currentRevisionId = entry.afterRevision;
      undoStack.push(entry);
    } catch (err) {
      redoStack.push(entry);
      void alerting.showError(err as string);
      throw err;
    }
  },

  newModuleLoaded: () => {
    undoStack.length = 0;
    redoStack.length = 0;
    nextRevisionId = 1;
    currentRevisionId = 0;
    savedRevisionId = 0;
  },
};
