import { useStore } from "../store/useStore.ts";
import "./SampleList.css";
import { hexadecimal } from "../rendering/hexadecimal.ts";

export default function SampleList() {
  const samples = useStore((state) => state.module.samples);
  const selectedSample = useStore((state) => state.selectedSample);
  const { setSelectedSample } = useStore.getState();
  return (
    <div className="sampleList uiArea">
      {samples
        .map((sample, index) => ({ sample, index }))
        .filter(({ sample }) => sample.sampleLength > 0)
        .map(({ sample, index }) => (
          <div key={index} className="sample">
            <button
              type="button"
              className={index === selectedSample ? "selected" : ""}
              onClick={() => setSelectedSample(index)}
            >
              {hexadecimal.toHex(index + 1, 2)}{': '}{sample.name}
            </button>
          </div>
        ))}
    </div>
  );
}
