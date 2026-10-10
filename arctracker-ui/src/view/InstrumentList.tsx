import { useStore } from "../store/useStore.ts";
import "./InstrumentList.css";
import { hexadecimal } from "../rendering/hexadecimal.ts";
import {commands} from "../control/commands.ts";
import {useEffect, useRef, useState} from "react";
import { Instrument } from "../editing/editInstrument.ts";
import { message } from "../language/messages.ts";

const AddInstrumentIcon = () => (
  <svg
    xmlns="http://www.w3.org/2000/svg"
    height="19px"
    viewBox="0 -960 960 960"
    width="19px"
    fill="currentColor"
  >
    <path d="M440-440H200v-80h240v-240h80v240h240v80H520v240h-80v-240Z" />
  </svg>
);

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
    setFirstVisiblePos((previous) => {
      if (selectedInstrument === null) return 0;
      if (selectedInstrument < previous) {
        // Selected instrument is above the viewport.
        return selectedInstrument;
      }
      if (selectedInstrument >= previous + visibleCount) {
        // Selected instrument is below the viewport.
        return selectedInstrument - visibleCount + 1;
      }
      const currentlyDisplayedCount = instruments.length - previous;
      if (visibleCount > currentlyDisplayedCount) {
        // We can show more instruments than we currently are showing.
        return Math.max(previous - (visibleCount - currentlyDisplayedCount), 0);
      }
      // Selected instrument is already visible. Don't do anything!
      return previous;
    });
  }, [selectedInstrument, visibleCount])

  const visibleInstruments = instruments.slice(
      firstVisiblePos,
      firstVisiblePos + visibleCount,
  );

  return (
    <div className="sampleList uiArea" ref={containerRef}>
      {visibleInstruments
        .map((instrument: Instrument, index: number) => ({ instrument, index }))
        .map(
          ({
            instrument,
            index,
          }: {
            instrument: Instrument;
            index: number;
          }) => (
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
              {" "}
              {instrument.assigned
                ? `♫ ${instrument.name}`
                : ""}
            </button>
          ),
        )}
      <button
        type="button"
        className="addInstrument"
        title={message("addInstrumentHintText")}
        ref={cellRef}
        onClick={(e) => {
          e.preventDefault();
          commands.addInstrument();
        }}
      >
        <AddInstrumentIcon />
      </button>
    </div>
  );
}
