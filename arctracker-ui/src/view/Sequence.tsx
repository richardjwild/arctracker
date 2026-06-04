import { useStore } from "../store/useStore.ts";
import React, { useEffect, useRef, useState } from "react";
import { engine } from "../engine/engine.ts";
import "./Sequence.css";
import useSyncSequenceWithTransport from "../hooks/useSyncSequenceWithTransport.ts";
import { commands } from "../control/commands.ts";
import useSequencePosition from "../hooks/useSequencePosition.ts";

export default function Sequence() {
  const moduleId = useStore((state) => state.moduleId);
  const tuneLength = useStore((state) => state.module.tuneLength);
  const sequenceRevision = useStore((state) => state.sequenceRevision);
  const moduleSequence = useStore((state) => state.sequence);
  const playing = useStore((state) => state.transportState.playing);
  const numPatterns = useStore((state) => state.module.numPatterns);
  const { sequencePos } = useSequencePosition();

  const digits = Math.max(2, String(numPatterns - 1).length);
  const containerRef = useRef<HTMLDivElement | null>(null);
  const cellRef = useRef<HTMLButtonElement | null>(null);
  const [viewportWidth, setViewportWidth] = useState(0);
  const [cellWidth, setCellWidth] = useState(0);
  const [firstVisiblePos, setFirstVisiblePos] = useState(0);

  const gap = 5;
  const visibleCount =
    cellWidth > 0
      ? Math.max(1, Math.floor((viewportWidth + gap) / (cellWidth + gap)))
      : 1;

  useEffect(() => {
    engine.getSequence(tuneLength).then((sequence) => {
      useStore.getState().setSequence(sequence);
    });
  }, [moduleId, tuneLength, sequenceRevision]);

  useEffect(() => {
    const container = containerRef.current;
    if (!container) return;
    const observer = new ResizeObserver(([entry]) => {
      setViewportWidth(entry.contentRect.width);
    });
    observer.observe(container);
    return () => observer.disconnect();
  }, []);

  useEffect(() => {
    if (cellRef.current) {
      setCellWidth(cellRef.current.offsetWidth);
    }
  }, [digits, moduleSequence.length]);

  useEffect(() => {
    setFirstVisiblePos((current) => {
      if (sequencePos < current) return sequencePos;
      if (sequencePos >= current + visibleCount) {
        return sequencePos - visibleCount + 1;
      }
      return current;
    });
  }, [sequencePos, visibleCount]);

  useSyncSequenceWithTransport();

  const visibleSequence = moduleSequence.slice(
    firstVisiblePos,
    firstVisiblePos + visibleCount,
  );

  const InsertBeforeIcon = () => (
    <>
      <svg
        viewBox="0 0 24 24"
        width="24"
        height="24"
        fill="none"
        stroke="currentColor"
        strokeWidth="2"
      >
        <path d="M4 9v6M1 12h6" />
        <rect x="9" y="6" width="12" height="12" rx="1" />
      </svg>
      <span className="visually-hidden">Insert before</span>
    </>
  );

  const InsertAfterIcon = () => (
    <>
      <svg
        viewBox="0 0 24 24"
        width="24"
        height="24"
        fill="none"
        stroke="currentColor"
        strokeWidth="2"
      >
        <rect x="3" y="6" width="12" height="12" rx="1" />
        <path d="M20 9v6M17 12h6" />
      </svg>
      <span className="visually-hidden">Insert after</span>
    </>
  );

  const DeleteIcon = () => (
    <>
      <svg
        viewBox="0 0 24 24"
        width="24"
        height="24"
        fill="none"
        stroke="currentColor"
        strokeWidth="2"
      >
        <rect x="6" y="6" width="12" height="12" rx="1" />
        <path d="M9 9 L15 15 M15 9 L9 15" />
      </svg>
      <span className="visually-hidden">Delete</span>
    </>
  );

  const IncrementPatternButton = () => {
    return (
      <button
        type="button"
        disabled={playing}
        className={`changePatternButton increment ${playing ? "disabled" : "enabled"}`}
        onClick={commands.incrementPatternAtCurrentPosition}
      >
        <svg
          xmlns="http://www.w3.org/2000/svg"
          height="24px"
          viewBox="0 -960 960 960"
          width="24px"
          fill="currentColor"
        >
          <path d="m280-400 200-200 200 200H280Z" />
        </svg>
      </button>
    );
  };

  const DecrementPatternButton = () => {
    return (
      <button
        type="button"
        disabled={playing}
        className={`changePatternButton decrement ${playing ? "disabled" : "enabled"}`}
        onClick={commands.decrementPatternAtCurrentPosition}
      >
        <svg
          xmlns="http://www.w3.org/2000/svg"
          height="24px"
          viewBox="0 -960 960 960"
          width="24px"
          fill="currentColor"
        >
          <path d="M480-360 280-560h400L480-360Z" />
        </svg>
      </button>
    );
  };

  const renderSequencePosition = (
    patternNo: number,
    absolutePos: number,
    visibleIndex: number,
  ) => {
    const current = absolutePos === sequencePos ? "Current" : "";
    if (current)
      return (
        <div key={absolutePos} className="withPatternControls">
          <IncrementPatternButton />
          <button
            type="button"
            title="Increment pattern at this position"
            className="sequencePos current"
            style={{ "--sequence-digits": digits } as React.CSSProperties}
            ref={visibleIndex === 0 ? cellRef : undefined}
          >
            {patternNo.toString().padStart(digits, "0")}
          </button>
          <DecrementPatternButton />
        </div>
      );
    else
      return (
        <button
          type="button"
          title="Decrement pattern at this position"
          key={absolutePos}
          className="sequencePos"
          style={{ "--sequence-digits": digits } as React.CSSProperties}
          ref={visibleIndex === 0 ? cellRef : undefined}
          onClick={() => commands.sequenceSeek(absolutePos)}
        >
          {patternNo.toString().padStart(digits, "0")}
        </button>
      );
  };

  return (
    <div className="sequenceView uiArea">
      <div className="sequence" ref={containerRef}>
        {visibleSequence.map((patternNo, index) =>
          renderSequencePosition(patternNo, firstVisiblePos + index, index),
        )}
      </div>
      <div className="sequenceEditButtons">
        <button
          type="button"
          title="Insert new position before current"
          disabled={playing}
          className={playing ? "disabled" : "enabled"}
          onClick={() => commands.insertSequencePositionBefore(false)}
        >
          <InsertBeforeIcon />
        </button>
        <button
          type="button"
          title="Insert new position after current"
          disabled={playing}
          className={playing ? "disabled" : "enabled"}
          onClick={() => commands.insertSequencePositionAfter(false)}
        >
          <InsertAfterIcon />
        </button>
        <button
          type="button"
          title="Delete position at current"
          disabled={playing}
          className={playing ? "disabled" : "enabled"}
          onClick={commands.deleteSequencePosition}
        >
          <DeleteIcon />
        </button>
      </div>
    </div>
  );
}
