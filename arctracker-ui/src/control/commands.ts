import { CursorField } from "../editing/cursor.ts";

type SampleCursorField = Extract<
  CursorField,
  { field: "sampleHigh" | "sampleLow" }
>;

type EffectCodeCursorField = Extract<CursorField, { field: "effectCode" }>;

type EffectDataCursorField = Extract<
  CursorField,
  { field: "effectData1" | "effectData2" }
>;

export enum CommandType {
  LOAD_FILE,
  EXPORT_AUDIO,
  CREATE_MODULE,
  TOGGLE_PLAY,
  TOGGLE_LOOP,
  TOGGLE_EDIT,
  CURSOR_FIELD_LEFT,
  CURSOR_FIELD_RIGHT,
  PATTERN_GRID_LEFT,
  PATTERN_GRID_RIGHT,
  PATTERN_GRID_UP,
  PATTERN_GRID_DOWN,
  INCREASE_EFFECTS_DISPLAYED,
  DECREASE_EFFECTS_DISPLAYED,
  SEQUENCE_SEEK_FORWARDS,
  SEQUENCE_SEEK_BACKWARDS,
  PATTERN_GRID_STRIDE_DOWN,
  PATTERN_GRID_STRIDE_UP,
  PATTERN_GRID_JUMP_TO_TOP,
  PATTERN_GRID_JUMP_TO_BOTTOM,
  EDIT_NOTE_FIELD,
  EDIT_SAMPLE_FIELD,
  EDIT_EFFECT_CODE,
  EDIT_EFFECT_DATA,
  CLEAR_PATTERN_EVENT_FIELD,
  CLEAR_PATTERN_EVENT,
  UNDO_EDIT,
  REDO_EDIT,
}

export type Command =
  | { type: CommandType.LOAD_FILE }
  | { type: CommandType.EXPORT_AUDIO }
  | { type: CommandType.CREATE_MODULE; numChannels: number }
  | { type: CommandType.TOGGLE_PLAY }
  | { type: CommandType.TOGGLE_LOOP }
  | { type: CommandType.TOGGLE_EDIT }
  | { type: CommandType.SEQUENCE_SEEK_FORWARDS }
  | { type: CommandType.SEQUENCE_SEEK_BACKWARDS }
  | { type: CommandType.PATTERN_GRID_LEFT }
  | { type: CommandType.PATTERN_GRID_RIGHT }
  | { type: CommandType.PATTERN_GRID_DOWN; wrap: boolean }
  | { type: CommandType.PATTERN_GRID_UP }
  | { type: CommandType.PATTERN_GRID_STRIDE_DOWN }
  | { type: CommandType.PATTERN_GRID_STRIDE_UP }
  | { type: CommandType.PATTERN_GRID_JUMP_TO_TOP }
  | { type: CommandType.PATTERN_GRID_JUMP_TO_BOTTOM }
  | { type: CommandType.CURSOR_FIELD_LEFT }
  | { type: CommandType.CURSOR_FIELD_RIGHT }
  | { type: CommandType.INCREASE_EFFECTS_DISPLAYED }
  | { type: CommandType.DECREASE_EFFECTS_DISPLAYED }
  | { type: CommandType.EDIT_NOTE_FIELD; note: number }
  | {
      type: CommandType.EDIT_SAMPLE_FIELD;
      field: SampleCursorField;
      value: string;
    }
  | {
      type: CommandType.EDIT_EFFECT_CODE;
      field: EffectCodeCursorField;
      value: string;
    }
  | {
      type: CommandType.EDIT_EFFECT_DATA;
      field: EffectDataCursorField;
      value: string;
    }
  | { type: CommandType.CLEAR_PATTERN_EVENT_FIELD }
  | { type: CommandType.CLEAR_PATTERN_EVENT }
  | { type: CommandType.UNDO_EDIT }
  | { type: CommandType.REDO_EDIT };

const queue: Command[] = [];

export const commandQueue = {
  push: (command: Command) => {
    queue.push(command);
  },

  consume: (): Command[] => {
    return queue.splice(0);
  },
};

export const commands = {
  loadFile: () => commandQueue.push({ type: CommandType.LOAD_FILE }),
  exportAudio: () => commandQueue.push({ type: CommandType.EXPORT_AUDIO }),
  createModule: (numChannels: number) =>
    commandQueue.push({ type: CommandType.CREATE_MODULE, numChannels }),
  togglePlay: () => commandQueue.push({ type: CommandType.TOGGLE_PLAY }),
  toggleLoop: () => commandQueue.push({ type: CommandType.TOGGLE_LOOP }),
  toggleEdit: () => commandQueue.push({ type: CommandType.TOGGLE_EDIT }),
  sequenceSeekForwards: () =>
    commandQueue.push({ type: CommandType.SEQUENCE_SEEK_FORWARDS }),
  sequenceSeekBackwards: () =>
    commandQueue.push({ type: CommandType.SEQUENCE_SEEK_BACKWARDS }),
  patternGridLeft: () =>
    commandQueue.push({ type: CommandType.PATTERN_GRID_LEFT }),
  patternGridRight: () =>
    commandQueue.push({ type: CommandType.PATTERN_GRID_RIGHT }),
  patternGridDown: (wrap: boolean = false) =>
    commandQueue.push({ type: CommandType.PATTERN_GRID_DOWN, wrap }),
  patternGridUp: () =>
    commandQueue.push({ type: CommandType.PATTERN_GRID_UP }),
  patternGridStrideDown: () =>
    commandQueue.push({ type: CommandType.PATTERN_GRID_STRIDE_DOWN }),
  patternGridStrideUp: () =>
    commandQueue.push({ type: CommandType.PATTERN_GRID_STRIDE_UP }),
  patternGridJumpToTop: () =>
    commandQueue.push({ type: CommandType.PATTERN_GRID_JUMP_TO_TOP }),
  patternGridJumpToBottom: () =>
    commandQueue.push({ type: CommandType.PATTERN_GRID_JUMP_TO_BOTTOM }),
  cursorFieldLeft: () =>
    commandQueue.push({ type: CommandType.CURSOR_FIELD_LEFT }),
  cursorFieldRight: () =>
    commandQueue.push({ type: CommandType.CURSOR_FIELD_RIGHT }),
  increaseEffectsDisplayed: () =>
    commandQueue.push({ type: CommandType.INCREASE_EFFECTS_DISPLAYED }),
  decreaseEffectsDisplayed: () =>
    commandQueue.push({ type: CommandType.DECREASE_EFFECTS_DISPLAYED }),
  editNoteField: (note: number) =>
    commandQueue.push({ type: CommandType.EDIT_NOTE_FIELD, note }),
  editSampleField: (field: SampleCursorField, value: string) =>
    commandQueue.push({ type: CommandType.EDIT_SAMPLE_FIELD, field, value }),
  editEffectCode: (field: EffectCodeCursorField, value: string) =>
    commandQueue.push({ type: CommandType.EDIT_EFFECT_CODE, field, value }),
  editEffectData: (field: EffectDataCursorField, value: string) =>
    commandQueue.push({ type: CommandType.EDIT_EFFECT_DATA, field, value }),
  clearPatternEventField: () =>
    commandQueue.push({ type: CommandType.CLEAR_PATTERN_EVENT_FIELD }),
  clearPatternEvent: () =>
    commandQueue.push({ type: CommandType.CLEAR_PATTERN_EVENT }),
  undoEdit: () => commandQueue.push({ type: CommandType.UNDO_EDIT }),
  redoEdit: () => commandQueue.push({ type: CommandType.REDO_EDIT }),
};
