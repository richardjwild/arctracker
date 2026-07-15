import { useStore } from "../store/useStore.ts";
import { cursor, CursorPosition } from "../editing/cursor.ts";

export type PatternLayout = {
  leftPadding: number;
  glyphWidth: number;
  rowHeight: number;
  playheadPadding: number;
  rowNumberWidth: number;
  getEventWidth: (track: number) => number;
  maxLines: number;
};

export type GridViewportFit = {
  playheadRowHeight: number;
  linesToShow: number;
  firstVisibleTrack: number;
  lastVisibleTrack: number;
  playheadLocationOnScreen: number;
};

const leftPadding = 10;
const glyphHeight = 20;
const glyphWidth = 10;

export const patternLayout = {
  getPatternLayout: (): PatternLayout => {
    const effectsDisplayed = useStore.getState().editorState.effectsDisplayed;
    return {
      leftPadding,
      glyphWidth,
      rowHeight: glyphHeight,
      playheadPadding: 2,
      rowNumberWidth: glyphWidth * 5,
      getEventWidth: (track: number) =>
        glyphWidth * 8 + effectsDisplayed[track] * glyphWidth * 4,
      maxLines: 1000,
    };
  },

  calculateGridViewportFit: (
    viewportSize: { width: number; height: number },
    numTracks: number,
  ): GridViewportFit => {
    const layout = patternLayout.getPatternLayout();
    const editorState = useStore.getState().editorState;
    const playheadRowHeight = layout.rowHeight + 2 * layout.playheadPadding;
    const linesToShow =
      1 +
      Math.floor((viewportSize.height - playheadRowHeight) / layout.rowHeight);
    let displayedWidth =
      layout.leftPadding + layout.rowNumberWidth - layout.glyphWidth;
    displayedWidth += layout.getEventWidth(0);
    let firstVisibleTrack = 0;
    let lastVisibleTrack = 0;
    const cursorTrack = editorState.cursorPosition.track;
    for (let track = 1; track < numTracks; track++) {
      const trackWidth = layout.getEventWidth(track);
      if (displayedWidth + trackWidth > viewportSize.width) {
        if (cursorTrack <= lastVisibleTrack) break;
        else firstVisibleTrack++;
      }
      displayedWidth += trackWidth;
      lastVisibleTrack++;
    }
    return {
      playheadRowHeight,
      linesToShow,
      firstVisibleTrack,
      lastVisibleTrack,
      playheadLocationOnScreen: Math.floor(linesToShow / 2),
    };
  },

  cursorPositionAt: (
    pointerX: number,
    pointerY: number,
    viewportSize: { width: number; height: number },
    playheadIndex: number,
    numTracks: number,
    patternLength: number,
  ): CursorPosition => {
    const currentPosition = cursor.currentPosition();
    const layout = patternLayout.getPatternLayout();
    const gridViewportFit = patternLayout.calculateGridViewportFit(
      viewportSize,
      numTracks,
    );
    let x = layout.leftPadding + layout.rowNumberWidth - layout.glyphWidth;
    if (pointerX <= x) return currentPosition;
    let trackIndex = currentPosition.track;
    for (
      let track = gridViewportFit.firstVisibleTrack;
      track <= gridViewportFit.lastVisibleTrack;
      track++
    ) {
      x += layout.getEventWidth(track);
      if (pointerX <= x) {
        trackIndex = track;
        break;
      }
    }
    let patternIndex = currentPosition.patternIndex;
    const playheadY = gridViewportFit.playheadLocationOnScreen * layout.rowHeight;
    const relativeLine = Math.floor((pointerY - playheadY - layout.playheadPadding) / layout.rowHeight);
    if (playheadIndex + relativeLine >= 0 && playheadIndex + relativeLine < patternLength) {
      patternIndex = playheadIndex + relativeLine;
    }
    return {
      ...currentPosition,
      track: trackIndex,
      patternIndex,
    };
  },
};
