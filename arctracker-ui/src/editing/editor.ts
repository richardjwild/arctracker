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
  CompoundEventEdit,
}

export type EventEditCommand = {
  type: EditType.EventEdit;
  patternNo: number;
  patternIndex: number;
  track: number;
  before: PatternEvent;
  after: PatternEvent;
};

export type CompoundEventEditCommand = {
  type: EditType.CompoundEventEdit;
  eventEdits: EventEditCommand[];
};

export type EditCommand = EventEditCommand | CompoundEventEditCommand;

const undoStack: EditCommand[] = [];
const redoStack: EditCommand[] = [];

async function applyEventEdit(command: EventEditCommand): Promise<boolean> {
  if (!eventsEqual(command.before, command.after)) {
    await engine.setEvent(
      command.patternNo,
      command.patternIndex,
      command.track,
      command.after,
    );
    return true;
  }
  return false;
}

async function applyCompoundEventEdit(
  command: CompoundEventEditCommand,
): Promise<boolean> {
  let revised = false;
  for (const edit of command.eventEdits) {
    if (!eventsEqual(edit.before, edit.after)) {
      await engine.setEvent(
        edit.patternNo,
        edit.patternIndex,
        edit.track,
        edit.after,
      );
      revised = true;
    }
  }
  return revised;
}

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
    const { editorState, setEditorState } =
        useStore.getState();
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
      let revised = false;
      if (command.type === EditType.EventEdit) {
        revised = await applyEventEdit(command);
      }
      if (command.type === EditType.CompoundEventEdit) {
        revised = await applyCompoundEventEdit(command);
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
        await applyEventEdit({
          ...command,
          before: command.after,
          after: command.before,
        });
      }
      if (command.type === EditType.CompoundEventEdit) {
        await applyCompoundEventEdit({
          ...command,
          eventEdits: command.eventEdits.map((edit) => ({
            ...edit,
            before: edit.after,
            after: edit.before,
          })),
        });
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
        await applyEventEdit(command);
      }
      if (command.type === EditType.CompoundEventEdit) {
        await applyCompoundEventEdit(command);
      }
      useStore.getState().moduleRevised();
      undoStack.push(command);
    } catch (err) {
      throw err;
    }
  },
};
