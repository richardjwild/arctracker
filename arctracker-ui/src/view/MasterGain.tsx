import "./MasterGain.css";
import { useStore } from "../store/useStore.ts";

function toText(value: number) {
  return `${Math.round(value * 100)}%`;
}

export default function MasterGain() {
  const masterGain = useStore((state) => state.module.masterGain);
  const setMasterGain = useStore((state) => state.setMasterGain);

  return (
    <div className="masterGain uiArea">
      <input
        type="range"
        min="0"
        max="1"
        step="0.01"
        value={masterGain}
        onChange={(e) => setMasterGain(Number(e.target.value))}
      />
      <span className="gainValue">{toText(masterGain)}</span>
      <canvas className="vuMeter"></canvas>
    </div>
  );
}
