import { BaseDirectory } from "@tauri-apps/api/path";
import { exists, readTextFile, writeTextFile } from "@tauri-apps/plugin-fs";
import { audioDevice, AudioDeviceInfo } from "../audioDevice/audioDevice.ts";
import { editor } from "../editing/editor.ts";
import { midi, MidiDeviceInfo } from "../midi/midi.ts";
import { useStore } from "../store/useStore.ts";
import { alerting } from "../alerting/alert.ts";
import { message } from "../language/messages.ts";

export type SelectedAudioDevice = "default" | { name: string; hostApiName: string };

export type AppConfig = {
  selectedAudioDevice: SelectedAudioDevice;
  selectedMidiDevice: MidiDeviceInfo | null;
  defaultAuthorName: string | null;
  defaultPatternLength: number;
  defaultTrackCount: number;
  defaultLinesPerBeat: number;
  defaultBeatsPerMinute: number;
};

export type DraftAppConfig = {
  selectedAudioDevice: "default" | AudioDeviceInfo;
  selectedMidiDevice: MidiDeviceInfo | null;
  defaultAuthorName: string | null;
  defaultPatternLength: number;
  defaultTrackCount: number;
  defaultLinesPerBeat: number;
  defaultBeatsPerMinute: number;
};

const configFileName = "app-config.json";
const defaultConfig: AppConfig = {
  selectedAudioDevice: "default",
  selectedMidiDevice: null,
  defaultAuthorName: null,
  defaultPatternLength: 64,
  defaultTrackCount: 8,
  defaultLinesPerBeat: 4,
  defaultBeatsPerMinute: 120,
};

let config: AppConfig | null = null;

function audioDeviceChanged(newDevice: SelectedAudioDevice) {
  if (config === null) return true;
  if (config.selectedAudioDevice === "default") return newDevice !== "default";
  // Now we know config.selectedAudioDevice is not default.
  if (newDevice === "default") return true;
  // Now we know neither of them is default.
  return (
    config.selectedAudioDevice.name !== newDevice.name ||
    config.selectedAudioDevice.hostApiName !== newDevice.hostApiName
  );
}

async function applyAudioDeviceConfig(
  selectedAudioDevice: SelectedAudioDevice,
) {
  if (selectedAudioDevice === "default") {
    await audioDevice.useDefaultOutputDevice();
    return true;
  }
  let foundMatchingDevice = false;
  const availableOutputs = await audioDevice.getAvailableOutputs();
  for (const output of availableOutputs) {
    if (
      output.hostApiName === selectedAudioDevice.hostApiName &&
      output.name === selectedAudioDevice.name
    ) {
      await audioDevice.useOutputDevice(output);
      foundMatchingDevice = true;
      break;
    }
  }
  if (!foundMatchingDevice) {
    await audioDevice.useDefaultOutputDevice();
  }
  return foundMatchingDevice;
}

function midiDeviceChanged(newDevice: MidiDeviceInfo | null) {
  if (config === null) return true;
  if (config.selectedMidiDevice === null) return newDevice !== null;
  if (newDevice === null) return true;
  return config.selectedMidiDevice.name !== newDevice.name;
}

async function applyMidiDeviceConfig(selectedMidiDevice: MidiDeviceInfo | null) {
  if (selectedMidiDevice === null) return true;
  const availableInputs = await midi.getAvailableInputs();
  for (const input of availableInputs) {
    if (input.name === selectedMidiDevice.name) {
      await midi.useInputDevice(input);
      return true;
    }
  }
  // None of the available inputs matched that selected, or none are available.
  return false;
}

export const appConfig = {
  editing: () => useStore.getState().editorState.editMode === "appConfig",

  showDialog: () => editor.setEditMode("appConfig"),

  hideDialog: () => editor.setEditMode("none"),

  load: async (): Promise<AppConfig> => {
    const configExists = await exists(configFileName, {
      baseDir: BaseDirectory.AppConfig,
    });
    if (!configExists) return defaultConfig;
    const configText = await readTextFile(configFileName, {
      baseDir: BaseDirectory.AppConfig,
    });
    return JSON.parse(configText);
  },

  save: async () => {
    await writeTextFile(configFileName, JSON.stringify(config, null, 2), {
      baseDir: BaseDirectory.AppConfig,
    });
  },

  apply: async (newConfig: AppConfig) => {
    if (audioDeviceChanged(newConfig.selectedAudioDevice)) {
      const ok = await applyAudioDeviceConfig(newConfig.selectedAudioDevice);
      if (!ok) newConfig.selectedAudioDevice = "default";
    }
    if (midiDeviceChanged(newConfig.selectedMidiDevice)) {
      const ok = await applyMidiDeviceConfig(newConfig.selectedMidiDevice);
      if (!ok) newConfig.selectedMidiDevice = null;
    }
    config = newConfig;
  },

  get: (): AppConfig => {
    return config || defaultConfig;
  },

  update: async () => {
    const draftAppConfig = useStore.getState().draftAppConfig;
    if (!draftAppConfig) return;
    try {
      await appConfig.apply({
        selectedAudioDevice: draftAppConfig.selectedAudioDevice,
        selectedMidiDevice: draftAppConfig.selectedMidiDevice,
        defaultAuthorName: draftAppConfig.defaultAuthorName,
        defaultPatternLength: draftAppConfig.defaultPatternLength,
        defaultTrackCount: draftAppConfig.defaultTrackCount,
        defaultLinesPerBeat: draftAppConfig.defaultLinesPerBeat,
        defaultBeatsPerMinute: draftAppConfig.defaultBeatsPerMinute,
      });
      appConfig.hideDialog();
    } catch (e) {
      void alerting.showErrorWithContext(
        message("appSettingsFailed"),
        e as string,
      );
    }
  },

  setToDefault: () => {
    void appConfig.apply(defaultConfig);
  },
};
