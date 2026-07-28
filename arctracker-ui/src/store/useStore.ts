import { create } from "zustand";
import { CurrentPattern, TransportState } from "../transport/transport.ts";
import { EditorState } from "../editing/editor.ts";
import { PatternSelection } from "../editing/selection.ts";
import { PasteBufferObjectType } from "../editing/pasteBuffer.ts";
import { ModuleTitle } from "../editing/moduleTitle.ts";
import { Instrument } from "../editing/editInstrument.ts";
import { Module } from "../module/module.ts";
import { ExportState } from "../audioExport/audioExport.ts";

interface AppStore {
  moduleId: number;
  patternRevision: number;
  module: Module;
  sequence: number[];
  pianoKeyboardTranspose: number;
  patternGridStrideLength: number;
  transportState: TransportState;
  editorState: EditorState;
  effectsDisplayed: number[];
  trackMuteState: boolean[];
  trackPanning: number[];
  patternSelection: PatternSelection | null;
  pasteBuffer: PasteBufferObjectType | null;
  exportMonitoring: boolean;
  exportState: ExportState | null;
  currentPattern: CurrentPattern;
  selectedInstrument: number | null;
  draftInstrument: Instrument;
  draftModuleTitle: ModuleTitle | null;
  isLoadingModule: boolean;
  replaceModule: (module: Module) => void;
  setMasterGain: (gain: number) => void;
  setModuleFilename: (fileName: string) => void;
  setModuleTitle: (name: string, author: string) => void;
  updateTracks: (numTracks: number) => void;
  updatePatterns: (numPatterns: number, patternLengths: number[]) => void;
  setSequence: (sequence: number[]) => void;
  setInstrument: (instrumentIndex: number, instrument: Instrument) => void;
  setDraftInstrument: (instrument: Instrument) => void;
  setDraftModuleTitle: (moduleTitle: ModuleTitle) => void;
  setPianoKeyboardTranspose: (octave: number) => void;
  patternRevised: () => void;
  setPatternGridStrideLength: (lines: number) => void;
  setTransportState: (state: TransportState) => void;
  setEditorState: (state: EditorState) => void;
  setEffectsDisplayed: (effectsDisplayed: number[]) => void;
  setTrackMuteState: (trackMuteState: boolean[]) => void;
  setTrackPanning: (trackPanning: number[]) => void;
  setPatternSelection: (selection: PatternSelection | null) => void;
  setPasteBuffer: (buffer: PasteBufferObjectType | null) => void;
  setExportMonitoring: (monitoring: boolean) => void;
  setExportState: (state: ExportState) => void;
  setCurrentPattern: (currentPattern: CurrentPattern) => void;
  setSelectedInstrument: (instrument: number) => void;
  setLoadingModule: (loading: boolean) => void;
}

const initialModule: Module = {
  fileName: "",
  name: "",
  author: "",
  numTracks: 0,
  numPatterns: 0,
  patternLengths: [],
  tuneLength: 1,
  instruments: [],
  masterGain: 1.0,
};

const initialTransportState: TransportState = {
  playing: false,
  looping: false,
  sequencePos: 0,
  patternIndex: 0,
  // patternNo: 0,
  // patternLength: 0,
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

const initialInstrument: Instrument = {
  name: "",
  assigned: false,
  defaultVolume: 255,
  transpose: 13,
  repeats: false,
  repeatOffset: 0,
  repeatLength: 0,
  sample: {
    sampleIndex: 0,
    sampleLength: 0,
  }
};


function initialEditorState(): EditorState {
  return {
    editMode: "none",
    inputtingText: false,
    cursorPosition: {
      track: 0,
      field: 0,
      patternIndex: 0,
    },
    sequencePosition: 0,
  };
}

function arraysEqual<T>(a: T[], b: T[]): boolean {
  if (a.length !== b.length) return false;
  for (let i = 0; i < a.length; i++) {
    if (a[i] !== b[i]) return false;
  }
  return true;
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
  effectsDisplayed: [],
  trackMuteState: [],
  trackPanning: [],
  patternSelection: null,
  pasteBuffer: null,
  exportMonitoring: false,
  exportState: null,
  currentPattern: initialPattern,
  selectedInstrument: null,
  draftInstrument: initialInstrument,
  draftModuleTitle: null,
  isLoadingModule: false,

  replaceModule: (result) => {
    set((state) => ({
      moduleId: state.moduleId + 1,
      patternRevision: 0,
      sequenceRevision: 0,
      module: result,
      sequence: [],
      selectedInstrument:
        result.instruments.findIndex((i: Instrument) => i.assigned) >= 0
          ? result.instruments.findIndex((i: Instrument) => i.assigned)
          : null,
      selectedChannel: 0,
      isLoadingModule: false,
      editorState: initialEditorState(),
      effectsDisplayed: Array.from({ length: result.numTracks }, () => 1),
      patternSelection: null,
      pasteBuffer: null,
    }));
  },

  setMasterGain: (gain: number) =>
    set((state) => ({
      module: {
        ...state.module,
        masterGain: gain,
      },
    })),

  setModuleFilename: (fileName: string) =>
    set((state) => ({
      module: {
        ...state.module,
        fileName,
      },
    })),

  setModuleTitle: (name: string, author: string) =>
    set((state) => ({
      module: {
        ...state.module,
        name,
        author,
      },
    })),

  updateTracks: (numTracks: number) =>
    set((state) => ({
      module: {
        ...state.module,
        numTracks,
      },
    })),

  updatePatterns: (numPatterns: number, patternLengths: number[]) =>
    set((state) => ({
      module: {
        ...state.module,
        numPatterns,
        patternLengths,
      },
    })),

  setSequence: (sequence) => set({ sequence }),

  setInstrument: (instrumentIndex: number, instrument: Instrument) =>
    set((state) => {
      let instruments = [...state.module.instruments];
      instruments[instrumentIndex] = {
        ...instruments[instrumentIndex],
        assigned: instrument.assigned,
        name: instrument.name,
        defaultVolume: instrument.defaultVolume,
        transpose: instrument.transpose,
        repeats: instrument.repeats,
        repeatOffset: instrument.repeatOffset,
        repeatLength: instrument.repeatLength,
        sample: {
          sampleIndex: instrument.sample.sampleIndex,
          sampleLength: instrument.sample.sampleLength,
        },
      };
      return {
        module: {
          ...state.module,
          instruments,
        },
      };
    }),

  setDraftInstrument: (draftInstrument) => set({ draftInstrument }),

  setDraftModuleTitle: (draftModuleTitle) => set({ draftModuleTitle }),

  setPianoKeyboardTranspose: (pianoKeyboardTranspose) =>
    set({ pianoKeyboardTranspose }),

  patternRevised: (): void => {
    set((state) => {
      return { patternRevision: state.patternRevision + 1 };
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
        current.patternIndex === next.patternIndex
      ) {
        return state;
      }
      return { transportState: next };
    }),

  setEditorState: (next) =>
    set({
      editorState: next,
    }),

  setEffectsDisplayed: (next) =>
    set((state) => {
      if (arraysEqual(state.effectsDisplayed, next)) return state;
      else return { effectsDisplayed: next };
    }),

  setTrackMuteState: (next) =>
    set((state) => {
      if (arraysEqual(state.trackMuteState, next)) return state;
      else return { trackMuteState: next };
    }),

  setTrackPanning: (next) =>
    set((state) => {
      if (arraysEqual(state.trackPanning, next)) return state;
      else return { trackPanning: next };
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

  setSelectedInstrument: (selectedInstrument) =>
    set({
      selectedInstrument,
    }),

  setLoadingModule: (isLoadingModule) =>
    set({
      isLoadingModule,
    }),
}));
