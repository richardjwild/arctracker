import { editor } from "./editor.ts";
import { engine } from "../engine/engine.ts";
import { useStore } from "../store/useStore.ts";

export const nameAndAuthor = {
  showDialog: () => {
    editor.setEditMode("nameAndAuthor");
  },

  hideDialog: () => {
    editor.setEditMode("none");
  },

  setModuleTitle(name: string, author: string) {
    const module = useStore.getState().module;
    if (module.name === name && module.author === author) return;
    void editor.applyEdit({
      apply: async () => {
        await engine.setModuleTitle(name, author);
        useStore.getState().setModuleTitle(name, author);
        return true;
      },
      undo: async () => {
        await engine.setModuleTitle(module.name, module.author);
        useStore.getState().setModuleTitle(module.name, module.author);
      },
    });
  }
}