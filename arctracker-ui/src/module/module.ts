import { engine } from "../engine/engine.ts";
import { useStore } from "../store/useStore.ts";
import {
  filePicker,
  ARCTRACKER_MODFILE_EXTENSION,
  DESKTOP_TRACKER_MODFILE_EXTENSION,
  TRACKER_MODFILE_EXTENSION,
} from "../filesystem/filePicker.ts";
import { alerting } from "../alerting/alert.ts";

export type { Module, Sample } from "../engine/engine.ts";

export const module = {
  getCurrent: async () => {
    engine
      .getCurrentModule()
      .then((module) => useStore.getState().setModule(module));
  },

  load: async (): Promise<boolean> => {
    const { setLoadingModule, setModule } = useStore.getState();
    setLoadingModule(true);
    try {
      const selected = await filePicker.chooseFileToOpen([
        ARCTRACKER_MODFILE_EXTENSION,
        TRACKER_MODFILE_EXTENSION,
        DESKTOP_TRACKER_MODFILE_EXTENSION,
      ]);
      if (selected) {
        let module = await engine.loadModule(selected);
        if (module) {
          module.fileName = selected;
          setModule(module);
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

  create: async (numTracks: number): Promise<boolean> => {
    try {
      const newModule = await engine.createModule(numTracks);
      if (newModule) useStore.getState().setModule(newModule);
      return true;
    } catch (err) {
      await alerting.showError(err as string);
      return false;
    }
  },
};
