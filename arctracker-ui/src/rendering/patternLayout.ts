import { useStore } from "../store/useStore.ts";

export type PatternLayout = {
  viewportSize: { width: number; height: number };
  leftPadding: number;
  glyphWidth: number;
  rowHeight: number;
  gutterWidth: number;
  trackHeaderHeight: number;
  trackFooterHeight: number;
  playheadPadding: number;
  rowNumberWidth: number;
  getEventWidth: (track: number) => number;
  maxLines: number;
};

export type GridViewportFit = {
  playheadRowHeight: number;
  linesToShow: number;
  lineOffset: number;
  trackOffset: number;
  firstVisibleTrack: number;
  lastVisibleTrack: number;
  playheadLocationOnScreen: number;
};

export type HorizontalScroll = {
  firstVisibleTrack: number;
  trackOffset: number;
};

export type PointerClickHit =
  | {
      objectType: "patternEvent";
      event: { track: number; patternIndex: number };
    }
  | { objectType: "trackHeader"; track: number }
  | { objectType: "trackFooter"; track: number };

const leftPadding = 10;
const glyphHeight = 20;
const glyphWidth = 10;

export const patternLayout = {
  getPatternLayout: (
    viewportSize: { width: number; height: number },
    effectsDisplayed: number[],
  ): PatternLayout => {
    const getEffectsDisplayed = (track: number) =>
      track >= 0 && track < effectsDisplayed.length
        ? effectsDisplayed[track]
        : 1;
    const rowNumberWidth = glyphWidth * 5;
    return {
      viewportSize,
      leftPadding,
      glyphWidth,
      rowHeight: glyphHeight,
      trackHeaderHeight: glyphHeight + 1,
      trackFooterHeight: glyphHeight + 5,
      playheadPadding: 2,
      rowNumberWidth,
      gutterWidth: leftPadding + rowNumberWidth - glyphWidth,
      getEventWidth: (track: number) =>
        glyphWidth * 8 + getEffectsDisplayed(track) * glyphWidth * 5,
      maxLines: 1000,
    };
  },

  calculateGridViewportFit: (
    viewportSize: { width: number; height: number },
    layout: PatternLayout,
    numTracks: number,
    firstVisibleTrack: number,
    trackOffset: number,
  ): GridViewportFit => {
    //
    // Calculate lines to show (always centred on playhead).
    //
    const playheadRowHeight = layout.rowHeight + 2 * layout.playheadPadding;
    const availableHeight =
      viewportSize.height -
      layout.trackHeaderHeight -
      layout.trackFooterHeight -
      playheadRowHeight;
    const spaceEachSide = availableHeight / 2;
    const linesEachSide = Math.ceil(spaceEachSide / layout.rowHeight);
    const linesToShow = 1 + linesEachSide * 2;
    const lineOffset = (availableHeight - linesEachSide * 2 * layout.rowHeight) / 2;
    //
    // Calculate tracks to show, with the constraint that the cursor must always be visible.
    //
    const cursorTrack = useStore.getState().editorState.cursorPosition.track;
    if (cursorTrack < firstVisibleTrack) {
      firstVisibleTrack = cursorTrack;
    }
    let displayedWidth = layout.gutterWidth + layout.getEventWidth(firstVisibleTrack);
    let lastVisibleTrack = firstVisibleTrack;
    let lastVisibleTrackPartial = false;
    for (let track = firstVisibleTrack + 1; track < numTracks; track++) {
      lastVisibleTrack = track;
      displayedWidth += layout.getEventWidth(track);
      if (displayedWidth >= viewportSize.width) {
        lastVisibleTrackPartial = (displayedWidth > viewportSize.width);
        break;
      }
    }
    //
    // Is the cursor off the right-hand edge or not fully visible?
    //
    if (cursorTrack > lastVisibleTrack || (cursorTrack === lastVisibleTrack && lastVisibleTrackPartial)) {
      // Clamp the cursor track against the right-hand edge and work backwards.
      lastVisibleTrack = cursorTrack;
      displayedWidth = layout.gutterWidth + layout.getEventWidth(lastVisibleTrack);
      for (let track = lastVisibleTrack - 1; track >= 0; track--) {
        firstVisibleTrack = track;
        displayedWidth += layout.getEventWidth(track);
        if (displayedWidth >= viewportSize.width) {
          break;
        }
      }
    }
    //
    // Do we have room for any more tracks to the left?
    //
    if (firstVisibleTrack > 0 && displayedWidth < viewportSize.width)
    {
      for (let track = firstVisibleTrack - 1; track >= 0; track--) {
        firstVisibleTrack = track;
        displayedWidth += layout.getEventWidth(firstVisibleTrack);
        if (displayedWidth > viewportSize.width) {
          break;
        }
      }
    }
    if (cursorTrack === firstVisibleTrack) {
      trackOffset = 0;
    } else {
      trackOffset = (displayedWidth > viewportSize.width) ? viewportSize.width - displayedWidth : 0;
    }
    return {
      playheadRowHeight,
      linesToShow,
      lineOffset,
      trackOffset: trackOffset,
      firstVisibleTrack,
      lastVisibleTrack,
      playheadLocationOnScreen: Math.floor(linesToShow / 2),
    };
  },

  pointerClickedOn: (
    pointerX: number,
    pointerY: number,
    viewportSize: { width: number; height: number },
    horizontalScroll: HorizontalScroll,
    playheadIndex: number,
    numTracks: number,
    effectsDisplayed: number[],
    patternLength: number,
  ): PointerClickHit | null => {
    const layout = patternLayout.getPatternLayout(
      viewportSize,
      effectsDisplayed,
    );
    const gridViewportFit = patternLayout.calculateGridViewportFit(
      viewportSize,
      layout,
      numTracks,
      horizontalScroll.firstVisibleTrack,
      horizontalScroll.trackOffset,
    );
    let x = layout.leftPadding + layout.rowNumberWidth - layout.glyphWidth;
    if (pointerX <= x) return null;
    let track = null;
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
    if (track === null) return null;
    if (pointerY <= layout.trackHeaderHeight) {
      return {
        objectType: "trackHeader",
        track,
      };
    }
    if (pointerY >= viewportSize.height - layout.trackFooterHeight) {
      return {
        objectType: "trackFooter",
        track,
      };
    }
    let patternIndex = null;
    const playheadY =
      layout.trackHeaderHeight +
      gridViewportFit.playheadLocationOnScreen * layout.rowHeight;
    const relativeLine = Math.floor(
      (pointerY - playheadY - layout.playheadPadding) / layout.rowHeight,
    );
    if (
      playheadIndex + relativeLine >= 0 &&
      playheadIndex + relativeLine < patternLength
    ) {
      patternIndex = playheadIndex + relativeLine;
    }
    if (patternIndex === null) return null;
    return {
      objectType: "patternEvent",
      event: {
        track,
        patternIndex,
      },
    };
  },
};
