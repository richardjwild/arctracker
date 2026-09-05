import { CursorField } from "../editing/cursor.ts";
import { AppConfig } from "../config/appConfig.ts";

type SampleCursorField = Extract<
  CursorField,
  { field: "sampleHigh" | "sampleLow" }
>;

type EffectCodeCursorField = Extract<
  CursorField,
  { field: "effectCode1" | "effectCode2" }
>;

type EffectDataCursorField = Extract<
  CursorField,
  { field: "effectData1" | "effectData2" }
>;

export type Command =
  | { type: "Edit application config" }
  | { type: "Set application config"; newConfig: AppConfig } // TODO: Verify this one, it seems not to be used.
  | { type: "Load module" }
  | { type: "Save module as" }
  | { type: "Save module" }
  | { type: "Export audio" }
  | { type: "Create module" }
  | { type: "Create module using defaults" }
  | { type: "Toggle play" }
  | { type: "Toggle loop mode" }
  | { type: "Toggle pattern edit mode" }
  | { type: "Sequence seek"; position: number }
  | { type: "Sequence seek forwards" }
  | { type: "Sequence seek backwards" }
  | { type: "Sequence seek to start" }
  | { type: "Sequence seek to end" }
  | { type: "Next instrument" }
  | { type: "Previous instrument" }
  | { type: "First instrument" }
  | { type: "Last instrument" }
  | { type: "Pattern grid left"; extendSelection: boolean }
  | { type: "Pattern grid right"; extendSelection: boolean }
  | {
      type: "Pattern grid down";
      extendSelection: boolean;
      wrap: boolean;
    }
  | { type: "Pattern grid up"; extendSelection: boolean }
  | { type: "Pattern grid stride down"; extendSelection: boolean }
  | { type: "Pattern grid stride up"; extendSelection: boolean }
  | { type: "Pattern grid jump to top"; extendSelection: boolean }
  | { type: "Pattern grid jump to bottom"; extendSelection: boolean }
  | {
      type: "Pattern grid jump to location";
      track: number;
      patternIndex: number;
      extendSelection: boolean;
    }
  | { type: "Cursor field left" }
  | { type: "Cursor field right" }
  | { type: "Increase effects displayed" }
  | { type: "Decrease effects displayed" }
  | { type: "Edit note field"; note: number }
  | {
      type: "Edit sample field";
      field: SampleCursorField;
      value: string;
    }
  | {
      type: "Edit effect code";
      field: EffectCodeCursorField;
      value: string;
    }
  | {
      type: "Edit effect data";
      field: EffectDataCursorField;
      value: string;
    }
  | { type: "Clear pattern event field" }
  | { type: "Clear pattern event" }
  | { type: "Copy pattern events" }
  | { type: "Cut pattern events" }
  | { type: "Paste pattern events" }
  | { type: "Copy track" }
  | { type: "Cut track" }
  | { type: "Paste track" }
  | { type: "Copy pattern" }
  | { type: "Cut pattern" }
  | { type: "Paste pattern" }
  | { type: "Undo edit" }
  | { type: "Redo edit" }
  | { type: "Increment pattern at current position" }
  | { type: "Decrement pattern at current position" }
  | {
      type: "Insert sequence position before";
      createNewPattern: boolean;
    }
  | {
      type: "Insert sequence position after";
      createNewPattern: boolean;
    }
  | { type: "Delete sequence position" }
  | { type: "Add instrument" }
  | { type: "Open instrument editor" }
  | { type: "Save and close instrument editor" }
  | { type: "Restore and close instrument editor" }
  | { type: "Load sample" }
  | { type: "Delete sample" }
  | { type: "Export sample" }
  | { type: "Edit current pattern length" }
  | {
      type: "Set current pattern length";
      newLength: number;
    }
  | { type: "Edit module metadata" }
  | { type: "Set module metadata" }
  | { type: "Edit track count" }
  | { type: "Set track count"; trackCount: number }
  | { type: "Edit tempo" }
  | { type: "Set tempo" }
  | { type: "Toggle current track mute" }
  | { type: "Toggle track mute"; track: number }
  | { type: "Shift keyboard octave up" }
  | { type: "Shift keyboard octave down" }
  | { type: "Open hex calculator" }
  | { type: "Close hex calculator" };

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
  editAppConfig: () => commandQueue.push({ type: "Edit application config" }),
  loadFile: () => commandQueue.push({ type: "Load module" }),
  saveModuleAs: () => commandQueue.push({ type: "Save module as" }),
  saveModule: () => commandQueue.push({ type: "Save module" }),
  exportAudio: () => commandQueue.push({ type: "Export audio" }),
  createModule: () => commandQueue.push({ type: "Create module" }),
  createModuleUsingDefaults: () =>
    commandQueue.push({ type: "Create module using defaults" }),
  togglePlay: () => commandQueue.push({ type: "Toggle play" }),
  toggleLoop: () => commandQueue.push({ type: "Toggle loop mode" }),
  toggleEdit: () => commandQueue.push({ type: "Toggle pattern edit mode" }),
  sequenceSeek: (position: number) =>
    commandQueue.push({ type: "Sequence seek", position }),
  sequenceSeekForwards: () =>
    commandQueue.push({ type: "Sequence seek forwards" }),
  sequenceSeekBackwards: () =>
    commandQueue.push({ type: "Sequence seek backwards" }),
  sequenceSeekToStart: () =>
    commandQueue.push({ type: "Sequence seek to start" }),
  sequenceSeekToEnd: () => commandQueue.push({ type: "Sequence seek to end" }),
  nextInstrument: () => commandQueue.push({ type: "Next instrument" }),
  previousInstrument: () => commandQueue.push({ type: "Previous instrument" }),
  firstInstrument: () => commandQueue.push({ type: "First instrument" }),
  lastInstrument: () => commandQueue.push({ type: "Last instrument" }),
  patternGridLeft: (extendSelection: boolean) =>
    commandQueue.push({ type: "Pattern grid left", extendSelection }),
  patternGridRight: (extendSelection: boolean) =>
    commandQueue.push({
      type: "Pattern grid right",
      extendSelection,
    }),
  patternGridDown: (extendSelection: boolean, wrap: boolean = false) =>
    commandQueue.push({
      type: "Pattern grid down",
      extendSelection,
      wrap,
    }),
  patternGridUp: (extendSelection: boolean) =>
    commandQueue.push({ type: "Pattern grid up", extendSelection }),
  patternGridStrideDown: (extendSelection: boolean) =>
    commandQueue.push({
      type: "Pattern grid stride down",
      extendSelection,
    }),
  patternGridStrideUp: (extendSelection: boolean) =>
    commandQueue.push({
      type: "Pattern grid stride up",
      extendSelection,
    }),
  patternGridJumpToTop: (extendSelection: boolean) =>
    commandQueue.push({
      type: "Pattern grid jump to top",
      extendSelection,
    }),
  patternGridJumpToBottom: (extendSelection: boolean) =>
    commandQueue.push({
      type: "Pattern grid jump to bottom",
      extendSelection,
    }),
  patternGridJumpToLocation: (
    track: number,
    patternIndex: number,
    extendSelection: boolean,
  ) =>
    commandQueue.push({
      type: "Pattern grid jump to location",
      track,
      patternIndex,
      extendSelection,
    }),
  cursorFieldLeft: () => commandQueue.push({ type: "Cursor field left" }),
  cursorFieldRight: () => commandQueue.push({ type: "Cursor field right" }),
  increaseEffectsDisplayed: () =>
    commandQueue.push({ type: "Increase effects displayed" }),
  decreaseEffectsDisplayed: () =>
    commandQueue.push({ type: "Decrease effects displayed" }),
  editNoteField: (note: number) =>
    commandQueue.push({ type: "Edit note field", note }),
  editSampleField: (field: SampleCursorField, value: string) =>
    commandQueue.push({ type: "Edit sample field", field, value }),
  editEffectCode: (field: EffectCodeCursorField, value: string) =>
    commandQueue.push({ type: "Edit effect code", field, value }),
  editEffectData: (field: EffectDataCursorField, value: string) =>
    commandQueue.push({ type: "Edit effect data", field, value }),
  clearPatternEventField: () =>
    commandQueue.push({ type: "Clear pattern event field" }),
  clearPatternEvent: () => commandQueue.push({ type: "Clear pattern event" }),
  copyPatternEvents: () => commandQueue.push({ type: "Copy pattern events" }),
  cutPatternEvents: () => commandQueue.push({ type: "Cut pattern events" }),
  pastePatternEvents: () => commandQueue.push({ type: "Paste pattern events" }),
  copyTrack: () => commandQueue.push({ type: "Copy track" }),
  cutTrack: () => commandQueue.push({ type: "Cut track" }),
  pasteTrack: () => commandQueue.push({ type: "Paste track" }),
  copyPattern: () => commandQueue.push({ type: "Copy pattern" }),
  cutPattern: () => commandQueue.push({ type: "Cut pattern" }),
  pastePattern: () => commandQueue.push({ type: "Paste pattern" }),
  undoEdit: () => commandQueue.push({ type: "Undo edit" }),
  redoEdit: () => commandQueue.push({ type: "Redo edit" }),
  incrementPatternAtCurrentPosition: () =>
    commandQueue.push({
      type: "Increment pattern at current position",
    }),
  decrementPatternAtCurrentPosition: () =>
    commandQueue.push({
      type: "Decrement pattern at current position",
    }),
  insertSequencePositionBefore: (createNewPattern: boolean = false) =>
    commandQueue.push({
      type: "Insert sequence position before",
      createNewPattern,
    }),
  insertSequencePositionAfter: (createNewPattern: boolean = false) =>
    commandQueue.push({
      type: "Insert sequence position after",
      createNewPattern,
    }),
  deleteSequencePosition: () =>
    commandQueue.push({ type: "Delete sequence position" }),
  addInstrument: () => commandQueue.push({ type: "Add instrument" }),
  openInstrumentEditor: () =>
    commandQueue.push({ type: "Open instrument editor" }),
  saveAndCloseInstrumentEditor: () =>
    commandQueue.push({ type: "Save and close instrument editor" }),
  restoreAndCloseInstrumentEditor: () =>
    commandQueue.push({
      type: "Restore and close instrument editor",
    }),
  loadSample: () => commandQueue.push({ type: "Load sample" }),
  deleteSample: () => commandQueue.push({ type: "Delete sample" }),
  exportSample: () => commandQueue.push({ type: "Export sample" }),
  editCurrentPatternLength: () =>
    commandQueue.push({ type: "Edit current pattern length" }),
  setCurrentPatternLength: (newLength: number) =>
    commandQueue.push({
      type: "Set current pattern length",
      newLength,
    }),
  editModuleMetaData: () => commandQueue.push({ type: "Edit module metadata" }),
  setModuleMetaData: () => commandQueue.push({ type: "Set module metadata" }),
  editTrackCount: () => commandQueue.push({ type: "Edit track count" }),
  setTrackCount: (trackCount: number) =>
    commandQueue.push({ type: "Set track count", trackCount }),
  editTempo: () => commandQueue.push({ type: "Edit tempo" }),
  setTempo: () => commandQueue.push({ type: "Set tempo" }),
  toggleCurrentTrackMute: () =>
    commandQueue.push({ type: "Toggle current track mute" }),
  toggleTrackMute: (track: number) =>
    commandQueue.push({ type: "Toggle track mute", track }),
  shiftKeyboardOctaveUp: () =>
    commandQueue.push({ type: "Shift keyboard octave up" }),
  shiftKeyboardOctaveDown: () =>
    commandQueue.push({ type: "Shift keyboard octave down" }),
  openHexCalculator: () => commandQueue.push({ type: "Open hex calculator" }),
  closeHexCalculator: () => commandQueue.push({ type: "Close hex calculator" }),
};
