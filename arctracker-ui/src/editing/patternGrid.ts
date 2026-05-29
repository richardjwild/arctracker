import { transport } from "../transport/transport.ts";
import { Cursor, cursor } from "./cursor.ts";
import { useStore } from "../store/useStore.ts";

export type PatternGridPosition = {
  track: number;
  patternIndex: number;
};

export const patternGrid = {
  moveLeft: (): PatternGridPosition => {
    const newPosition = cursor.moveEventLeft();
    return {
      track: newPosition.track,
      patternIndex: useStore.getState().transportState.patternIndex,
    };
  },

  moveRight: (): PatternGridPosition => {
    const newPosition = cursor.moveEventRight();
    return {
      track: newPosition.track,
      patternIndex: useStore.getState().transportState.patternIndex,
    };
  },

  moveUp: (): PatternGridPosition => {
    const {
      transportState: { patternIndex },
    } = useStore.getState();
    const newPatternIndex = patternIndex > 0 ? patternIndex - 1 : patternIndex;
    transport.patternSeek(newPatternIndex);
    return {
      track: new Cursor().currentPosition().track,
      patternIndex: newPatternIndex,
    };
  },

  moveDown: (wrap: boolean = false): PatternGridPosition => {
    const {
      currentPattern,
      transportState: { patternIndex },
    } = useStore.getState();
    const lastPatternIndex = currentPattern.lines.length - 1;
    let newPatternIndex;
    if (patternIndex < lastPatternIndex) newPatternIndex = patternIndex + 1;
    else newPatternIndex = wrap ? 0 : lastPatternIndex;
    transport.patternSeek(newPatternIndex);
    return {
      track: new Cursor().currentPosition().track,
      patternIndex: newPatternIndex,
    };
  },

  strideUp: (): PatternGridPosition => {
    const {
      patternGridStrideLength,
      transportState: { patternIndex },
    } = useStore.getState();
    let newPatternIndex = patternIndex - patternGridStrideLength;
    if (newPatternIndex < 0) newPatternIndex = 0;
    transport.patternSeek(newPatternIndex);
    return {
      track: new Cursor().currentPosition().track,
      patternIndex: newPatternIndex,
    };
  },

  strideDown: (): PatternGridPosition => {
    const {
      currentPattern,
      patternGridStrideLength,
      transportState: { patternIndex },
    } = useStore.getState();
    let newPatternIndex = patternIndex + patternGridStrideLength;
    if (newPatternIndex >= currentPattern.lines.length)
      newPatternIndex = currentPattern.lines.length - 1;
    transport.patternSeek(newPatternIndex);
    return {
      track: new Cursor().currentPosition().track,
      patternIndex: newPatternIndex,
    };
  },

  jumpToTop: () => {
    transport.patternSeek(0);
    return {
      track: new Cursor().currentPosition().track,
      patternIndex: 0,
    };
  },

  jumpToBottom: (): PatternGridPosition => {
    const currentPattern = useStore.getState().currentPattern;
    transport.patternSeek(currentPattern.lines.length - 1);
    return {
      track: new Cursor().currentPosition().track,
      patternIndex: currentPattern.lines.length - 1,
    };
  },

  currentPosition: (): PatternGridPosition => {
    return {
      track: useStore.getState().editorState.cursorPosition.track,
      patternIndex: useStore.getState().transportState.patternIndex,
    };
  },
};
