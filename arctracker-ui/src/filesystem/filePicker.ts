import { open, save } from "@tauri-apps/plugin-dialog";

export const TRACKER_MODFILE_EXTENSION = "musx";
export const DESKTOP_TRACKER_MODFILE_EXTENSION = "dskt";
export const ARCTRACKER_MODFILE_EXTENSION = "arctm";
export const AUDIO_EXPORT_EXTENSION = "wav";

export const filePicker = {
  chooseFileToOpen: async (
    filteredExtensions: string[],
    filterName: string,
  ) => {
    const selected = await open({
      multiple: false,
      filters: [
        {
          name: filterName,
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
    title: string,
    defaultPath: string | undefined,
    filteredExtensions: string[],
    filterName: string,
  ) => {
    return await save({
      title,
      defaultPath,
      filters: [
        {
          name: filterName,
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

  sanitiseFilename: (filename: string): string =>
    filename.replace(/[<>:"/\\|?*\x00-\x1F]+/g, "_").replace(/[ .]+$/, ""),
};
