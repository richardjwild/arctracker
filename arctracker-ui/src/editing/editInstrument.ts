import { useStore } from "../store/useStore";
import { engine, Instrument, InstrumentUpdate } from "../engine/engine.ts";
import { EditCommand, editor } from "./editor.ts";

export const editInstrument = {
  showDialog: () => {
    const editorState = useStore.getState().editorState;
    useStore.getState().setEditorState({
      ...editorState,
      instrumentEditing: true,
    });
  },

  updateInstrument: async (instrumentIndex: number, draft: Instrument) => {
    console.log('Updating instrument', instrumentIndex, draft)
    const instrument = useStore.getState().module.instruments[instrumentIndex];
    const before: InstrumentUpdate = {
      assigned: instrument.assigned,
      name: instrument.name,
      defaultVolume: instrument.defaultVolume,
      transpose: instrument.transpose,
      repeats: instrument.repeats,
      repeatOffset: instrument.repeatOffset,
      repeatLength: instrument.repeatLength,
      sampleIndex: instrument.sampleIndex,
    };
    const after: InstrumentUpdate = {
      assigned: draft.assigned,
      name: draft.name,
      defaultVolume: draft.defaultVolume,
      transpose: draft.transpose,
      repeats: draft.repeats,
      repeatOffset: draft.repeatOffset,
      repeatLength: draft.repeatLength,
      sampleIndex: draft.sampleIndex,
    };
    const editCommand: EditCommand = {
      apply: async () => {
        await engine.updateInstrument(instrumentIndex, after);
        useStore.getState().updateInstrument(instrumentIndex, after);
        return true;
      },
      undo: async () => {
        await engine.updateInstrument(instrumentIndex, before);
        useStore.getState().updateInstrument(instrumentIndex, before);
      },
    }
    await editor.applyEdit(editCommand);
  },

  restoreInstrument: async (instrumentIndex: number) => {
    console.log('Restoring instrument', instrumentIndex);
    const instrument = useStore.getState().module.instruments[instrumentIndex];
    const update: InstrumentUpdate = {
      assigned: instrument.assigned,
      name: instrument.name,
      defaultVolume: instrument.defaultVolume,
      transpose: instrument.transpose,
      repeats: instrument.repeats,
      repeatOffset: instrument.repeatOffset,
      repeatLength: instrument.repeatLength,
      sampleIndex: instrument.sampleIndex,
    };
    await engine.updateInstrument(instrumentIndex, update);
  },

  closeDialog: () => {
    const editorState = useStore.getState().editorState;
    useStore.getState().setEditorState({
      ...editorState,
      instrumentEditing: false,
    });
  }
};
