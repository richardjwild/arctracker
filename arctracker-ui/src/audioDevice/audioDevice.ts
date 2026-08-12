import { engine } from "../engine/engine.ts";

export type AudioDeviceInfo = {
  deviceIndex: number;
  name: string;
  hostApiName: string;
}

export const audioDevice = {
  getAvailableOutputs: async (): Promise<AudioDeviceInfo[]> => {
    return await engine.getAvailableOutputs();
  },

  useDefaultOutputDevice: async () => {
    await engine.useDefaultOutput();
  },

  useOutputDevice: async (device: AudioDeviceInfo) => {
    await engine.useOutput(device.deviceIndex, device.name, device.hostApiName);
  },
};