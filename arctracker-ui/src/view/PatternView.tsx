import "./PatternView.css";
import { useStore } from "../store/useStore.ts";
import React, { useEffect, useRef, useState } from "react";
import { engine } from "../engine/engine.ts";
import { animation } from "../rendering/animation.ts";
import {
  Colours,
  getPatternContentDimensions,
  PatternRenderer,
} from "../rendering/renderPattern.ts";
import useSyncCursorWithTransport from "../hooks/useSyncCursorWithTransport.ts";
import { patternGrid } from "../editing/patternGrid.ts";
import useSequencePosition from "../hooks/useSequencePosition.ts";
import { commands } from "../control/commands.ts";
import { patternLayout } from "../rendering/patternLayout.ts";
import { patternEvents } from "../editing/patternEvents.ts";

const wheelScrollThreshold = 40;

function cssProperty(name: string): string {
  return getComputedStyle(document.documentElement)
    .getPropertyValue(name)
    .trim();
}

function normalisedWheelDelta(event: React.WheelEvent): number {
  switch (event.deltaMode) {
    case WheelEvent.DOM_DELTA_LINE:
      return event.deltaY * 16;
    case WheelEvent.DOM_DELTA_PAGE:
      return event.deltaY * window.innerHeight;
    case WheelEvent.DOM_DELTA_PIXEL:
    default:
      return event.deltaY;
  }
}

function resizeCanvas(
  canvas: HTMLCanvasElement,
  logicalWidth: number,
  logicalHeight: number,
) {
  const dpr = window.devicePixelRatio || 1;
  canvas.width = logicalWidth * dpr;
  canvas.height = logicalHeight * dpr;
  canvas.style.width = `${logicalWidth}px`;
  canvas.style.height = `${logicalHeight}px`;
  const ctx = canvas.getContext("2d");
  ctx?.setTransform(dpr, 0, 0, dpr, 0, 0);
}

function getPatternIndex(): number {
  return useStore.getState().transportState.playing
    ? useStore.getState().transportState.patternIndex
    : patternGrid.currentPosition().patternIndex;
}

export default function PatternView() {
  const moduleId = useStore((state) => state.moduleId);
  const moduleVersion = useStore((state) => state.patternRevision);
  const numTracks = useStore((state) => state.module.numTracks);
  const setCurrentPattern = useStore((state) => state.setCurrentPattern);
  const playing = useStore((state) => state.transportState.playing);
  const effectsDisplayed = useStore((state) => state.effectsDisplayed);
  const cursorTrack = useStore((state) => state.editorState.cursorPosition.track);
  const containerRef = useRef<HTMLDivElement | null>(null);
  const canvasRef = useRef<HTMLCanvasElement | null>(null);
  const canvasSizeRef = useRef({ width: 0, height: 0 });
  const [viewportSize, setViewportSize] = useState({ width: 0, height: 0 });
  const wheelDeltaRef = useRef(0);
  const { patternNo, patternLength } = useSequencePosition();

  const coloursAtPlayhead: Colours = {
    background: cssProperty("--colour-panel-bg"),
    beatLine: cssProperty("--colour-beat-line"),
    trackLaneSeparator: cssProperty("--colour-track-lane-separator"),
    trackHeaderMutedFg: cssProperty("--colour-track-header-muted-fg"),
    trackHeaderMutedBg: cssProperty("--colour-track-header-muted-bg"),
    trackHeaderNotMutedFg: cssProperty("--colour-track-header-not-muted-fg"),
    trackHeaderNotMutedBg: cssProperty("--colour-track-header-not-muted-bg"),
    trackFooterMutedFg: cssProperty("--colour-track-footer-muted-fg"),
    trackFooterMutedBg: cssProperty("--colour-track-footer-muted-bg"),
    trackFooterPanGuide: cssProperty("--colour-track-footer-pan-guide"),
    trackFooterPanMarker: cssProperty("--colour-track-footer-pan-marker"),
    trackFooterLeftLabel: cssProperty("--colour-track-footer-left-label"),
    trackFooterRightLabel: cssProperty("--colour-track-footer-right-label"),
    trackFooterNotMutedBg: cssProperty("--colour-track-footer-not-muted-bg"),
    playheadBackground: cssProperty("--colour-playhead"),
    text: cssProperty("--colour-pattern-text-bright"),
    channelMuted: cssProperty("--colour-pattern-text-channel-muted"),
    cursor: cssProperty("--colour-cursor"),
    cursorNonEdit: cssProperty("--colour-cursor-non-edit"),
    cursorText: cssProperty("--colour-cursor-text"),
    note: cssProperty("--colour-note-at-playhead"),
    sample: cssProperty("--colour-sample-at-playhead"),
    command: cssProperty("--colour-command-at-playhead"),
    effect: cssProperty("--colour-effect-at-playhead"),
    selectionBox: cssProperty("--colour-selection-fill"),
    selectionBoxOutline: cssProperty("--colour-selection-outline"),
  };

  const coloursOffPlayhead: Colours = {
    background: cssProperty("--colour-panel-bg"),
    beatLine: cssProperty("--colour-beat-line"),
    trackLaneSeparator: cssProperty("--colour-track-lane-separator"),
    trackHeaderMutedFg: cssProperty("--colour-track-header-muted-fg"),
    trackHeaderMutedBg: cssProperty("--colour-track-header-muted-bg"),
    trackHeaderNotMutedFg: cssProperty("--colour-track-header-not-muted-fg"),
    trackHeaderNotMutedBg: cssProperty("--colour-track-header-not-muted-bg"),
    trackFooterMutedFg: cssProperty("--colour-track-footer-muted-fg"),
    trackFooterMutedBg: cssProperty("--colour-track-footer-muted-bg"),
    trackFooterPanGuide: cssProperty("--colour-track-footer-pan-guide"),
    trackFooterPanMarker: cssProperty("--colour-track-footer-pan-marker"),
    trackFooterLeftLabel: cssProperty("--colour-track-footer-left-label"),
    trackFooterRightLabel: cssProperty("--colour-track-footer-right-label"),
    trackFooterNotMutedBg: cssProperty("--colour-track-footer-not-muted-bg"),
    playheadBackground: cssProperty("--colour-playhead"),
    text: cssProperty("--colour-pattern-text-muted"),
    channelMuted: cssProperty("--colour-pattern-text-channel-muted"),
    cursor: cssProperty("--colour-cursor"),
    cursorNonEdit: cssProperty("--colour-cursor-non-edit"),
    cursorText: cssProperty("--colour-cursor-text"),
    note: cssProperty("--colour-note"),
    sample: cssProperty("--colour-sample"),
    command: cssProperty("--colour-command"),
    effect: cssProperty("--colour-effect"),
    selectionBox: cssProperty("--colour-selection-fill"),
    selectionBoxOutline: cssProperty("--colour-selection-outline"),
  };

  const trackHeaderFont: string = cssProperty("--font-track-header");
  const patternDataFont: string = cssProperty("--font-pattern-data");

  const ensureCanvasSize = (
    canvas: HTMLCanvasElement,
    minWidth: number,
    minHeight: number,
  ) => {
    const current = canvasSizeRef.current;
    if (current.width >= minWidth && current.height >= minHeight) {
      return;
    }
    const nextWidth = Math.max(current.width, minWidth);
    const nextHeight = Math.max(current.height, minHeight);
    resizeCanvas(canvas, nextWidth, nextHeight);
    canvasSizeRef.current = {
      width: nextWidth,
      height: nextHeight,
    };
  };

  const getPatternViewRenderer = (ctx: CanvasRenderingContext2D) => {
    const patternRenderer = new PatternRenderer(
      ctx,
      coloursAtPlayhead,
      coloursOffPlayhead,
      trackHeaderFont,
      patternDataFont,
      numTracks,
      effectsDisplayed,
      viewportSize,
    );
    return () => {
      const { track, field } = useStore.getState().editorState.cursorPosition;
      patternRenderer.renderPattern({
        playheadIndex: getPatternIndex(),
        pattern: useStore.getState().currentPattern,
        linesPerBeat: useStore.getState().module.linesPerBeat,
        trackMuteState: useStore.getState().trackMuteState,
        trackPanning: useStore.getState().trackPanning,
        cursorTrack: track,
        cursorField: field,
        editing: patternEvents.editing(),
        patternSelection: useStore.getState().patternSelection,
      });
    };
  };

  const handleWheel = (event: React.WheelEvent<HTMLCanvasElement>) => {
    event.preventDefault();
    wheelDeltaRef.current += normalisedWheelDelta(event);
    while (wheelDeltaRef.current >= wheelScrollThreshold) {
      commands.patternGridDown(false);
      wheelDeltaRef.current -= wheelScrollThreshold;
    }
    while (wheelDeltaRef.current <= -wheelScrollThreshold) {
      commands.patternGridUp(false);
      wheelDeltaRef.current += wheelScrollThreshold;
    }
  };

  const handlePointer = (event: React.MouseEvent<HTMLCanvasElement>) => {
    event.preventDefault();
    const container = containerRef.current;
    if (!container) return;
    const boundingRect = event.currentTarget.getBoundingClientRect();
    const pointerX = event.clientX - boundingRect.left;
    const pointerY = event.clientY - boundingRect.top;
    const playheadIndex = getPatternIndex();
    const patternLength = useStore.getState().currentPattern.lines.length;
    const clickedPosition = patternLayout.pointerClickedOn(
      pointerX,
      pointerY,
      { width: container.clientWidth, height: container.clientHeight },
      playheadIndex,
      numTracks,
      effectsDisplayed,
      patternLength,
    );
    if (!clickedPosition) return;
    switch (clickedPosition.objectType) {
      case "trackHeader":
        commands.toggleTrackMute(clickedPosition.track);
        break;
      case "patternEvent":
        if (!playing) {
          commands.patternGridJumpToLocation(
            clickedPosition.event.track,
            clickedPosition.event.patternIndex,
            event.shiftKey,
          );
        }
        break;
    }
  };

  useEffect(() => {
    // Register the pattern renderer.
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext("2d");
    if (!ctx) return;
    const container = containerRef.current;
    if (!container) return;
    const viewportWidth = container.clientWidth;
    const viewportHeight = container.clientHeight;
    const { contentWidth } = getPatternContentDimensions(
      { width: viewportWidth, height: viewportHeight },
      numTracks,
      effectsDisplayed,
    );
    const logicalWidth = Math.max(contentWidth, viewportWidth);
    ensureCanvasSize(canvas, logicalWidth * 1.5, viewportHeight * 1.5);
    const renderPatternView = getPatternViewRenderer(ctx);
    return animation.registerRenderer(() => renderPatternView());
  }, [
    trackHeaderFont,
    patternDataFont,
    coloursAtPlayhead.background,
    coloursAtPlayhead.beatLine,
    coloursAtPlayhead.trackLaneSeparator,
    coloursAtPlayhead.trackHeaderMutedFg,
    coloursAtPlayhead.trackHeaderMutedBg,
    coloursAtPlayhead.trackHeaderNotMutedFg,
    coloursAtPlayhead.trackHeaderNotMutedBg,
    coloursAtPlayhead.trackFooterMutedFg,
    coloursAtPlayhead.trackFooterMutedBg,
    coloursAtPlayhead.trackFooterPanGuide,
    coloursAtPlayhead.trackFooterPanMarker,
    coloursAtPlayhead.trackFooterLeftLabel,
    coloursAtPlayhead.trackFooterRightLabel,
    coloursAtPlayhead.trackFooterNotMutedBg,
    coloursAtPlayhead.playheadBackground,
    coloursAtPlayhead.text,
    coloursAtPlayhead.channelMuted,
    coloursAtPlayhead.cursor,
    coloursAtPlayhead.cursorNonEdit,
    coloursAtPlayhead.cursorText,
    coloursAtPlayhead.note,
    coloursAtPlayhead.sample,
    coloursAtPlayhead.effect,
    coloursAtPlayhead.selectionBox,
    coloursAtPlayhead.selectionBoxOutline,
    coloursOffPlayhead.background,
    coloursOffPlayhead.beatLine,
    coloursOffPlayhead.trackLaneSeparator,
    coloursOffPlayhead.trackHeaderMutedFg,
    coloursOffPlayhead.trackHeaderMutedBg,
    coloursOffPlayhead.trackHeaderNotMutedFg,
    coloursOffPlayhead.trackHeaderNotMutedBg,
    coloursOffPlayhead.trackFooterMutedFg,
    coloursOffPlayhead.trackFooterMutedBg,
    coloursAtPlayhead.trackFooterPanGuide,
    coloursAtPlayhead.trackFooterPanMarker,
    coloursAtPlayhead.trackFooterLeftLabel,
    coloursAtPlayhead.trackFooterRightLabel,
    coloursOffPlayhead.trackFooterNotMutedBg,
    coloursOffPlayhead.playheadBackground,
    coloursOffPlayhead.text,
    coloursOffPlayhead.channelMuted,
    coloursOffPlayhead.cursor,
    coloursOffPlayhead.cursorNonEdit,
    coloursOffPlayhead.cursorText,
    coloursOffPlayhead.note,
    coloursOffPlayhead.sample,
    coloursOffPlayhead.effect,
    coloursOffPlayhead.selectionBox,
    coloursOffPlayhead.selectionBoxOutline,
    numTracks,
    effectsDisplayed,
    cursorTrack,
    viewportSize.width,
    viewportSize.height,
  ]);

  useEffect(() => {
    // Keep updated with the current width/height of the viewport.
    const container = containerRef.current;
    if (!container) return;
    const observer = new ResizeObserver(([entry]) => {
      setViewportSize({
        width: entry.contentRect.width,
        height: entry.contentRect.height,
      });
      const canvas = canvasRef.current;
      if (canvas) {
        const { contentWidth } = getPatternContentDimensions(
          viewportSize,
          numTracks,
          effectsDisplayed,
        );
        const width = Math.max(contentWidth, entry.contentRect.width);
        const height = entry.contentRect.height;
        ensureCanvasSize(canvas, width, height);
      }
    });
    observer.observe(container);
    return () => observer.disconnect();
  }, [numTracks, effectsDisplayed]);

  useEffect(() => {
    // Get the current pattern whenever it changes and we are not playing.
    if (!playing && patternLength > 0 && numTracks > 0)
      engine
        .getPattern(patternNo, patternLength, numTracks)
        .then((patternLines) =>
          setCurrentPattern({ patternNo, lines: patternLines }),
        );
  }, [
    moduleId,
    moduleVersion,
    patternNo,
    patternLength,
    numTracks,
    setCurrentPattern,
  ]);

  useSyncCursorWithTransport();

  return (
    <div ref={containerRef} className="patternView uiArea" id="patternView">
      <canvas
        className="uiArea"
        ref={canvasRef}
        onWheel={handleWheel}
        onClick={handlePointer}
        width="1024"
        height="1024"
      />
    </div>
  );
}
