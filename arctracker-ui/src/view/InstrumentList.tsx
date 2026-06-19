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
          ? Math.max(1, Math.floor((viewportHeight) / (cellHeight + gap)) - 1)
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
    setFirstVisiblePos(() => {
      if (selectedInstrument === null) return 0;
      if (selectedInstrument >= visibleCount)
        return 1 + selectedInstrument - visibleCount;
      else
        return 0;
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
            className={
              selectedInstrument === index + firstVisiblePos ? "selected" : ""
            }
            ref={cellRef}
            onClick={(e) => {
              e.preventDefault();
              if (selectedInstrument === index + firstVisiblePos)
                commands.openInstrumentEditor();
              else {
                setSelectedInstrument(index + firstVisiblePos);
                if (e.shiftKey) commands.openInstrumentEditor();
              }
            }}
          >
            {hexadecimal.toHex(index + firstVisiblePos + 1, 2)}
            {": "}
            {instrument.assigned ? instrument.name : "(empty)"}
          </button>
        ))}
      <button
        type="button"
        ref={cellRef}
        onClick={(e) => {
          e.preventDefault();
          setSelectedInstrument(instruments.length);
          commands.openInstrumentEditor();
        }}
      >
        Add instrument
      </button>
    </div>
  );
}
