import "./KeyboardOctave.css";
import { message } from "../language/messages.ts";
import { useEffect, useRef } from "react";
import { useStore } from "../store/useStore.ts";
import { commands } from "../control/commands.ts";

function cssProperty(name: string): string {
  return getComputedStyle(document.documentElement)
    .getPropertyValue(name)
    .trim();
}

export default function KeyboardOctave() {
  const ArrowLeftIcon = () => (
    <svg
      xmlns="http://www.w3.org/2000/svg"
      height="24px"
      viewBox="0 -960 960 960"
      width="24px"
      fill="currentColor"
    >
      <path d="M400-240 160-480l240-240 56 58-142 142h486v80H314l142 142-56 58Z" />
    </svg>
  );

  const ArrowRightIcon = () => (
    <svg
      xmlns="http://www.w3.org/2000/svg"
      height="24px"
      viewBox="0 -960 960 960"
      width="24px"
      fill="currentColor"
    >
      <path d="m560-240-56-58 142-142H160v-80h486L504-662l56-58 240 240-240 240Z" />
    </svg>
  );

  const canvasRef = useRef<HTMLCanvasElement | null>(null);
  const GraphicWidth = 210;
  const GraphicHeight = 50;
  const pianoKeyboardTranspose = useStore(
    (state) => state.pianoKeyboardTranspose,
  );
  const currentOctave = (pianoKeyboardTranspose - 1) / 12;
  const whiteKeyColor = cssProperty("--colour-piano-key-white");
  const blackKeyColor = cssProperty("--colour-piano-key-black");
  const octaveHighlight = cssProperty("--colour-piano-octave-highlight");

  const renderOctave = (ctx: CanvasRenderingContext2D, x: number) => {
    ctx.fillStyle = whiteKeyColor;
    for (let whiteNote = 0; whiteNote < 7; whiteNote++) {
      ctx.fillRect(x + whiteNote * 6, 10, 5, GraphicHeight - 20);
    }
    ctx.fillStyle = blackKeyColor;
    ctx.fillRect(x + 3.5, 10, 4, GraphicHeight - 30);
    ctx.fillRect(x + 9.5, 10, 4, GraphicHeight - 30);
    ctx.fillRect(x + 21.5, 10, 4, GraphicHeight - 30);
    ctx.fillRect(x + 27.5, 10, 4, GraphicHeight - 30);
    ctx.fillRect(x + 33.5, 10, 4, GraphicHeight - 30);
  }

  const renderOctaveHighlight = (ctx: CanvasRenderingContext2D) => {
    ctx.strokeStyle = octaveHighlight;
    ctx.lineWidth = 2;
    ctx.strokeRect(1 + currentOctave * 42, 9, 82, GraphicHeight - 18);
  }

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext("2d");
    if (!ctx) return;
    ctx.clearRect(0, 0, GraphicWidth, GraphicHeight);
    for (let octave = 0; octave < 5; octave++)
      renderOctave(ctx, octave * 42);
    renderOctaveHighlight(ctx);
  }, [currentOctave]);

  return (
    <div className="keyboardOctave">
      <div className="control">
        <button
          type="button"
          title={message("shiftKeyboardOctaveDown")}
          onClick={commands.shiftKeyboardOctaveDown}
        >
          <ArrowLeftIcon />
        </button>
        <canvas
          className="keyboardGraphic"
          ref={canvasRef}
          width={GraphicWidth}
          height={GraphicHeight}
        ></canvas>
        <button
          type="button"
          title={message("shiftKeyboardOctaveUp")}
          onClick={commands.shiftKeyboardOctaveUp}
        >
          <ArrowRightIcon />
        </button>
      </div>
    </div>
  );
}
