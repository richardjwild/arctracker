import { engine } from "../engine/engine.ts";
import { useStore } from "../store/useStore.ts";
import {
  filePicker,
  AUDIO_EXPORT_EXTENSION,
} from "../filesystem/filePicker.ts";
import { alerting } from "../alerting/alert.ts";
import { message } from "../language/messages.ts";

export type ExportState = {
  completed: boolean;
  percentComplete: number;
}

const { setExportState, setExportMonitoring } = useStore.getState();

function handleExportError(errorMessage: string) {
  alerting.showErrorWithContext(message("exportAudioFailed"), errorMessage).then(() => {
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
      message("audioFileFilterDescription")
    );
    if (!filePath) return;
    setExportMonitoring(true);
    try {
      await engine.exportAudio(filePath);
    } catch (err) {
      setExportMonitoring(false);
      await alerting.showError(err as string);
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
