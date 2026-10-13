import { editor } from "./editor.ts";
import { useStore } from "../store/useStore.ts";
import { engine } from "../engine/engine.ts";
import { messageFn2 } from "../language/messages.ts";

export type ModuleTempo = { linesPerBeat: number; beatsPerMinute: number };

export const tempo = {
  editing: () => useStore.getState().editorState.editMode === "tempo",

  showDialog: () => {
    editor.setEditMode("tempo");
  },

  hideDialog: () => {
    editor.restorePreviousEditMode();
  },

  setTempo: async () => {
    const { linesPerBeat, beatsPerMinute } = useStore.getState().draftTempo;
    const newLinesPerBeat = linesPerBeat;
    const newBeatsPerMinute = (newLinesPerBeat === 0) ? 0 : beatsPerMinute;
    const oldLinesPerBeat = useStore.getState().module.linesPerBeat;
    const oldBeatsPerMinute = useStore.getState().module.beatsPerMinute;
    void editor.applyEdit({
      apply: async () => {
        await engine.setTempo(newLinesPerBeat, newBeatsPerMinute);
        useStore.getState().updateTempo(newLinesPerBeat, newBeatsPerMinute);
        return true;
      },
      undo: async () => {
        await engine.setTempo(oldLinesPerBeat, oldBeatsPerMinute);
        useStore.getState().updateTempo(oldLinesPerBeat, oldBeatsPerMinute);
      },
      undoMessage: messageFn2("setTempo")(newLinesPerBeat.toString(), newBeatsPerMinute.toString())
    });
    tempo.hideDialog();
  },
};