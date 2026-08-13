import { engine } from "../engine/engine.ts";

export type MidiDeviceInfo = {
  name: string;
}

export const midi = {
  getAvailableInputs: async (): Promise<MidiDeviceInfo[]> => {
    return await engine.getAvailableMidiDevices();
  },

  useInputDevice: async (device: MidiDeviceInfo) => {
    await engine.useMidiDevice(device.name);
  },

  useInstrument: (instrument: number) => {
    engine.setSelectedInstrument(instrument);
  },

  useChannel: (channel: number) => {
    engine.setSelectedChannel(channel);
  },
}
