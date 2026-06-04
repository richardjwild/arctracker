import "./PatternView.css";
import { useStore } from "../store/useStore.ts";
import { useEffect, useRef } from "react";
import { engine } from "../engine/engine.ts";
import { animation } from "../rendering/animation.ts";
import {
  getPatternContentDimensions,
  PatternRenderer,
} from "../rendering/renderPattern.ts";
import useSyncCursorWithTransport from "../hooks/useSyncCursorWithTransport.ts";
import { patternGrid } from "../editing/patternGrid.ts";
import useSequencePosition from "../hooks/useSequencePosition.ts";

export default function PatternView() {
  const moduleId = useStore((state) => state.moduleId);
  const moduleVersion = useStore((state) => state.patternRevision);
  const numChannels = useStore((state) => state.module.numChannels);
  const setCurrentPattern = useStore((state) => state.setCurrentPattern);
  const containerRef = useRef<HTMLDivElement | null>(null);
  const canvasRef = useRef<HTMLCanvasElement | null>(null);
  const canvasSizeRef = useRef({ width: 0, height: 0 });
  const viewportSizeRef = useRef({ width: 0, height: 0 });
  const { patternNo, patternLength } = useSequencePosition();

  const resizeCanvas = (
    canvas: HTMLCanvasElement,
    logicalWidth: number,
    logicalHeight: number,
  ) => {
    const dpr = window.devicePixelRatio || 1;
    canvas.width = logicalWidth * dpr;
    canvas.height = logicalHeight * dpr;
    canvas.style.width = `${logicalWidth}px`;
    canvas.style.height = `${logicalHeight}px`;
    const ctx = canvas.getContext("2d");
    ctx?.setTransform(dpr, 0, 0, dpr, 0, 0);
  };

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

  const getPatternIndex = (): number => {
    return useStore.getState().transportState.playing
      ? useStore.getState().transportState.patternIndex
      : patternGrid.currentPosition().patternIndex;
  }

  const getPatternViewRenderer = (
    ctx: CanvasRenderingContext2D,
    numChannels: number,
  ) => {
    return () => {
      const currentPattern = useStore.getState().currentPattern;
      const patternIndex = getPatternIndex();
      const patternRenderer = new PatternRenderer(
        currentPattern,
        ctx,
        viewportSizeRef.current,
        numChannels,
      );
      patternRenderer.renderPattern(patternIndex);
    };
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
    const { contentWidth } = getPatternContentDimensions(numChannels);
    const logicalWidth = Math.max(contentWidth, viewportWidth);
    const logicalHeight = viewportHeight;
    ensureCanvasSize(canvas, logicalWidth * 1.5, logicalHeight * 1.5);
    const renderPatternView = getPatternViewRenderer(ctx, numChannels);
    return animation.registerRenderer(() => renderPatternView());
  }, [numChannels]);

  useEffect(() => {
    // Keep updated with the current width/height of the viewport.
    const container = containerRef.current;
    if (!container) return;
    const observer = new ResizeObserver(([entry]) => {
      viewportSizeRef.current = {
        width: entry.contentRect.width,
        height: entry.contentRect.height,
      };
      const canvas = canvasRef.current;
      if (canvas) {
        const { contentWidth } = getPatternContentDimensions(numChannels);
        const width = Math.max(contentWidth, entry.contentRect.width);
        const height = entry.contentRect.height;
        ensureCanvasSize(canvas, width, height);
      }
    });
    observer.observe(container);
    return () => observer.disconnect();
  }, [numChannels]);

  useEffect(() => {
    // Get the current pattern whenever it changes.
    if (patternLength > 0 && numChannels > 0)
      engine
        .getPattern(patternNo, patternLength, numChannels)
        .then((patternLines) =>
          setCurrentPattern({ patternNo, lines: patternLines }),
        );
  }, [
    moduleId,
    moduleVersion,
    patternNo,
    patternLength,
    numChannels,
    setCurrentPattern,
  ]);

  useSyncCursorWithTransport();

  return (
    <div ref={containerRef} className="patternView uiArea" id="patternView">
      <canvas className="uiArea" ref={canvasRef} width="1024" height="1024" />
    </div>
  );
}
