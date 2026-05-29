import { create } from "zustand";
import type { ExportState, Module, TransportState } from "../engine/engine.ts";
import { CurrentPattern } from "../transport/transport.ts";
import { EditorState } from "../editing/editor.ts";

interface AppStore {
  moduleId: number;
  moduleRevision: number;
  module: Module;
  pianoKeyboardTranspose: number;
  patternGridStrideLength: number;
  transportState: TransportState;
  editorState: EditorState;
  exportMonitoring: boolean;
  exportState: ExportState | null;
  currentPattern: CurrentPattern | null;
  selectedSample: number | null;
  isLoadingModule: boolean;
  setModule: (result: Module) => void;
  setPianoKeyboardTranspose: (octave: number) => void;
  moduleRevised: () => void;
  setPatternGridStrideLength: (lines: number) => void;
  setTransportState: (state: TransportState) => void;
  setEditorState: (state: EditorState) => void;
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
  tuneLength: 0,
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

function initialEditorState(numChannels: number = 0): EditorState {
  return {
    editing: false,
    cursorPosition: {
      track: 0,
      field: 0,
    },
    effectsDisplayed: Array.from({ length: numChannels }, () => 1),
  };
}

export const useStore = create<AppStore>((set) => ({
  moduleId: 0,
  moduleRevision: 0,
  module: initialModule,
  pianoKeyboardTranspose: 13,
  patternGridStrideLength: 8,
  transportState: initialTransportState,
  editorState: initialEditorState(),
  exportMonitoring: false,
  exportState: null,
  currentPattern: null,
  selectedSample: null,
  isLoadingModule: false,

  setModule: (result) =>
    set((state) => ({
      moduleId: state.moduleId + 1,
      moduleRevision: 0,
      module: result,
      selectedSample: result.samples.some((sample) => sample.sampleLength > 0)
        ? 0
        : null,
      selectedChannel: 0,
      isLoadingModule: false,
      editorState: initialEditorState(result.numChannels),
    })),

  setPianoKeyboardTranspose: (pianoKeyboardTranspose) =>
    set({ pianoKeyboardTranspose }),

  moduleRevised: (): void => {
    set((state) => {
      return { moduleRevision: state.moduleRevision + 1 };
    });
  },

  setPatternGridStrideLength: (patternGridStrideLength) => set({ patternGridStrideLength }),

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
