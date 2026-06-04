import { create } from "zustand";
import type { ExportState, Module, TransportState } from "../engine/engine.ts";
import { CurrentPattern } from "../transport/transport.ts";
import { EditorState } from "../editing/editor.ts";
import { PatternSelection } from "../editing/selection.ts";
import { PasteBufferObjectType } from "../editing/pasteBuffer.ts";

interface AppStore {
  moduleId: number;
  patternRevision: number;
  sequenceRevision: number;
  module: Module;
  sequence: number[];
  pianoKeyboardTranspose: number;
  patternGridStrideLength: number;
  transportState: TransportState;
  editorState: EditorState;
  patternSelection: PatternSelection | null;
  pasteBuffer: PasteBufferObjectType | null;
  exportMonitoring: boolean;
  exportState: ExportState | null;
  currentPattern: CurrentPattern;
  selectedSample: number | null;
  isLoadingModule: boolean;
  setModule: (result: Module) => void;
  setSequence: (sequence: number[]) => void;
  setPianoKeyboardTranspose: (octave: number) => void;
  patternRevised: () => void;
  sequenceRevised: () => void;
  setPatternGridStrideLength: (lines: number) => void;
  setTransportState: (state: TransportState) => void;
  setEditorState: (state: EditorState) => void;
  setPatternSelection: (selection: PatternSelection | null) => void;
  setPasteBuffer: (buffer: PasteBufferObjectType | null) => void;
  setExportMonitoring: (monitoring: boolean) => void;
  setExportState: (state: ExportState) => void;
  setCurrentPattern: (currentPattern: CurrentPattern) => void;
  setSelectedSample: (sampleNo: number) => void;
  setLoadingModule: (loading: boolean) => void;
}

const initialModule: Module = {
  fileName: "",
  name: "",
  author: "",
  numChannels: 0,
  numPatterns: 0,
  patternLengths: [],
  tuneLength: 1,
  numSamples: 0,
  samples: [],
};

const initialTransportState: TransportState = {
  playing: false,
  looping: false,
  sequencePos: 0,
  patternIndex: 0,
  patternNo: 0,
  patternLength: 0,
};

const initialPattern: CurrentPattern = {
  patternNo: 0,
  lines: [
    {
      row: 0,
      events: [
        {
          note: 0,
          sampleNo: 0,
          effects: Array.from({ length: 4 }, () => {
            return {
              effectCode: "",
              effectData: [0, 0],
            };
          }),
        },
      ],
    },
  ],
};

function initialEditorState(numChannels: number = 0): EditorState {
  return {
    editing: false,
    cursorPosition: {
      track: 0,
      field: 0,
      patternIndex: 0,
    },
    sequencePosition: 0,
    effectsDisplayed: Array.from({ length: numChannels }, () => 1),
  };
}

export const useStore = create<AppStore>((set) => ({
  moduleId: 0,
  patternRevision: 0,
  sequenceRevision: 0,
  module: initialModule,
  sequence: [],
  pianoKeyboardTranspose: 13,
  patternGridStrideLength: 8,
  transportState: initialTransportState,
  editorState: initialEditorState(),
  patternSelection: null,
  pasteBuffer: null,
  exportMonitoring: false,
  exportState: null,
  currentPattern: initialPattern,
  selectedSample: null,
  isLoadingModule: false,

  setModule: (result) =>
    set((state) => ({
      moduleId: state.moduleId + 1,
      patternRevision: 0,
      sequenceRevision: 0,
      module: result,
      selectedSample: result.samples.some((sample) => sample.sampleLength > 0)
        ? 0
        : null,
      selectedChannel: 0,
      isLoadingModule: false,
      editorState: initialEditorState(result.numChannels),
      patternSelection: null,
      pasteBuffer: null,
    })),

  setSequence: (sequence) => set({ sequence }),

  setPianoKeyboardTranspose: (pianoKeyboardTranspose) =>
    set({ pianoKeyboardTranspose }),

  patternRevised: (): void => {
    set((state) => {
      return { patternRevision: state.patternRevision + 1 };
    });
  },

  sequenceRevised: (): void => {
    set((state) => {
      return { sequenceRevision: state.sequenceRevision + 1 };
    });
  },

  setPatternGridStrideLength: (patternGridStrideLength) =>
    set({ patternGridStrideLength }),

  setTransportState: (next) =>
    set((state) => {
      const current = state.transportState;
      if (
        current.playing === next.playing &&
        current.looping === next.looping &&
        current.sequencePos === next.sequencePos &&
        current.patternIndex === next.patternIndex &&
        current.patternNo === next.patternNo &&
        current.patternLength === next.patternLength
      ) {
        return state;
      }
      return { transportState: next };
    }),

  setEditorState: (next) =>
    set({
      editorState: next,
    }),

  setPatternSelection: (patternSelection) => set({ patternSelection }),

  setPasteBuffer: (pasteBuffer) => set({ pasteBuffer }),

  setExportMonitoring: (exportMonitoring) => set({ exportMonitoring }),

  setExportState: (next) =>
    set((state) => {
      const current = state.exportState;
      if (
        current?.completed === next?.completed &&
        current?.percentComplete === next?.percentComplete
      ) {
        return state;
      }
      return { exportState: next };
    }),

  setCurrentPattern: (currentPattern) =>
    set({
      currentPattern,
    }),

  setSelectedSample: (selectedSample) =>
    set({
      selectedSample,
    }),

  setLoadingModule: (isLoadingModule) =>
    set({
      isLoadingModule,
    }),
}));
