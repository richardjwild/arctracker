import { useStore } from "../store/useStore";
import { engine, InstrumentUpdate } from "../engine/engine.ts";
import { EditCommand, editor } from "./editor.ts";
import { AUDIO_EXPORT_EXTENSION, filePicker } from "../filesystem/filePicker.ts";
import { alerting } from "../alerting/alert.ts";
import { message } from "../language/messages.ts";
import { userMessages } from "../messages/userMessages.ts";

export type Instrument = {
  assigned: boolean;
  name: string;
  defaultVolume: number;
  transpose: number;
  repeats: boolean;
  repeatOffset: number;
  repeatLength: number;
  sample: Sample;
};

export type Sample = {
  sampleIndex: number;
  sampleLength: number;
};

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

function instrumentsEqual(a: Instrument, b: Instrument): boolean {
  return (
    a.assigned === b.assigned &&
    a.name === b.name &&
    a.defaultVolume === b.defaultVolume &&
    a.transpose === b.transpose &&
    a.repeats === b.repeats &&
    a.repeatOffset === b.repeatOffset &&
    a.repeatLength === b.repeatLength &&
    a.sample.sampleIndex === b.sample.sampleIndex
  );
}

export const editInstrument = {
  instrumentEditing: () => {
    return useStore.getState().editorState.editMode === "instrument";
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
    editor.setEditMode("instrument");
  },

  auditionInstrument: async () => {
    const {
      draftInstrument,
      selectedInstrument,
      module: { instruments },
    } = useStore.getState();
    if (selectedInstrument === null) return;
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
    const path = await filePicker.chooseFileToOpen(["wav"], message("audioFileFilterDescription"));
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
      void alerting.showErrorWithContext(message("sampleLoadFailed"), e as string);
    }
  },

  deleteSample: () => {
    if (!editInstrument.instrumentEditing()) return;
    const { setDraftInstrument } = useStore.getState();
    setDraftInstrument(emptyInstrument());
  },

  exportSample: async () => {
    const selectedInstrument = useStore.getState().selectedInstrument;
    if (selectedInstrument == null) return;
    const module = useStore.getState().module;
    const proposedFilename = filePicker.sanitiseFilename(module.instruments[selectedInstrument].name);
    const filePath = await filePicker.chooseFileToSave(
      message("exportSampleTitle"),
      proposedFilename,
      [AUDIO_EXPORT_EXTENSION],
      message("audioFileFilterDescription")
    );
    if (!filePath) return;
    try {
      await engine.exportSample(selectedInstrument, filePath);
      userMessages.logMessage({
        type: "info",
        message: message("exportSampleSucceeded"),
      })
    } catch (err) {
      void alerting.showError(err as string);
    }
  },

  closeDialog: () => {
    editor.setEditMode("none");
  },
};
