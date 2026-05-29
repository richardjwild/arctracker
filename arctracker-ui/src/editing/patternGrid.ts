import { transport } from "../transport/transport.ts";
import { cursor } from "./cursor.ts";
import { useStore } from "../store/useStore.ts";

export const patternGrid = {
  moveLeft: () => {
    cursor.moveEventLeft();
  },

  moveRight: () => {
    cursor.moveEventRight();
  },

  moveUp: () => {
    const {
      currentPattern,
      transportState: { patternIndex },
    } = useStore.getState();
    if (!currentPattern) return;
    const newPatternIndex = patternIndex - 1;
    if (newPatternIndex >= 0) transport.patternSeek(newPatternIndex);
  },

  moveDown: (wrap: boolean = false) => {
    const {
      currentPattern,
      transportState: { patternIndex },
    } = useStore.getState();
    if (!currentPattern) return;
    const newPatternIndex = patternIndex + 1;
    if (newPatternIndex < currentPattern.lines.length)
      transport.patternSeek(newPatternIndex);
    else if (wrap) transport.patternSeek(0);
  },

  strideUp: () => {
    const {
      currentPattern,
      patternGridStrideLength,
      transportState: { patternIndex },
    } = useStore.getState();
    if (!currentPattern) return;
    let newPatternIndex = patternIndex - patternGridStrideLength;
    if (newPatternIndex < 0) newPatternIndex = 0;
    transport.patternSeek(newPatternIndex);
  },

  strideDown: () => {
    const {
      currentPattern,
      patternGridStrideLength,
      transportState: { patternIndex },
    } = useStore.getState();
    if (!currentPattern) return;
    let newPatternIndex = patternIndex + patternGridStrideLength;
    if (newPatternIndex >= currentPattern.lines.length)
      newPatternIndex = currentPattern.lines.length - 1;
    transport.patternSeek(newPatternIndex);
  },

  jumpToTop: () => {
    transport.patternSeek(0);
  },

  jumpToBottom: () => {
    const currentPattern = useStore.getState().currentPattern;
    if (!currentPattern || currentPattern.lines.length === 0) return;
    transport.patternSeek(currentPattern.lines.length - 1);
  },
};
