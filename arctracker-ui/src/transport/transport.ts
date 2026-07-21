import { engine } from "../engine/engine.ts";
import { useStore } from "../store/useStore.ts";
import { sequence } from "../editing/sequence.ts";
import { PatternLine } from "../editing/patternEvents.ts";

export type TransportState = {
  playing: boolean;
  looping: boolean;
  sequencePos: number;
  patternIndex: number;
}

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
};
