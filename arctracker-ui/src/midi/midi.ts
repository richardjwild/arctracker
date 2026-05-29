import { engine } from "../engine/engine.ts";

export const midi = {
  useSample: (sampleNumber: number) => {
    engine.setSelectedSample(sampleNumber);
  },

  useChannel: (channel: number) => {
    engine.setSelectedChannel(channel);
  },
}
