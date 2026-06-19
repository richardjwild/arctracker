import { useStore } from "../store/useStore";
import {
  engine,
  Instrument,
  instrumentsEqual,
  InstrumentUpdate,
} from "../engine/engine.ts";
import { EditCommand, editor } from "./editor.ts";
import { filePicker } from "../filesystem/filePicker.ts";
import { alerting } from "../alerting/alert.ts";

export const SampleNameMaxLength = 33;

export function emptyInstrument(): Instrument {
  return {
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
    },
  };
}

export const editInstrument = {
  instrumentEditing: () => {
    return useStore.getState().editorState.instrumentEditing;
  },

  firstInstrument: () => {
    const instruments = useStore.getState().module.instruments;
    const { setSelectedInstrument } = useStore.getState();
    if (instruments.length > 0) setSelectedInstrument(0);
  },

  lastInstrument: () => {
    const instruments = useStore.getState().module.instruments;
    const { setSelectedInstrument } = useStore.getState();
    if (instruments.length > 0) setSelectedInstrument(instruments.length - 1);
  },

  nextInstrument: () => {
    const instruments = useStore.getState().module.instruments;
    const { selectedInstrument, setSelectedInstrument } = useStore.getState();
    if (
      selectedInstrument !== null &&
      selectedInstrument < instruments.length - 1
    )
      setSelectedInstrument(selectedInstrument + 1);
  },

  previousInstrument: () => {
    const { selectedInstrument, setSelectedInstrument } = useStore.getState();
    if (selectedInstrument !== null && selectedInstrument > 0)
      setSelectedInstrument(selectedInstrument - 1);
  },

  showDialog: () => {
    const editorState = useStore.getState().editorState;
    useStore.getState().setEditorState({
      ...editorState,
      instrumentEditing: true,
    });
  },

  auditionInstrument: async () => {
    const {
      draftInstrument,
      selectedInstrument,
      module: { instruments },
    } = useStore.getState();
    if (selectedInstrument === null || !draftInstrument.assigned) return;
    const instrument = instruments[selectedInstrument] || emptyInstrument();
    if (instrumentsEqual(instrument, draftInstrument)) return;
    const update: InstrumentUpdate = {
      assigned: draftInstrument.assigned,
      name: draftInstrument.name,
      defaultVolume: draftInstrument.defaultVolume,
      transpose: draftInstrument.transpose,
      repeats: draftInstrument.repeats,
      repeatOffset: draftInstrument.repeatOffset,
      repeatLength: draftInstrument.repeatLength,
      sampleIndex: draftInstrument.sample.sampleIndex,
    };
    void engine.updateInstrument(selectedInstrument, update);
  },

  updateInstrument: async () => {
    const { selectedInstrument, draftInstrument } = useStore.getState();
    if (selectedInstrument === null) return;
    const instruments = useStore.getState().module.instruments;
    const instrument = instruments[selectedInstrument] || emptyInstrument();
    const before: InstrumentUpdate = {
      assigned: instrument.assigned,
      name: instrument.name,
      defaultVolume: instrument.defaultVolume,
      transpose: instrument.transpose,
      repeats: instrument.repeats,
      repeatOffset: instrument.repeatOffset,
      repeatLength: instrument.repeatLength,
      sampleIndex: instrument.sample.sampleIndex,
    };
    const after: InstrumentUpdate = {
      assigned: draftInstrument.assigned,
      name: draftInstrument.name,
      defaultVolume: draftInstrument.defaultVolume,
      transpose: draftInstrument.transpose,
      repeats: draftInstrument.repeats,
      repeatOffset: draftInstrument.repeatOffset,
      repeatLength: draftInstrument.repeatLength,
      sampleIndex: draftInstrument.sample.sampleIndex,
    };
    const editCommand: EditCommand = {
      apply: async () => {
        await engine.updateInstrument(selectedInstrument, after);
        useStore.getState().setInstrument(selectedInstrument, draftInstrument);
        return true;
      },
      undo: async () => {
        await engine.updateInstrument(selectedInstrument, before);
        useStore.getState().setInstrument(selectedInstrument, instrument);
      },
    };
    await editor.applyEdit(editCommand);
  },

  restoreInstrument: async () => {
    const selectedInstrument = useStore.getState().selectedInstrument;
    if (selectedInstrument === null) return;
    const instrument =
      useStore.getState().module.instruments[selectedInstrument];
    const update: InstrumentUpdate = {
      assigned: instrument.assigned,
      name: instrument.name,
      defaultVolume: instrument.defaultVolume,
      transpose: instrument.transpose,
      repeats: instrument.repeats,
      repeatOffset: instrument.repeatOffset,
      repeatLength: instrument.repeatLength,
      sampleIndex: instrument.sample.sampleIndex,
    };
    await engine.updateInstrument(selectedInstrument, update);
  },

  loadSample: async () => {
    if (!editInstrument.instrumentEditing()) return;
    const { draftInstrument, setDraftInstrument } = useStore.getState();
    const path = await filePicker.chooseFileToOpen(["wav"]);
    if (!path) return;
    try {
      const sample = await engine.loadSample(path);
      const updatedDraft = {
        ...draftInstrument,
        name: filePicker.leafName(path).substring(0, SampleNameMaxLength),
        assigned: true,
        defaultVolume: 255,
        transpose: 12,
        repeats: false,
        repeatOffset: 0,
        repeatLength: 0,
        sample,
      };
      setDraftInstrument(updatedDraft);
    } catch (e) {
      void alerting.showError(`Failed to load sample: ${e}`);
    }
  },

  closeDialog: () => {
    const editorState = useStore.getState().editorState;
    useStore.getState().setEditorState({
      ...editorState,
      instrumentEditing: false,
    });
  },
};
