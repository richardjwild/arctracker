import "./MasterGain.css";

export default function MasterGain() {
  return (
    <div className="masterGain uiArea">
      <input type="range" min="0" max="1" step="0.01" />
      <span className="gainValue">&nbsp;25%</span>
      <canvas className="vuMeter"></canvas>
    </div>
  );
}
