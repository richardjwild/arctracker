import { useStore } from "../store/useStore.ts";
import { engine, eventsEqual, PatternEvent } from "../engine/engine.ts";
import { Cursor, CursorPosition } from "./cursor.ts";

export type EditorState = {
  editing: boolean;
  cursorPosition: CursorPosition;
  effectsDisplayed: number[];
};

export enum EditType {
  EventEdit,
}

export type EditCommand = {
  type: EditType.EventEdit;
  patternNo: number;
  patternIndex: number;
  track: number;
  before: PatternEvent;
  after: PatternEvent;
};

const undoStack: EditCommand[] = [];
const redoStack: EditCommand[] = [];

export const editor = {
  togglePatternEdit: () => {
    const { editorState, setEditorState, setPatternSelection } =
      useStore.getState();
    setEditorState({
      ...editorState,
      editing: !editorState.editing,
    });
    setPatternSelection(null);
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
    if (effectsDisplayed[track] === 1) return;
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
      let revised = false;
      if (
        command.type === EditType.EventEdit &&
        !eventsEqual(command.before, command.after)
      ) {
        await engine.setEvent(
          command.patternNo,
          command.patternIndex,
          command.track,
          command.after,
        );
        revised = true;
      }
      if (revised) {
        useStore.getState().moduleRevised();
        undoStack.push(command);
        redoStack.length = 0;
      }
    } catch (err) {
      throw err;
    }
  },

  undo: async () => {
    const command = undoStack.pop();
    if (!command) return;
    try {
      if (command.type === EditType.EventEdit) {
        await engine.setEvent(
          command.patternNo,
          command.patternIndex,
          command.track,
          command.before,
        );
      }
      useStore.getState().moduleRevised();
      redoStack.push(command);
    } catch (err) {
      throw err;
    }
  },

  redo: async () => {
    const command = redoStack.pop();
    if (!command) return;
    try {
      if (command.type === EditType.EventEdit) {
        await engine.setEvent(
          command.patternNo,
          command.patternIndex,
          command.track,
          command.after,
        );
      }
      useStore.getState().moduleRevised();
      undoStack.push(command);
    } catch (err) {
      throw err;
    }
  },
};
