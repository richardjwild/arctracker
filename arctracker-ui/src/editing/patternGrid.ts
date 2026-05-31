import { Cursor, cursor } from "./cursor.ts";
import { useStore } from "../store/useStore.ts";

export type PatternGridPosition = {
  track: number;
  patternIndex: number;
};

export const patternGrid = {
  moveLeft: (): PatternGridPosition => {
    const newPosition = cursor.moveTrackLeft();
    return {
      track: newPosition.track,
      patternIndex: cursor.currentPosition().patternIndex,
    };
  },

  moveRight: (): PatternGridPosition => {
    const newPosition = cursor.moveTrackRight();
    return {
      track: newPosition.track,
      patternIndex: cursor.currentPosition().patternIndex,
    };
  },

  moveUp: (): PatternGridPosition => {
    const patternIndex = cursor.currentPosition().patternIndex;
    const newPatternIndex = patternIndex > 0 ? patternIndex - 1 : patternIndex;
    cursor.updatePatternIndex(newPatternIndex);
    return {
      track: new Cursor().currentPosition().track,
      patternIndex: newPatternIndex,
    };
  },

  moveDown: (wrap: boolean = false): PatternGridPosition => {
    const { currentPattern } = useStore.getState();
    const patternIndex = cursor.currentPosition().patternIndex;
    const lastPatternIndex = currentPattern.lines.length - 1;
    let newPatternIndex;
    if (patternIndex < lastPatternIndex) newPatternIndex = patternIndex + 1;
    else newPatternIndex = wrap ? 0 : lastPatternIndex;
    cursor.updatePatternIndex(newPatternIndex);
    return {
      track: new Cursor().currentPosition().track,
      patternIndex: newPatternIndex,
    };
  },

  moveTo: (position: PatternGridPosition): PatternGridPosition => {
    cursor.moveToTrack(position.track);
    cursor.updatePatternIndex(position.patternIndex);
    return {
      track: position.track,
      patternIndex: position.patternIndex,
    };
  },

  strideUp: (): PatternGridPosition => {
    const { patternGridStrideLength } = useStore.getState();
    const patternIndex = cursor.currentPosition().patternIndex;
    let newPatternIndex = patternIndex - patternGridStrideLength;
    if (newPatternIndex < 0) newPatternIndex = 0;
    cursor.updatePatternIndex(newPatternIndex);
    return {
      track: new Cursor().currentPosition().track,
      patternIndex: newPatternIndex,
    };
  },

  strideDown: (): PatternGridPosition => {
    const { currentPattern, patternGridStrideLength } = useStore.getState();
    const patternIndex = cursor.currentPosition().patternIndex;
    let newPatternIndex = patternIndex + patternGridStrideLength;
    if (newPatternIndex >= currentPattern.lines.length)
      newPatternIndex = currentPattern.lines.length - 1;
    cursor.updatePatternIndex(newPatternIndex);
    return {
      track: new Cursor().currentPosition().track,
      patternIndex: newPatternIndex,
    };
  },

  jumpToTop: () => {
    cursor.updatePatternIndex(0);
    return {
      track: new Cursor().currentPosition().track,
      patternIndex: 0,
    };
  },

  jumpToBottom: (): PatternGridPosition => {
    const currentPattern = useStore.getState().currentPattern;
    cursor.updatePatternIndex(currentPattern.lines.length - 1);
    return {
      track: new Cursor().currentPosition().track,
      patternIndex: currentPattern.lines.length - 1,
    };
  },

  currentPosition: (): PatternGridPosition => {
    return cursor.currentPosition();
  },
};
