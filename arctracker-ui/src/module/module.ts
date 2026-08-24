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
import { message, messageFn } from "../language/messages.ts";
import { appConfig } from "../config/appConfig.ts";
import { InterpolationType } from "../editing/moduleMetaData.ts";
import { userMessages } from "../messages/userMessages.ts";

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
  interpolationType: InterpolationType;
};

export type NewModuleParams = {
  numTracks: number;
  defaultPatternLength: number;
  linesPerBeat: number;
  beatsPerMinute: number;
  author: string;
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
          userMessages.logMessage({
            type: "info",
            message: messageFn("moduleLoadedSuccessfully")(module.fileName),
          });
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
        userMessages.logMessage({
          type: "info",
          message: messageFn("moduleSavedSuccessfully")(module.fileName),
        });
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
      message("saveModuleTitle"),
      defaultPath,
      [ARCTRACKER_MODFILE_EXTENSION],
      message("moduleFileFilterDescription")
    );
    if (!filePath) return;
    try {
      await engine.saveModule(filePath, FormatArctracker);
      useStore.getState().setModuleFilename(filePath);
      editor.allChangesSaved();
      userMessages.logMessage({
        type: "info",
        message: messageFn("moduleSavedSuccessfully")(filePath),
      });
    } catch (err) {
      void alerting.showError(err as string);
    }
  },

  create: async (usingDefaults: boolean): Promise<boolean> => {
    if (!(await okToDiscardModule())) return false;
    const config = appConfig.get();
    const params: NewModuleParams = {
      numTracks: config.defaultTrackCount,
      defaultPatternLength: config.defaultPatternLength,
      linesPerBeat: config.defaultLinesPerBeat,
      beatsPerMinute: config.defaultBeatsPerMinute,
      author: config.defaultAuthorName || "",
    }
    try {
      const newModule = usingDefaults
        ? await engine.createModuleUsingDefaults(params)
        : await engine.createModule();
      if (newModule) {
        useStore.getState().replaceModule(newModule);
        editor.newModuleLoaded();
        userMessages.logMessage({
          type: "info",
          message: message("moduleCreatedSuccessfully"),
        });
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
          // TODO: Don't clear the new tracks if the undo operation increases the track count.
          await engine.setNumTracks(currentTracks);
          useStore.getState().updateTracks(currentTracks);
        },
      });
    }
    editor.setEditMode("none");
  },
};
