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
  SEQUENCE_SEEK,
  SEQUENCE_SEEK_FORWARDS,
  SEQUENCE_SEEK_BACKWARDS,
  SEQUENCE_SEEK_TO_START,
  SEQUENCE_SEEK_TO_END,
  NEXT_INSTRUMENT,
  PREVIOUS_INSTRUMENT,
  FIRST_INSTRUMENT,
  LAST_INSTRUMENT,
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
  COPY_PATTERN_EVENTS,
  CUT_PATTERN_EVENTS,
  PASTE_PATTERN_EVENTS,
  COPY_TRACK,
  CUT_TRACK,
  PASTE_TRACK,
  COPY_PATTERN,
  CUT_PATTERN,
  PASTE_PATTERN,
  UNDO_EDIT,
  REDO_EDIT,
  INCREMENT_PATTERN_AT_CURRENT_POSITION,
  DECREMENT_PATTERN_AT_CURRENT_POSITION,
  INSERT_SEQUENCE_POSITION_BEFORE,
  INSERT_SEQUENCE_POSITION_AFTER,
  DELETE_SEQUENCE_POSITION,
  OPEN_INSTRUMENT_EDITOR,
  SAVE_AND_CLOSE_INSTRUMENT_EDITOR,
  RESTORE_AND_CLOSE_INSTRUMENT_EDITOR,
  LOAD_SAMPLE,
  DELETE_SAMPLE,
}

export type Command =
  | { type: CommandType.LOAD_FILE }
  | { type: CommandType.EXPORT_AUDIO }
  | { type: CommandType.CREATE_MODULE; numChannels: number }
  | { type: CommandType.TOGGLE_PLAY }
  | { type: CommandType.TOGGLE_LOOP }
  | { type: CommandType.TOGGLE_EDIT }
  | { type: CommandType.SEQUENCE_SEEK; position: number }
  | { type: CommandType.SEQUENCE_SEEK_FORWARDS }
  | { type: CommandType.SEQUENCE_SEEK_BACKWARDS }
  | { type: CommandType.SEQUENCE_SEEK_TO_START }
  | { type: CommandType.SEQUENCE_SEEK_TO_END }
  | { type: CommandType.NEXT_INSTRUMENT }
  | { type: CommandType.PREVIOUS_INSTRUMENT }
  | { type: CommandType.FIRST_INSTRUMENT }
  | { type: CommandType.LAST_INSTRUMENT }
  | { type: CommandType.PATTERN_GRID_LEFT; extendSelection: boolean }
  | { type: CommandType.PATTERN_GRID_RIGHT; extendSelection: boolean }
  | {
      type: CommandType.PATTERN_GRID_DOWN;
      extendSelection: boolean;
      wrap: boolean;
    }
  | { type: CommandType.PATTERN_GRID_UP; extendSelection: boolean }
  | { type: CommandType.PATTERN_GRID_STRIDE_DOWN; extendSelection: boolean }
  | { type: CommandType.PATTERN_GRID_STRIDE_UP; extendSelection: boolean }
  | { type: CommandType.PATTERN_GRID_JUMP_TO_TOP; extendSelection: boolean }
  | { type: CommandType.PATTERN_GRID_JUMP_TO_BOTTOM; extendSelection: boolean }
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
  | { type: CommandType.COPY_PATTERN_EVENTS }
  | { type: CommandType.CUT_PATTERN_EVENTS }
  | { type: CommandType.PASTE_PATTERN_EVENTS }
  | { type: CommandType.COPY_TRACK }
  | { type: CommandType.CUT_TRACK }
  | { type: CommandType.PASTE_TRACK }
  | { type: CommandType.COPY_PATTERN }
  | { type: CommandType.CUT_PATTERN }
  | { type: CommandType.PASTE_PATTERN }
  | { type: CommandType.UNDO_EDIT }
  | { type: CommandType.REDO_EDIT }
  | { type: CommandType.INCREMENT_PATTERN_AT_CURRENT_POSITION }
  | { type: CommandType.DECREMENT_PATTERN_AT_CURRENT_POSITION }
  | {
      type: CommandType.INSERT_SEQUENCE_POSITION_BEFORE;
      createNewPattern: boolean;
    }
  | {
      type: CommandType.INSERT_SEQUENCE_POSITION_AFTER;
      createNewPattern: boolean;
    }
  | { type: CommandType.DELETE_SEQUENCE_POSITION }
  | { type: CommandType.OPEN_INSTRUMENT_EDITOR }
  | { type: CommandType.SAVE_AND_CLOSE_INSTRUMENT_EDITOR }
  | { type: CommandType.RESTORE_AND_CLOSE_INSTRUMENT_EDITOR }
  | { type: CommandType.LOAD_SAMPLE }
  | { type: CommandType.DELETE_SAMPLE };

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
  sequenceSeek: (position: number) =>
    commandQueue.push({ type: CommandType.SEQUENCE_SEEK, position }),
  sequenceSeekForwards: () =>
    commandQueue.push({ type: CommandType.SEQUENCE_SEEK_FORWARDS }),
  sequenceSeekBackwards: () =>
    commandQueue.push({ type: CommandType.SEQUENCE_SEEK_BACKWARDS }),
  sequenceSeekToStart: () =>
    commandQueue.push({ type: CommandType.SEQUENCE_SEEK_TO_START }),
  sequenceSeekToEnd: () =>
    commandQueue.push({ type: CommandType.SEQUENCE_SEEK_TO_END }),
  nextInstrument: () =>
    commandQueue.push({ type: CommandType.NEXT_INSTRUMENT }),
  previousInstrument: () =>
    commandQueue.push({ type: CommandType.PREVIOUS_INSTRUMENT }),
  firstInstrument: () =>
    commandQueue.push({ type: CommandType.FIRST_INSTRUMENT }),
  lastInstrument: () =>
    commandQueue.push({ type: CommandType.LAST_INSTRUMENT }),
  patternGridLeft: (extendSelection: boolean) =>
    commandQueue.push({ type: CommandType.PATTERN_GRID_LEFT, extendSelection }),
  patternGridRight: (extendSelection: boolean) =>
    commandQueue.push({
      type: CommandType.PATTERN_GRID_RIGHT,
      extendSelection,
    }),
  patternGridDown: (extendSelection: boolean, wrap: boolean = false) =>
    commandQueue.push({
      type: CommandType.PATTERN_GRID_DOWN,
      extendSelection,
      wrap,
    }),
  patternGridUp: (extendSelection: boolean) =>
    commandQueue.push({ type: CommandType.PATTERN_GRID_UP, extendSelection }),
  patternGridStrideDown: (extendSelection: boolean) =>
    commandQueue.push({
      type: CommandType.PATTERN_GRID_STRIDE_DOWN,
      extendSelection,
    }),
  patternGridStrideUp: (extendSelection: boolean) =>
    commandQueue.push({
      type: CommandType.PATTERN_GRID_STRIDE_UP,
      extendSelection,
    }),
  patternGridJumpToTop: (extendSelection: boolean) =>
    commandQueue.push({
      type: CommandType.PATTERN_GRID_JUMP_TO_TOP,
      extendSelection,
    }),
  patternGridJumpToBottom: (extendSelection: boolean) =>
    commandQueue.push({
      type: CommandType.PATTERN_GRID_JUMP_TO_BOTTOM,
      extendSelection,
    }),
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
  copyPatternEvents: () =>
    commandQueue.push({ type: CommandType.COPY_PATTERN_EVENTS }),
  cutPatternEvents: () =>
    commandQueue.push({ type: CommandType.CUT_PATTERN_EVENTS }),
  pastePatternEvents: () =>
    commandQueue.push({ type: CommandType.PASTE_PATTERN_EVENTS }),
  copyTrack: () => commandQueue.push({ type: CommandType.COPY_TRACK }),
  cutTrack: () => commandQueue.push({ type: CommandType.CUT_TRACK }),
  pasteTrack: () => commandQueue.push({ type: CommandType.PASTE_TRACK }),
  copyPattern: () => commandQueue.push({ type: CommandType.COPY_PATTERN }),
  cutPattern: () => commandQueue.push({ type: CommandType.CUT_PATTERN }),
  pastePattern: () => commandQueue.push({ type: CommandType.PASTE_PATTERN }),
  undoEdit: () => commandQueue.push({ type: CommandType.UNDO_EDIT }),
  redoEdit: () => commandQueue.push({ type: CommandType.REDO_EDIT }),
  incrementPatternAtCurrentPosition: () =>
    commandQueue.push({
      type: CommandType.INCREMENT_PATTERN_AT_CURRENT_POSITION,
    }),
  decrementPatternAtCurrentPosition: () =>
    commandQueue.push({
      type: CommandType.DECREMENT_PATTERN_AT_CURRENT_POSITION,
    }),
  insertSequencePositionBefore: (createNewPattern: boolean = false) =>
    commandQueue.push({
      type: CommandType.INSERT_SEQUENCE_POSITION_BEFORE,
      createNewPattern,
    }),
  insertSequencePositionAfter: (createNewPattern: boolean = false) =>
    commandQueue.push({
      type: CommandType.INSERT_SEQUENCE_POSITION_AFTER,
      createNewPattern,
    }),
  deleteSequencePosition: () =>
    commandQueue.push({ type: CommandType.DELETE_SEQUENCE_POSITION }),
  openInstrumentEditor: () =>
    commandQueue.push({ type: CommandType.OPEN_INSTRUMENT_EDITOR }),
  saveAndCloseInstrumentEditor: () =>
    commandQueue.push({ type: CommandType.SAVE_AND_CLOSE_INSTRUMENT_EDITOR }),
  restoreAndCloseInstrumentEditor: () =>
    commandQueue.push({
      type: CommandType.RESTORE_AND_CLOSE_INSTRUMENT_EDITOR,
    }),
  loadSample: () => commandQueue.push({ type: CommandType.LOAD_SAMPLE }),
  deleteSample: () => commandQueue.push({ type: CommandType.DELETE_SAMPLE }),
};
