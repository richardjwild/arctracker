import { editor } from "./editor.ts";
import { useStore } from "../store/useStore.ts";
import { engine } from "../engine/engine.ts";

export type ModuleTempo = { linesPerBeat: number; beatsPerMinute: number };

export const tempo = {
  showDialog: () => {
    editor.setEditMode("tempo");
  },

  hideDialog: () => {
    editor.setEditMode("none");
  },

  setTempo: async (linesPerBeat: number, beatsPerMinute: number) => {
    await engine.setTempo(linesPerBeat, beatsPerMinute);
    useStore.getState().updateTempo(linesPerBeat, beatsPerMinute);
    tempo.hideDialog();
  },
};