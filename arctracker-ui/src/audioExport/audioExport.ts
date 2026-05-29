import { engine, ExportState } from "../engine/engine.ts";
import { useStore } from "../store/useStore.ts";
import { filePicker, AUDIO_EXPORT_EXTENSION } from "../filesystem/filePicker.ts";
import { message } from "@tauri-apps/plugin-dialog";

const { setExportState, setExportMonitoring } = useStore.getState();

export const audioExport = {
  start: async () => {
    const defaultExportPath = await getDefaultExportPath();
    const filePath = await filePicker.chooseFileToSave(defaultExportPath, [
      AUDIO_EXPORT_EXTENSION,
    ]);
    if (!filePath) return;
    setExportMonitoring(true);
    try {
      await engine.exportAudio(filePath);
    } catch (err) {
      setExportMonitoring(false);
      await message(err as string, { title: "Arctracker", kind: "error" });
    }
  },

  handleStateChange: (exportState: ExportState) => {
    if (exportState.completed) {
      setExportMonitoring(false);
      void engine.exportCleanup();
    }
  },

  poller: () => {
    engine.getExportState().then((state) => setExportState(state));
  },
}

async function getDefaultExportPath() {
  const moduleFileName = useStore.getState().module.fileName;
  return moduleFileName
    ? await engine.defaultExportPath(moduleFileName)
    : undefined;
}
