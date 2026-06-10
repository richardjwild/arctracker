import { engine, type PatternLine, TransportState } from "../engine/engine.ts";
import { useStore } from "../store/useStore.ts";
import { AppPoller } from "../polling/poller.ts";
import { sequence } from "../editing/sequence.ts";

export type { TransportState };

export type CurrentPattern = {
  patternNo: number;
  lines: PatternLine[];
}

export const transport = {
  playing: () => {
    return useStore.getState().transportState.playing;
  },

  togglePlay: () => {
    if (!transport.playing())
      transport.sequenceSeek(sequence.currentPosition());
    engine.togglePlay();
  },

  toggleLoop: () => {
    engine.toggleLoop();
  },

  sequenceSeek: (toSequencePos: number) => {
    const sequence = useStore.getState().sequence;
    if (toSequencePos >= 0 && toSequencePos < sequence.length)
      engine.seek(toSequencePos, 0);
  },

  sequenceSeekForwards: () => {
    const sequencePos = useStore.getState().transportState.sequencePos;
    transport.sequenceSeek(sequencePos + 1);
  },

  sequenceSeekBackwards: () => {
    const sequencePos = useStore.getState().transportState.sequencePos;
    transport.sequenceSeek(sequencePos - 1);
  },

  getTransportState: async (): Promise<TransportState> => {
    return await engine.getTransportState();
  },

  transportStatePoller: (() =>
    transport
      .getTransportState()
      .then(useStore.getState().setTransportState)) as AppPoller,
};
