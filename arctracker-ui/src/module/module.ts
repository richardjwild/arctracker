import { engine } from "../engine/engine.ts";
import { useStore } from "../store/useStore.ts";
import {
  filePicker,
  ARCTRACKER_MODFILE_EXTENSION,
  DESKTOP_TRACKER_MODFILE_EXTENSION,
  TRACKER_MODFILE_EXTENSION,
} from "../filesystem/filePicker.ts";
import { alerting } from "../alerting/alert.ts";
import { editor } from "../editing/editor.ts";
import { commands } from "../control/commands.ts";
import { Instrument } from "../editing/editInstrument.ts";
import { message } from "../language/messages.ts";

export type Module = {
  fileName: string | null;
  name: string;
  author: string;
  numTracks: number;
  numPatterns: number;
  patternLengths: number[];
  tuneLength: number;
  instruments: Instrument[];
  masterGain: number;
  defaultPatternLength: number;
  linesPerBeat: number;
  beatsPerMinute: number;
};

const FormatArctracker = 0;

async function okToDiscardModule() {
  if (!editor.hasUnsavedChanges()) return true;
  return await alerting.askConfirmation(message("unsavedChanges"));
}

export const module = {
  getCurrent: async () => {
    engine
      .getCurrentModule()
      .then((module) => useStore.getState().replaceModule(module));
  },

  load: async (): Promise<boolean> => {
    if (!(await okToDiscardModule())) return false;
    const { setLoadingModule, replaceModule } = useStore.getState();
    setLoadingModule(true);
    try {
      const selected = await filePicker.chooseFileToOpen(
        [
          ARCTRACKER_MODFILE_EXTENSION,
          TRACKER_MODFILE_EXTENSION,
          DESKTOP_TRACKER_MODFILE_EXTENSION,
        ],
        message("moduleFileFilterDescription"),
      );
      if (selected) {
        let module = await engine.loadModule(selected);
        if (module) {
          module.fileName = selected;
          replaceModule(module);
        }
      }
      return true;
    } catch (err) {
      await alerting.showError(err as string);
      return false;
    } finally {
      setLoadingModule(false);
    }
  },

  save: async () => {
    const module = useStore.getState().module;
    if (
      module.fileName &&
      module.fileName.endsWith(`.${ARCTRACKER_MODFILE_EXTENSION}`)
    ) {
      try {
        await engine.saveModule(module.fileName, FormatArctracker);
        editor.allChangesSaved();
      } catch (err) {
        void alerting.showError(err as string);
      }
    } else {
      commands.saveModuleAs();
    }
  },

  saveAs: async () => {
    const module = useStore.getState().module;
    const defaultPath = await engine.defaultSavePath(
      module.fileName || module.name,
    );
    const filePath = await filePicker.chooseFileToSave(
      "Save module",
      defaultPath,
      [ARCTRACKER_MODFILE_EXTENSION],
      message("moduleFileFilterDescription")
    );
    if (!filePath) return;
    try {
      await engine.saveModule(filePath, FormatArctracker);
      useStore.getState().setModuleFilename(filePath);
      editor.allChangesSaved();
    } catch (err) {
      void alerting.showError(err as string);
    }
  },

  create: async (): Promise<boolean> => {
    if (!(await okToDiscardModule())) return false;
    const numTracks = 8; // TODO: Make this a configuration option.
    try {
      const newModule = await engine.createModule(numTracks);
      if (newModule) {
        useStore.getState().replaceModule(newModule);
        editor.newModuleLoaded();
      }
      return true;
    } catch (err) {
      await alerting.showError(err as string);
      return false;
    }
  },

  editTrackCount: () => editor.setEditMode("trackCount"),

  setTrackCount: async (trackCount: number) => {
    const module = useStore.getState().module;
    const currentTracks = module.numTracks;
    if (currentTracks !== trackCount) {
      void editor.applyEdit({
        apply: async () => {
          await engine.setNumTracks(trackCount);
          useStore.getState().updateTracks(trackCount);
          return true;
        },
        undo: async () => {
          await engine.setNumTracks(currentTracks);
          useStore.getState().updateTracks(currentTracks);
        },
      });
    }
    editor.setEditMode("none");
  },
};
