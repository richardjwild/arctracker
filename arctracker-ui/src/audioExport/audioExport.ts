import { engine } from "../engine/engine.ts";
import { useStore } from "../store/useStore.ts";
import {
  filePicker,
  AUDIO_EXPORT_EXTENSION,
} from "../filesystem/filePicker.ts";
import { message } from "@tauri-apps/plugin-dialog";

export type ExportState = {
  completed: boolean;
  percentComplete: number;
}

const { setExportState, setExportMonitoring } = useStore.getState();

function handleExportError(errorMessage: string) {
  message(`Export audio encountered an error: ${errorMessage}`, {
    title: "Arctracker",
    kind: "error",
  }).then(() => {
    setExportMonitoring(false);
  });
}

export const audioExport = {
  start: async () => {
    const defaultExportPath = await getDefaultExportPath();
    const filePath = await filePicker.chooseFileToSave(
      "Export audio",
      defaultExportPath,
      [AUDIO_EXPORT_EXTENSION],
    );
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
    engine.pollExportEvents().then((events) => {
      for (const event of events) {
        switch (event.eventType) {
          case "playerError":
            handleExportError(event.errorMessage);
            return; // No point trying to handle any other events.
        }
      }
    });
  },
};

async function getDefaultExportPath() {
  const moduleFileName = useStore.getState().module.fileName;
  return moduleFileName
    ? await engine.defaultExportPath(moduleFileName)
    : undefined;
}
