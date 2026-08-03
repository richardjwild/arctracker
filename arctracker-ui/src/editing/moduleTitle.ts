import { editor } from "./editor.ts";
import { engine } from "../engine/engine.ts";
import { useStore } from "../store/useStore.ts";

export type ModuleTitle = {
  moduleName: string;
  author: string;
  defaultPatternLength: number;
};

export const moduleTitle = {
  editing: () => useStore.getState().editorState.editMode === "nameAndAuthor",

  showDialog: () => {
    editor.setEditMode("nameAndAuthor");
  },

  hideDialog: () => {
    editor.setEditMode("none");
  },

  setModuleTitle() {
    const module = useStore.getState().module;
    const draftModuleTitle = useStore.getState().draftModuleTitle;
    if (!draftModuleTitle) return;
    if (
      module.name === draftModuleTitle.moduleName &&
      module.author === draftModuleTitle.author &&
      module.defaultPatternLength === draftModuleTitle.defaultPatternLength
    )
      return;
    void editor.applyEdit({
      apply: async () => {
        await engine.setModuleTitle(
          draftModuleTitle.moduleName,
          draftModuleTitle.author,
          draftModuleTitle.defaultPatternLength,
        );
        useStore
          .getState()
          .setModuleTitle(
            draftModuleTitle.moduleName,
            draftModuleTitle.author,
            draftModuleTitle.defaultPatternLength,
          );
        return true;
      },
      undo: async () => {
        await engine.setModuleTitle(
          module.name,
          module.author,
          module.defaultPatternLength,
        );
        useStore.getState().setModuleTitle(
          module.name,
          module.author,
          module.defaultPatternLength,
        );
      },
    });
  },
};
