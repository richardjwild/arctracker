import { engine } from "../engine/engine.ts";

export const midi = {
  useInstrument: (instrument: number) => {
    engine.setSelectedInstrument(instrument);
  },

  useChannel: (channel: number) => {
    engine.setSelectedChannel(channel);
  },
}
