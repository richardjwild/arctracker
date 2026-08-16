import Modal from "./Modal.tsx";
import { message } from "../language/messages.ts";
import "./EditAppConfig.css";
import { useStore } from "../store/useStore.ts";
import { audioDevice, AudioDeviceInfo } from "../audioDevice/audioDevice.ts";
import { useEffect, useState } from "react";
import {
  AppConfig,
  appConfig, DraftAppConfig,
  SelectedAudioDevice
} from "../config/appConfig.ts";
import { alerting } from "../alerting/alert.ts";
import { midi, MidiDeviceInfo } from "../midi/midi.ts";
import { editor } from "../editing/editor.ts";

const defaultOutputDeviceIndex = -1;
const defaultConfig: DraftAppConfig = {
  selectedAudioDevice: "default",
  selectedMidiDevice: null,
  defaultAuthorName: null,
  defaultPatternLength: 64,
  defaultTrackCount: 8,
  defaultLinesPerBeat: 4,
  defaultBeatsPerMinute: 120,
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
  const editMode = useStore((state) => state.editorState.editMode);
  const editing = editMode === "appConfig";
  const draftAppConfig = useStore((state) => state).draftAppConfig || defaultConfig;
  const setDraftAppConfig = useStore((state) => state.setDraftAppConfig);
  const [inputPatternLength, setInputPatternLength] = useState("");
  const [inputTrackCount, setInputTrackCount] = useState("");
  const [inputLinesPerBeat, setInputLinesPerBeat] = useState("");
  const [inputBeatsPerMinute, setInputBeatsPerMinute] = useState("");
  const [availableOutputs, setAvailableOutputs] = useState<AudioDeviceInfo[]>(
    [],
  );
  const [availableMidiInputs, setAvailableMidiInputs] = useState<
    MidiDeviceInfo[]
  >([]);

  const syncDraftConfigWithCurrent = async (currentConfig: AppConfig) => {
    const audioOutputs = await audioDevice.getAvailableOutputs();
    const midiInputs = await midi.getAvailableInputs();
    setAvailableOutputs(audioOutputs);
    setAvailableMidiInputs(midiInputs);
    const selectedAudioDevice =
      currentConfig.selectedAudioDevice === "default"
        ? "default"
        : (audioOutputs.find((output) =>
            deviceSelected(output, currentConfig.selectedAudioDevice),
          ) ?? "default");
    const selectedMidiDevice =
      currentConfig.selectedMidiDevice === null
        ? null
        : (midiInputs.find(
            (input) => input.name === currentConfig.selectedMidiDevice?.name,
          ) ?? null);
    setDraftAppConfig({
      selectedAudioDevice,
      selectedMidiDevice,
      defaultAuthorName: currentConfig.defaultAuthorName,
      defaultPatternLength: currentConfig.defaultPatternLength,
      defaultTrackCount: currentConfig.defaultTrackCount,
      defaultLinesPerBeat: currentConfig.defaultLinesPerBeat,
      defaultBeatsPerMinute: currentConfig.defaultBeatsPerMinute,
    });
    setInputPatternLength(currentConfig.defaultPatternLength.toString());
    setInputTrackCount(currentConfig.defaultTrackCount.toString());
    setInputLinesPerBeat(currentConfig.defaultLinesPerBeat.toString());
    setInputBeatsPerMinute(currentConfig.defaultBeatsPerMinute.toString());
  };

  useEffect(() => {
    if (!editing) return;
    const currentConfig = appConfig.get();
    void syncDraftConfigWithCurrent(currentConfig);
  }, [editing]);

  if (!editing) return null;

  const setSelectedAudioDevice = (deviceIndex: number) => {
    if (deviceIndex === defaultOutputDeviceIndex) {
      setDraftAppConfig({
        ...draftAppConfig,
        selectedAudioDevice: "default",
      });
      return;
    }
    const selectedOutput = availableOutputs.find(
      (output) => output.deviceIndex === deviceIndex,
    );
    if (selectedOutput) {
      setDraftAppConfig({
        ...draftAppConfig,
        selectedAudioDevice: selectedOutput,
      });
    }
  };

  const setSelectedMidiDevice = (deviceName: string) => {
    if (deviceName === "")
      setDraftAppConfig({
        ...draftAppConfig,
        selectedMidiDevice: null,
      });
    else
      setDraftAppConfig({
        ...draftAppConfig,
        selectedMidiDevice: { name: deviceName },
      });
  };

  const validateDefaultPatternLength = (): boolean => {
    const patternLength = Number(inputPatternLength);
    if (
      Number.isInteger(patternLength) &&
      patternLength >= 1 &&
      patternLength <= 1000
    ) {
      setDraftAppConfig({
        ...draftAppConfig,
        defaultPatternLength: patternLength,
      })
      return true;
    } else {
      void alerting.showInfo(message("invalidDefaultPatternLength"));
      setInputPatternLength(draftAppConfig.defaultPatternLength.toString());
      return false;
    }
  };

  const validateDefaultTrackCount = (): boolean => {
    const trackCount = Number(inputTrackCount);
    if (
      Number.isInteger(trackCount) &&
      trackCount >= 1 &&
      trackCount <= 256
    ) {
      setDraftAppConfig({
        ...draftAppConfig,
        defaultTrackCount: trackCount,
      })
      return true;
    } else {
      void alerting.showInfo(message("invalidTrackCount"));
      setInputTrackCount(draftAppConfig.defaultTrackCount.toString());
      return false;
    }
  };

  const validateDefaultLinesPerBeat = (): boolean => {
    const linesPerBeat = Number(inputLinesPerBeat);
    if (
      Number.isInteger(linesPerBeat) &&
      linesPerBeat >= 1 &&
      linesPerBeat <= 256
    ) {
      setDraftAppConfig({
        ...draftAppConfig,
        defaultLinesPerBeat: linesPerBeat,
      })
      return true;
    } else {
      void alerting.showInfo(message("invalidLinesPerBeat"));
      setInputLinesPerBeat(draftAppConfig.defaultLinesPerBeat.toString());
      return false;
    }
  };

  const validateDefaultBeatsPerMinute = (): boolean => {
    const beatsPerMinute = Number(inputBeatsPerMinute);
    if (
      Number.isInteger(beatsPerMinute) &&
      beatsPerMinute >= 1 &&
      beatsPerMinute <= 256
    ) {
      setDraftAppConfig({
        ...draftAppConfig,
        defaultBeatsPerMinute: beatsPerMinute,
      })
      return true;
    } else {
      void alerting.showInfo(message("invalidTempo"));
      setInputBeatsPerMinute(draftAppConfig.defaultBeatsPerMinute.toString());
      return false;
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
          value={getDeviceIndex(draftAppConfig.selectedAudioDevice)}
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
      <div className="midiInputDeviceLabel">
        <label htmlFor="midiInputDeviceSelect">
          {message("midiInputDeviceLabel")}
        </label>
      </div>
      <div className="midiInputDeviceEdit uiArea padded rounded">
        <select
          id="midiInputDeviceSelect"
          name="midiInputDevice"
          onChange={(e) => setSelectedMidiDevice(e.target.value)}
          value={draftAppConfig.selectedMidiDevice?.name || ""}
        >
          <option value={""}>{message("noMidiInputDevice")}</option>
          {availableMidiInputs.map((input, i) => (
            <option key={i} value={input.name}>
              {input.name}
            </option>
          ))}
        </select>
      </div>
      <div className="defaultAuthorNameLabel">
        <label htmlFor="defaultAuthorNameInput">
          {message("defaultAuthorNameLabel")}
        </label>
      </div>
      <div className="defaultAuthorNameEdit uiArea padded rounded">
        <input
          type="text"
          id="defaultAuthorNameInput"
          maxLength={65}
          value={draftAppConfig.defaultAuthorName || ""}
          onFocus={editor.startTextInput}
          onBlur={editor.stopTextInput}
          onChange={(e) =>
            setDraftAppConfig({
              ...draftAppConfig,
              defaultAuthorName: e.target.value,
            })
          }
        />
      </div>
      <div className="defaultPatternLengthLabel">
        <label htmlFor="defaultPatternLengthInput">
          {message("defaultPatternLengthLabel")}
        </label>
      </div>
      <div className="defaultPatternLengthEdit uiArea padded rounded">
        <input
          type="text"
          id="defaultPatternLengthInput"
          maxLength={4}
          value={inputPatternLength}
          onChange={(e) => setInputPatternLength(e.target.value)}
          onFocus={editor.startTextInput}
          onBlur={() => {
            validateDefaultPatternLength();
            editor.stopTextInput();
          }}
        />
      </div>
      <div className="defaultTrackCountLabel">
        <label htmlFor="defaultTrackCountInput">
          {message("defaultTrackCountLabel")}
        </label>
      </div>
      <div className="defaultTrackCountEdit uiArea padded rounded">
        <input
          type="text"
          id="defaultTrackCountInput"
          maxLength={3}
          value={inputTrackCount}
          onChange={(e) => setInputTrackCount(e.target.value)}
          onFocus={editor.startTextInput}
          onBlur={() => {
            validateDefaultTrackCount();
            editor.stopTextInput();
          }}
        />
      </div>
      <div className="defaultLinesPerBeatLabel">
        <label htmlFor="defaultLinesPerBeatInput">
          {message("defaultLinesPerBeatLabel")}
        </label>
      </div>
      <div className="defaultLinesPerBeatEdit uiArea padded rounded">
        <input
          type="text"
          id="defaultLinesPerBeatInput"
          maxLength={3}
          value={inputLinesPerBeat}
          onChange={(e) => setInputLinesPerBeat(e.target.value)}
          onFocus={editor.startTextInput}
          onBlur={() => {
            validateDefaultLinesPerBeat();
            editor.stopTextInput();
          }}
        />
      </div>
      <div className="defaultBeatsPerMinuteLabel">
        <label htmlFor="defaultBeatsPerMinuteInput">
          {message("defaultBeatsPerMinuteLabel")}
        </label>
      </div>
      <div className="defaultBeatsPerMinuteEdit uiArea padded rounded">
        <input
          type="text"
          id="defaultBeatsPerMinuteInput"
          maxLength={3}
          value={inputBeatsPerMinute}
          onChange={(e) => setInputBeatsPerMinute(e.target.value)}
          onFocus={editor.startTextInput}
          onBlur={() => {
            validateDefaultBeatsPerMinute();
            editor.stopTextInput();
          }}
        />
      </div>
      <div className="saveCloseButtons uiArea padded rounded">
        <button type="button" onClick={appConfig.update}>
          {message("saveButtonLabel")}
        </button>
        <button type="button" onClick={appConfig.hideDialog}>
          {message("cancelButtonLabel")}
        </button>
      </div>
    </Modal>
  );
}
