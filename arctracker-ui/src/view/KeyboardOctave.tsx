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

const CanvasWidth = 210;
const CanvasHeight = 50;
const KeyboardTop = 10;
const WhiteKeySpacing = 6;
const WhiteKeyWidth = WhiteKeySpacing - 1;
const WhiteKeyHeight = 30;
const BlackKeyOffset = 3.5;
const BlackKeyWidth = 4;
const BlackKeyHeight = 20;
const OctaveWidth = WhiteKeySpacing * 7;

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
  const pianoKeyboardTranspose = useStore(
    (state) => state.pianoKeyboardTranspose,
  );
  const currentOctave = pianoKeyboardTranspose / 12;
  const whiteKeySelectedColor = cssProperty(
    "--colour-piano-key-white-selected",
  );
  const whiteKeyNotSelectedColor = cssProperty(
    "--colour-piano-key-white-not-selected",
  );
  const blackKeyColor = cssProperty("--colour-piano-key-black");

  const isSelected = (octave: number) =>
    octave === currentOctave || octave === currentOctave + 1;

  const renderOctave = (ctx: CanvasRenderingContext2D, octave: number) => {
    ctx.fillStyle = isSelected(octave)
      ? whiteKeySelectedColor
      : whiteKeyNotSelectedColor;
    const x = octave * OctaveWidth;
    for (let whiteNote = 0; whiteNote < 7; whiteNote++) {
      ctx.fillRect(
        x + whiteNote * WhiteKeySpacing,
        KeyboardTop,
        WhiteKeyWidth,
        WhiteKeyHeight,
      );
    }
    ctx.fillStyle = blackKeyColor;
    [0, 1, 3, 4, 5].forEach((blackNote) =>
      ctx.fillRect(
        x + BlackKeyOffset + blackNote * WhiteKeySpacing,
        KeyboardTop,
        BlackKeyWidth,
        BlackKeyHeight,
      ),
    );
  };

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext("2d");
    if (!ctx) return;
    ctx.clearRect(0, 0, CanvasWidth, CanvasHeight);
    for (let octave = 0; octave < 5; octave++) renderOctave(ctx, octave);
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
          width={CanvasWidth}
          height={CanvasHeight}
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
