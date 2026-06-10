import { useStore } from "../store/useStore.ts";
import { Cursor, CursorPosition } from "./cursor.ts";

export type EditorState = {
  editing: boolean;
  cursorPosition: CursorPosition;
  sequencePosition: number;
  effectsDisplayed: number[];
};

export type EditCommand = {
  apply: (redoing: boolean) => Promise<boolean>;
  undo: () => Promise<void>;
};

const undoStack: EditCommand[] = [];
const redoStack: EditCommand[] = [];

export const editor = {
  editing: () => {
    const { editorState } = useStore.getState();
    return editorState.editing;
  },

  togglePatternEdit: () => {
    if (useStore.getState().transportState.playing) return;
    const { editorState, setEditorState, setPatternSelection } =
      useStore.getState();
    setEditorState({
      ...editorState,
      editing: !editorState.editing,
    });
    setPatternSelection(null);
  },

  cancelPatternEdit: () => {
    const { editorState, setEditorState } = useStore.getState();
    setEditorState({
      ...editorState,
      editing: false,
    });
  },

  increaseEffectsDisplayed: () => {
    const { editorState, setEditorState } = useStore.getState();
    const track = editorState.cursorPosition.track;
    const effectsDisplayed = [...editorState.effectsDisplayed];
    if (effectsDisplayed[track] === 4) return;
    effectsDisplayed[track] += 1;
    setEditorState({
      ...editorState,
      effectsDisplayed,
    });
  },

  decreaseEffectsDisplayed: () => {
    const { editorState, setEditorState } = useStore.getState();
    const track = editorState.cursorPosition.track;
    let cursor = new Cursor();
    let effectsDisplayed = [...editorState.effectsDisplayed];
    if (effectsDisplayed[track] === 0) return;
    effectsDisplayed[track] -= 1;
    cursor.effectsDisplayedDecreased();
    setEditorState({
      ...editorState,
      cursorPosition: cursor.currentPosition(),
      effectsDisplayed,
    });
  },

  applyEdit: async (command: EditCommand) => {
    try {
      const revised = await command.apply(false);
      if (revised) {
        undoStack.push(command);
        redoStack.length = 0;
      }
    } catch (err) {
      throw err;
    }
  },

  undoEdit: async () => {
    const command = undoStack.pop();
    if (!command) return;
    try {
      await command.undo();
      redoStack.push(command);
    } catch (err) {
      throw err;
    }
  },

  redoEdit: async () => {
    const command = redoStack.pop();
    if (!command) return;
    try {
      await command.apply(true);
      undoStack.push(command);
    } catch (err) {
      throw err;
    }
  },

  clearUndoBuffer: () => {
    undoStack.length = 0;
    redoStack.length = 0;
  },
};
