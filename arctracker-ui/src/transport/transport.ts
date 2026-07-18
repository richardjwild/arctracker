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

  sequenceSeekToStart: () => {
    transport.sequenceSeek(0);
  },

  sequenceSeekToEnd: () => {
    const { sequence } = useStore.getState();
    transport.sequenceSeek(sequence.length - 1);
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
    const numTracks = useStore.getState().module.numTracks;
    const displayedPatternNo = useStore.getState().currentPattern?.patternNo;
    return await engine.getTransportState(displayedPatternNo, numTracks);
  },

  transportStatePoller: (() => {
    transport.getTransportState().then((state) => {
      useStore.getState().setTransportState(state);
      if (state.newPattern !== null)
        useStore.getState().setCurrentPattern({
          patternNo: state.patternNo,
          lines: state.newPattern,
        });
    });
  }) as AppPoller,
};
