import { editor } from "./editor.ts";
import { engine } from "../engine/engine.ts";
import { useStore } from "../store/useStore.ts";
import { transport } from "../transport/transport.ts";

export type InterpolationType = "ARCTRACKER" | "ARCHIMEDES";

export type VolumeMappingType = "ARCHIMEDES" | "AMIGA";

export type ModuleMetaData = {
  moduleName: string;
  author: string;
  defaultPatternLength: number;
  interpolationType: InterpolationType;
  volumeMappingType: VolumeMappingType;
};

export const moduleMetaData = {
  editing: () => useStore.getState().editorState.editMode === "moduleMetaData",

  showDialog: () => {
    if (transport.playing()) transport.togglePlay();
    editor.setEditMode("moduleMetaData");
  },

  hideDialog: () => {
    editor.setEditMode("none");
  },

  setModuleMetaData() {
    const module = useStore.getState().module;
    const draftModuleMetaData = useStore.getState().draftModuleMetaData;
    if (!draftModuleMetaData) return;
    if (
      module.name === draftModuleMetaData.moduleName &&
      module.author === draftModuleMetaData.author &&
      module.defaultPatternLength === draftModuleMetaData.defaultPatternLength &&
      module.interpolationType === draftModuleMetaData.interpolationType &&
      module.volumeMapping === draftModuleMetaData.volumeMappingType
    )
      return;
    void editor.applyEdit({
      apply: async () => {
        await engine.setModuleMetaData(
          draftModuleMetaData.moduleName,
          draftModuleMetaData.author,
          draftModuleMetaData.defaultPatternLength,
          draftModuleMetaData.interpolationType,
          draftModuleMetaData.volumeMappingType,
        );
        useStore.getState().setModuleMetaData(
          draftModuleMetaData.moduleName,
          draftModuleMetaData.author,
          draftModuleMetaData.defaultPatternLength,
          draftModuleMetaData.interpolationType,
          draftModuleMetaData.volumeMappingType,
        );
        return true;
      },
      undo: async () => {
        await engine.setModuleMetaData(
          module.name,
          module.author,
          module.defaultPatternLength,
          module.interpolationType,
          module.volumeMapping,
        );
        useStore.getState().setModuleMetaData(
          module.name,
          module.author,
          module.defaultPatternLength,
          module.interpolationType,
          module.volumeMapping,
        );
      },
    });
  },
};
