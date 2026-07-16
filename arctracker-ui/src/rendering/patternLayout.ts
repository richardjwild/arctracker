import { useStore } from "../store/useStore.ts";
import { cursor, CursorPosition } from "../editing/cursor.ts";

export type PatternLayout = {
  leftPadding: number;
  glyphWidth: number;
  rowHeight: number;
  trackHeaderHeight: number;
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
      trackHeaderHeight: glyphHeight + 1,
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
    const availableHeight = viewportSize.height - layout.trackHeaderHeight - playheadRowHeight;
    const linesToShow = 1 + Math.floor(availableHeight / layout.rowHeight);
    let displayedWidth =
      layout.leftPadding + layout.rowNumberWidth - layout.glyphWidth;
    displayedWidth += layout.getEventWidth(0);
    let firstVisibleTrack = 0;
    let lastVisibleTrack = 0;
    const cursorTrack = editorState.cursorPosition.track;
    for (let track = 1; track < numTracks; track++) {
      displayedWidth += layout.getEventWidth(track);
      if (displayedWidth > viewportSize.width) {
        if (cursorTrack <= lastVisibleTrack) {
          // Cursor is visible, we have our answer now.
          break;
        }
        // Cursor is not visible, so progressively cut off the leftmost track
        // until everything fits within the viewport again.
        while (displayedWidth > viewportSize.width && firstVisibleTrack <= lastVisibleTrack) {
          displayedWidth -= layout.getEventWidth(firstVisibleTrack);
          firstVisibleTrack++;
        }
      }
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
    let track = currentPosition.track;
    for (
      let candidateTrack = gridViewportFit.firstVisibleTrack;
      candidateTrack <= gridViewportFit.lastVisibleTrack;
      candidateTrack++
    ) {
      x += layout.getEventWidth(candidateTrack);
      if (pointerX <= x) {
        track = candidateTrack;
        break;
      }
    }
    let patternIndex = currentPosition.patternIndex;
    const playheadY = layout.trackHeaderHeight + gridViewportFit.playheadLocationOnScreen * layout.rowHeight;
    const relativeLine = Math.floor((pointerY - playheadY - layout.playheadPadding) / layout.rowHeight);
    if (playheadIndex + relativeLine >= 0 && playheadIndex + relativeLine < patternLength) {
      patternIndex = playheadIndex + relativeLine;
    }
    return {
      ...currentPosition,
      track,
      patternIndex,
    };
  },
};
