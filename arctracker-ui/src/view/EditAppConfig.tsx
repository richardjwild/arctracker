import Modal from "./Modal.tsx";
import { message } from "../language/messages.ts";
import "./EditAppConfig.css";
import { useStore } from "../store/useStore.ts";
import { audioDevice, AudioDeviceInfo } from "../audioDevice/audioDevice.ts";
import { useEffect, useState } from "react";
import { appConfig, SelectedAudioDevice } from "../config/appConfig.ts";
import { alerting } from "../alerting/alert.ts";

type DraftAppConfig = {
  selectedAudioDevice: "default" | AudioDeviceInfo;
};

const defaultOutputDeviceIndex = -1;
const defaultConfig: DraftAppConfig = {
  selectedAudioDevice: "default",
};

function deviceSelected(
  outputDevice: AudioDeviceInfo,
  selectedDevice: SelectedAudioDevice | undefined,
) {
  if (!selectedDevice) return false;
  if (selectedDevice === "default") {
    return outputDevice.deviceIndex === defaultOutputDeviceIndex;
  }
  return (
    selectedDevice.name === outputDevice.name &&
    selectedDevice.hostApiName === outputDevice.hostApiName
  );
}

function getDeviceIndex(device: AudioDeviceInfo | "default") {
  return device === "default" ? defaultOutputDeviceIndex : device.deviceIndex;
}

export default function EditAppConfig() {
  const editing =
    useStore((state) => state.editorState.editMode) === "appConfig";
  const [draftConfig, setDraftConfig] = useState<DraftAppConfig>(defaultConfig);
  const [availableOutputs, setAvailableOutputs] = useState<AudioDeviceInfo[]>(
    [],
  );

  useEffect(() => {
    if (!editing) return;
    const currentConfig = appConfig.get();
    audioDevice.getAvailableOutputs().then((outputs) => {
      setAvailableOutputs(outputs);
      const selectedAudioDevice =
        currentConfig.selectedAudioDevice === "default"
          ? "default"
          : (outputs.find((output) =>
              deviceSelected(output, currentConfig.selectedAudioDevice),
            ) ?? "default");
      setDraftConfig({
        selectedAudioDevice,
      });
    });
  }, [editing]);

  if (!editing) return null;

  const setSelectedAudioDevice = (deviceIndex: number) => {
    if (deviceIndex === defaultOutputDeviceIndex) {
      setDraftConfig({
        ...draftConfig,
        selectedAudioDevice: "default",
      });
      return;
    }
    const selectedOutput = availableOutputs.find(
      (output) => output.deviceIndex === deviceIndex,
    );
    if (selectedOutput) {
      setDraftConfig({
        ...draftConfig,
        selectedAudioDevice: selectedOutput,
      });
    }
  };

  const applyConfig = async () => {
    try {
      await appConfig.apply({
        selectedAudioDevice: draftConfig.selectedAudioDevice,
      });
      appConfig.hideDialog();
    } catch (e) {
      void alerting.showErrorWithContext(
        message("appSettingsFailed"),
        e as string,
      );
    }
  };

  return (
    <Modal className="editAppConfig">
      <div className="outputDeviceLabel">
        <label htmlFor="outputDeviceSelect">
          {message("outputDeviceLabel")}
        </label>
      </div>
      <div className="outputDeviceEdit uiArea padded rounded">
        <select
          id="outputDeviceSelect"
          name="outputDevice"
          onChange={(e) => setSelectedAudioDevice(Number(e.target.value))}
          value={getDeviceIndex(draftConfig.selectedAudioDevice)}
        >
          <option value={defaultOutputDeviceIndex}>
            {message("defaultOutputDevice")}
          </option>
          {availableOutputs.map((output, i) => (
            <option key={i} value={output.deviceIndex}>
              {`${output.name} (${output.hostApiName})`}
            </option>
          ))}
        </select>
      </div>
      <div className="saveCloseButtons uiArea padded rounded">
        <button type="button" onClick={applyConfig}>
          {message("saveButtonLabel")}
        </button>
        <button type="button" onClick={appConfig.hideDialog}>
          {message("cancelButtonLabel")}
        </button>
      </div>
    </Modal>
  );
}
