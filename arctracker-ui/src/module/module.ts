import { message } from "@tauri-apps/plugin-dialog";
import { engine } from "../engine/engine.ts";
import { useStore } from "../store/useStore.ts";
import {
  filePicker,
  ARCTRACKER_MODFILE_EXTENSION,
  DESKTOP_TRACKER_MODFILE_EXTENSION,
  TRACKER_MODFILE_EXTENSION,
} from "../filesystem/filePicker.ts";

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
      await message(err as string, { title: "Arctracker", kind: "error" });
      return false;
    } finally {
      setLoadingModule(false);
    }
  },

  create: async (numChannels: number): Promise<boolean> => {
    try {
      const newModule = await engine.createModule(numChannels);
      if (newModule) useStore.getState().setModule(newModule);
      return true;
    } catch (err) {
      await message(err as string, { title: "Arctracker", kind: "error" });
      return false;
    }
  },
};
