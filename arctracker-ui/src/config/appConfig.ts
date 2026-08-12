import { BaseDirectory } from "@tauri-apps/api/path";
import { exists, readTextFile, writeTextFile } from "@tauri-apps/plugin-fs";
import { audioDevice } from "../audioDevice/audioDevice.ts";
import { editor } from "../editing/editor.ts";

export type SelectedAudioDevice = "default" | { name: string; hostApiName: string };

export type AppConfig = {
  selectedAudioDevice: SelectedAudioDevice;
};

const configFileName = "app-config.json";
const defaultConfig: AppConfig = {
  selectedAudioDevice: "default",
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
    return;
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
}

export const appConfig = {
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
      await applyAudioDeviceConfig(newConfig.selectedAudioDevice);
    }
    config = newConfig;
    console.log("config is now", config);
  },

  get: (): AppConfig => {
    console.log('returning config', config || defaultConfig);
    return config || defaultConfig;
  },
};
