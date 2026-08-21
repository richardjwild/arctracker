import "./MasterGain.css";
import { useStore } from "../store/useStore.ts";
import { engine } from "../engine/engine.ts";
import { useEffect, useRef } from "react";
import { animation } from "../rendering/animation.ts";
import {
  VuMeterColours,
  VuMeterRenderer,
} from "../rendering/renderVuMeters.ts";

function cssColour(name: string): string {
  return getComputedStyle(document.documentElement)
    .getPropertyValue(name)
    .trim();
}

export default function MasterGain() {
  const canvasRef = useRef<HTMLCanvasElement | null>(null);
  const masterGain = useStore((state) => state.module.masterGain);
  const setMasterGain = useStore((state) => state.setMasterGain);
  const updateMasterGain = (gain: number) => {
    setMasterGain(gain);
    engine.setMasterGain(gain);
  };
  const colours: VuMeterColours = {
    clipIndicatorOff: cssColour("--colour-clip-indicator-off"),
    clipIndicatorOn: cssColour("--colour-clip-indicator-on"),
    guide: cssColour("--colour-vu-meter-guide"),
    vuMeterBottom: cssColour("--colour-vu-meter-bottom"),
    vuMeterMiddle: cssColour("--colour-vu-meter-middle"),
    vuMeterTop: cssColour("--colour-vu-meter-top"),
  };

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext("2d");
    if (!ctx) return;
    const renderer = new VuMeterRenderer(ctx, colours);
    return animation.registerRenderer((timestamp) => {
      void renderer.render(timestamp);
    });
  }, [
    colours.clipIndicatorOff,
    colours.clipIndicatorOn,
    colours.guide,
    colours.vuMeterBottom,
    colours.vuMeterMiddle,
    colours.vuMeterTop,
  ]);

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
      <span className="gainValue">{Math.round(masterGain * 100) + "%"}</span>
      <canvas
        className="vuMeter"
        ref={canvasRef}
        width={240}
        height={100}
      ></canvas>
    </div>
  );
}
