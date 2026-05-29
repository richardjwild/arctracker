import { engine, type PatternLine, TransportState } from "../engine/engine.ts";
import { useStore } from "../store/useStore.ts";
import { AppPoller } from "../polling/poller.ts";

export type { TransportState };

export interface CurrentPattern {
  patternNo: number;
  lines: PatternLine[];
}

export const transport = {
  togglePlay: () => {
    if (!useStore.getState().transportState.playing)
      transport.patternSeek(0);
    engine.togglePlay();
  },

  toggleLoop: () => {
    engine.toggleLoop();
  },

  sequenceSeekForwards: () => {
    const moduleInfo = useStore.getState().module;
    const sequencePos = useStore.getState().transportState.sequencePos;
    const newSequencePos = sequencePos + 1;
    if (newSequencePos < moduleInfo.tuneLength) engine.seek(newSequencePos, 0);
  },

  sequenceSeekBackwards: () => {
    const sequencePos = useStore.getState().transportState.sequencePos;
    const newSequencePos = sequencePos - 1;
    if (newSequencePos >= 0) engine.seek(newSequencePos, 0);
  },

  patternSeek: (toPatternIndex: number) => {
    const currentPattern = useStore.getState().currentPattern;
    if (!currentPattern) return;
    if (toPatternIndex < 0 || toPatternIndex >= currentPattern.lines.length) return;
    const { sequencePos } = useStore.getState().transportState;
    engine.seek(sequencePos, toPatternIndex);
  },

  getTransportState: async (): Promise<TransportState> => {
    return await engine.getTransportState();
  },

  transportStatePoller: (() =>
    transport
      .getTransportState()
      .then(useStore.getState().setTransportState)) as AppPoller,
};
