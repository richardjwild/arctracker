import { useStore } from "../store/useStore.ts";
import "./InstrumentList.css";
import { hexadecimal } from "../rendering/hexadecimal.ts";
import {commands} from "../control/commands.ts";
import {useEffect, useRef, useState} from "react";

export default function InstrumentList() {
  const instruments = useStore((state) => state.module.instruments);
  const { selectedInstrument, setSelectedInstrument } = useStore(
    (state) => state,
  );
  const containerRef = useRef<HTMLDivElement | null>(null);
  const cellRef = useRef<HTMLButtonElement | null>(null);
  const [viewportHeight, setViewportHeight] = useState(0);
  const [cellHeight, setCellHeight] = useState(0);
  const [firstVisiblePos, setFirstVisiblePos] = useState(0);

  const gap = 5;
  const visibleCount =
      cellHeight > 0
          ? Math.max(1, Math.floor((viewportHeight) / (cellHeight + gap)))
          : 1;

  useEffect(() => {
    const container = containerRef.current;
    if (!container) return;
    const observer = new ResizeObserver(([entry]) => {
      setViewportHeight(entry.contentRect.height);
    });
    observer.observe(container);
    return () => observer.disconnect();
  }, []);

  useEffect(() => {
    if (cellRef.current) {
      setCellHeight(cellRef.current.offsetHeight);
    }
  }, [instruments.length]);

  useEffect(() => {
    setFirstVisiblePos((current) => {
      if (selectedInstrument === null) return 0;
      if (selectedInstrument < current) return selectedInstrument;
      if (selectedInstrument >= current + visibleCount) {
        return selectedInstrument - visibleCount + 1;
      }
      return current;
    });
  }, [selectedInstrument, visibleCount])

  const visibleInstruments = instruments.slice(
      firstVisiblePos,
      firstVisiblePos + visibleCount,
  );

  return (
    <div className="sampleList uiArea" ref={containerRef}>
      {visibleInstruments
        .map((instrument, index) => ({ instrument, index }))
        .map(({ instrument, index }) => (
          <button
            key={index}
            type="button"
            className={index === selectedInstrument ? "selected" : ""}
            ref={cellRef}
            onClick={(e) => {
              e.preventDefault();
              setSelectedInstrument(index);
              if (e.shiftKey) commands.openInstrumentEditor();
            }}
          >
            {hexadecimal.toHex(index + 1, 2)}
            {": "}
            {instrument.assigned ? instrument.name : "(empty)"}
          </button>
        ))}
    </div>
  );
}
