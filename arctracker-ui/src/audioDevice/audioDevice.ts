import { engine } from "../engine/engine.ts";
import { SelectedSampleRate } from "../config/appConfig.ts";

export type AudioDeviceInfo = {
  deviceIndex: number;
  name: string;
  hostApiName: string;
}

export const audioDevice = {
  getAvailableOutputs: async (): Promise<AudioDeviceInfo[]> => {
    return await engine.getAvailableOutputs();
  },

  useDefaultOutputDevice: async (selectedSampleRate: SelectedSampleRate) => {
    const sampleRate = selectedSampleRate === "auto" ? null : selectedSampleRate;
    await engine.useDefaultOutput(sampleRate);
  },

  useOutputDevice: async (device: AudioDeviceInfo, selectedSampleRate: SelectedSampleRate) => {
    const sampleRate = selectedSampleRate === "auto" ? null : selectedSampleRate;
    await engine.useOutput(device.deviceIndex, device.name, device.hostApiName, sampleRate);
  },
};