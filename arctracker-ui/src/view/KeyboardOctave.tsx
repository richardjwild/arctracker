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

const SilenceAudioIcon = () => (
  <svg
    xmlns="http://www.w3.org/2000/svg"
    height="24px"
    viewBox="0 -960 960 960"
    width="24px"
    fill="currentColor"
  >
    <path d="m616-320-56-56 104-104-104-104 56-56 104 104 104-104 56 56-104 104 104 104-56 56-104-104-104 104Zm-496-40v-240h160l200-200v640L280-360H120Zm280-246-86 86H200v80h114l86 86v-252ZM300-480Z" />
  </svg>
);

export default function KeyboardOctave() {
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
        <canvas
          className="keyboardGraphic"
          ref={canvasRef}
          width={CanvasWidth}
          height={CanvasHeight}
        ></canvas>
        <button
          className="killAllAudio"
          title={message("panic")}
          onClick={commands.silenceAllAudio}
        >
          <SilenceAudioIcon />
        </button>
      </div>
    </div>
  );
}
