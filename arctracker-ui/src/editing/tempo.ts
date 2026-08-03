import { editor } from "./editor.ts";
import { useStore } from "../store/useStore.ts";
import { engine } from "../engine/engine.ts";

export type ModuleTempo = { linesPerBeat: number; beatsPerMinute: number };

export const tempo = {
  editing: () => useStore.getState().editorState.editMode === "tempo",

  showDialog: () => {
    editor.setEditMode("tempo");
  },

  hideDialog: () => {
    editor.setEditMode("none");
  },

  setTempo: async () => {
    const { linesPerBeat, beatsPerMinute } = useStore.getState().draftTempo;
    const oldLinesPerBeat = useStore.getState().module.linesPerBeat;
    const oldBeatsPerMinute = useStore.getState().module.beatsPerMinute;
    void editor.applyEdit({
      apply: async () => {
        await engine.setTempo(linesPerBeat, beatsPerMinute);
        useStore.getState().updateTempo(linesPerBeat, beatsPerMinute);
        return true;
      },
      undo: async () => {
        await engine.setTempo(oldLinesPerBeat, oldBeatsPerMinute);
        useStore.getState().updateTempo(oldLinesPerBeat, oldBeatsPerMinute);
      },
    });
    tempo.hideDialog();
  },
};