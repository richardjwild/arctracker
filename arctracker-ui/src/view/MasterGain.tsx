import "./MasterGain.css";
import { useStore } from "../store/useStore.ts";
import { engine } from "../engine/engine.ts";
import { useEffect, useRef } from "react";
import { animation } from "../rendering/animation.ts";

function toText(value: number) {
  return `${Math.round(value * 100)}%`;
}

function cssColour(name: string): string {
  return getComputedStyle(document.documentElement)
    .getPropertyValue(name)
    .trim();
}

const FALL_PER_SECOND = 1.5; // full-scale units per second

export default function MasterGain() {
  const displayedPeaksRef = useRef({ left: 0, right: 0 });
  const lastFrameTimeRef = useRef<number | null>(null);
  const canvasRef = useRef<HTMLCanvasElement | null>(null);
  const masterGain = useStore((state) => state.module.masterGain);
  const setMasterGain = useStore((state) => state.setMasterGain);
  const updateMasterGain = (gain: number) => {
    setMasterGain(gain);
    engine.setMasterGain(gain);
  };
  const clipIndicatorOffColour = cssColour("--colour-clip-indicator-off");
  const vuMeterGuideColour = cssColour("--colour-vu-meter-guide");
  const bottomColour = cssColour("--colour-vu-meter-bottom");
  const topColour = cssColour("--colour-vu-meter-top");

  const renderVuMeter = async (ctx: CanvasRenderingContext2D) => {
    const now = performance.now();
    const last = lastFrameTimeRef.current ?? now;
    const dt = (now - last) / 1000;
    lastFrameTimeRef.current = now;
    const peakLevels = await engine.getAndResetPeakLevels();
    const displayed = displayedPeaksRef.current;
    displayed.left = Math.max(
      peakLevels.left,
      displayed.left - FALL_PER_SECOND * dt,
    );
    displayed.right = Math.max(
      peakLevels.right,
      displayed.right - FALL_PER_SECOND * dt,
    );
    ctx.clearRect(0, 0, 240, 100);
    // draw VU meters
    ctx.fillStyle = clipIndicatorOffColour;
    ctx.fillRect(220, 30, 20, 40);
    ctx.strokeStyle = vuMeterGuideColour;
    ctx.lineWidth = 1;
    ctx.strokeRect(1, 24, 200, 22);
    ctx.strokeRect(1, 56, 200, 22);
    const gradient = ctx.createLinearGradient(1, 0, 201, 0);
    gradient.addColorStop(0, bottomColour);
    gradient.addColorStop(1, topColour);
    ctx.fillStyle = gradient;
    if (displayed.left > 0) ctx.fillRect(1, 24, 200 * displayed.left, 22);
    if (displayed.right > 0) ctx.fillRect(1, 56, 200 * displayed.right, 22);
  };

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext("2d");
    if (!ctx) return;
    return animation.registerRenderer(() => renderVuMeter(ctx));
  }, []);

  return (
    <div className="masterGain uiArea">
      <input
        type="range"
        min="0"
        max="1"
        step="0.01"
        value={masterGain}
        onChange={(e) => updateMasterGain(Number(e.target.value))}
      />
      <span className="gainValue">{toText(masterGain)}</span>
      <canvas
        className="vuMeter"
        ref={canvasRef}
        width={240}
        height={100}
      ></canvas>
    </div>
  );
}
