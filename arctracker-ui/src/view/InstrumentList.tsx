import { useStore } from "../store/useStore.ts";
import "./SampleList.css";
import { hexadecimal } from "../rendering/hexadecimal.ts";

export default function InstrumentList() {
  const instruments = useStore((state) => state.module.instruments);
  const { selectedInstrument, setSelectedInstrument } = useStore(
    (state) => state,
  );
  return (
    <div className="sampleList uiArea padded">
      {instruments
        .map((instrument, index) => ({ instrument, index }))
        .map(({ instrument, index }) => (
          <div key={index} className="sample">
            <button
              type="button"
              className={index === selectedInstrument ? "selected" : ""}
              onClick={() => setSelectedInstrument(index)}
            >
              {hexadecimal.toHex(index + 1, 2)}
              {": "}
              {instrument.assigned ? instrument.sample.name : "(empty)"}
            </button>
          </div>
        ))}
    </div>
  );
}
