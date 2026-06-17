import { open, save } from "@tauri-apps/plugin-dialog";

export const TRACKER_MODFILE_EXTENSION = "musx";
export const DESKTOP_TRACKER_MODFILE_EXTENSION = "dskt";
export const ARCTRACKER_MODFILE_EXTENSION = "arctm";
export const AUDIO_EXPORT_EXTENSION = "wav";

export const filePicker = {
  chooseFileToOpen: async (filteredExtensions: string[]) => {
    const selected = await open({
      multiple: false,
      filters: [
        {
          name: "Tracker Modules",
          extensions: filteredExtensions,
        },
      ],
    });
    if (!selected || Array.isArray(selected)) {
      return null;
    }
    return selected;
  },

  chooseFileToSave: async (
    defaultPath: string | undefined,
    filteredExtensions: string[],
  ) => {
    return await save({
      title: "Export audio",
      defaultPath,
      filters: [
        {
          name: "Audio files",
          extensions: filteredExtensions,
        },
      ],
    });
  },

  leafName: (path: string): string => {
    return (
      path
        .split(/[\\/]/)
        .pop()
        ?.replace(/\.[^.]+$/, "") ?? ""
    );
  },
};
