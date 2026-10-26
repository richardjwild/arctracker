import Modal from "./Modal.tsx";
import { message } from "../language/messages.ts";
import "./EditAppConfig.css";
import { useStore } from "../store/useStore.ts";
import { audioDevice, AudioDeviceInfo } from "../audioDevice/audioDevice.ts";
import { useEffect, useRef, useState } from "react";
import {
  AppConfig,
  appConfig, DraftAppConfig,
  SelectedAudioDevice
} from "../config/appConfig.ts";
import { alerting } from "../alerting/alert.ts";
import { midi, MidiDeviceInfo } from "../midi/midi.ts";
import FieldLabel from "./components/FieldLabel.tsx";
import { primaryModifier } from "../keyboard/keyBinding.ts";
import { KeyHandler } from "../keyboard/keyHandler.ts";
import { useKeyboardHandler } from "../hooks/useKeyboardHandler.ts";

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
  const outputDeviceSelectRef = useRef<HTMLSelectElement>(null);
  const midiInputDeviceSelectRef = useRef<HTMLSelectElement>(null);
  const defaultAuthorNameInputRef = useRef<HTMLInputElement>(null);
  const defaultPatternLengthInputRef = useRef<HTMLInputElement>(null);
  const defaultTrackCountInputRef = useRef<HTMLInputElement>(null);
  const defaultLinesPerBeatInputRef = useRef<HTMLInputElement>(null);
  const defaultBeatsPerMinuteInputRef = useRef<HTMLInputElement>(null);

  const keyHandler: KeyHandler = (e) => {
    if (e.code === "Escape") {
      appConfig.hideDialog();
      return true;
    } else if (e.code === "KeyS" && primaryModifier(e)) {
      void appConfig.update();
      return true;
    } else if (e.code === "Digit1" && primaryModifier(e)) {
      outputDeviceSelectRef.current?.focus();
      return true;
    } else if (e.code === "Digit2" && primaryModifier(e)) {
      midiInputDeviceSelectRef.current?.focus();
      return true;
    } else if (e.code === "Digit3" && primaryModifier(e)) {
      defaultAuthorNameInputRef.current?.focus();
      return true;
    } else if (e.code === "Digit4" && primaryModifier(e)) {
      defaultPatternLengthInputRef.current?.focus();
      return true;
    } else if (e.code === "Digit5" && primaryModifier(e)) {
      defaultTrackCountInputRef.current?.focus();
      return true;
    } else if (e.code === "Digit6" && primaryModifier(e)) {
      defaultLinesPerBeatInputRef.current?.focus();
      return true;
    } else if (e.code === "Digit7" && primaryModifier(e)) {
      defaultBeatsPerMinuteInputRef.current?.focus();
      return true;
    }
    return false;
  };

  useKeyboardHandler(keyHandler, editing);

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
      <FieldLabel
        className="outputDeviceLabel"
        associatedField="outputDeviceSelect"
        hotkey={1}
        text={message("outputDeviceLabel")}
      />
      <div className="outputDeviceEdit uiArea padded rounded">
        <select
          id="outputDeviceSelect"
          name="outputDevice"
          onChange={(e) => setSelectedAudioDevice(Number(e.target.value))}
          value={getDeviceIndex(draftAppConfig.selectedAudioDevice)}
          ref={outputDeviceSelectRef}
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
      <FieldLabel
        className="midiInputDeviceLabel"
        associatedField="midiInputDeviceSelect"
        hotkey={2}
        text={message("midiInputDeviceLabel")}
        />
      <div className="midiInputDeviceEdit uiArea padded rounded">
        <select
          id="midiInputDeviceSelect"
          name="midiInputDevice"
          onChange={(e) => setSelectedMidiDevice(e.target.value)}
          value={draftAppConfig.selectedMidiDevice?.name || ""}
          ref={midiInputDeviceSelectRef}
        >
          <option value={""}>{message("noMidiInputDevice")}</option>
          {availableMidiInputs.map((input, i) => (
            <option key={i} value={input.name}>
              {input.name}
            </option>
          ))}
        </select>
      </div>
      <FieldLabel
        className="defaultAuthorNameLabel"
        associatedField="defaultAuthorNameInput"
        hotkey={3}
        text={message("defaultAuthorNameLabel")}
      />
      <div className="defaultAuthorNameEdit uiArea padded rounded">
        <input
          type="text"
          id="defaultAuthorNameInput"
          maxLength={65}
          value={draftAppConfig.defaultAuthorName || ""}
          onChange={(e) =>
            setDraftAppConfig({
              ...draftAppConfig,
              defaultAuthorName: e.target.value,
            })
          }
          ref={defaultAuthorNameInputRef}
        />
      </div>
      <FieldLabel
        className="defaultPatternLengthLabel"
        associatedField="defaultPatternLengthInput"
        hotkey={4}
        text={message("defaultPatternLengthLabel")}
      />
      <div className="defaultPatternLengthEdit uiArea padded rounded">
        <input
          type="text"
          id="defaultPatternLengthInput"
          maxLength={4}
          value={inputPatternLength}
          onChange={(e) => setInputPatternLength(e.target.value)}
          onBlur={validateDefaultPatternLength}
          ref={defaultPatternLengthInputRef}
        />
      </div>
      <FieldLabel
        className="defaultTrackCountLabel"
        associatedField="defaultTrackCountInput"
        hotkey={5}
        text={message("defaultTrackCountLabel")}
      />
      <div className="defaultTrackCountEdit uiArea padded rounded">
        <input
          type="text"
          id="defaultTrackCountInput"
          maxLength={3}
          value={inputTrackCount}
          onChange={(e) => setInputTrackCount(e.target.value)}
          onBlur={validateDefaultTrackCount}
          ref={defaultTrackCountInputRef}
        />
      </div>
      <FieldLabel
        className="defaultLinesPerBeatLabel"
        associatedField="defaultLinesPerBeatInput"
        hotkey={6}
        text={message("defaultLinesPerBeatLabel")}
      />
      <div className="defaultLinesPerBeatEdit uiArea padded rounded">
        <input
          type="text"
          id="defaultLinesPerBeatInput"
          maxLength={3}
          value={inputLinesPerBeat}
          onChange={(e) => setInputLinesPerBeat(e.target.value)}
          onBlur={validateDefaultLinesPerBeat}
          ref={defaultLinesPerBeatInputRef}
        />
      </div>
      <FieldLabel
        className="defaultBeatsPerMinuteLabel"
        associatedField="defaultBeatsPerMinuteInput"
        hotkey={7}
        text={message("defaultBeatsPerMinuteLabel")}
      />
      <div className="defaultBeatsPerMinuteEdit uiArea padded rounded">
        <input
          type="text"
          id="defaultBeatsPerMinuteInput"
          maxLength={3}
          value={inputBeatsPerMinute}
          onChange={(e) => setInputBeatsPerMinute(e.target.value)}
          onBlur={validateDefaultBeatsPerMinute}
          ref={defaultBeatsPerMinuteInputRef}
        />
      </div>
      <div className="saveCloseButtons">
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
