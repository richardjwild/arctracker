import { useStore } from "../store/useStore.ts";
import { Cursor } from "../editing/cursor.ts";
import { engine } from "../engine/engine.ts";
import { commands } from "../control/commands.ts";
import { KeyHandler } from "./keyHandler.ts";
import { patternEvents } from "../editing/patternEvents.ts";
import { notes } from "../rendering/notes.ts";

const pianoLayout = [
  "KeyZ",
  "KeyS",
  "KeyX",
  "KeyD",
  "KeyC",
  "KeyV",
  "KeyG",
  "KeyB",
  "KeyH",
  "KeyN",
  "KeyJ",
  "KeyM",
  "KeyQ",
  "Digit2",
  "KeyW",
  "Digit3",
  "KeyE",
  "KeyR",
  "Digit5",
  "KeyT",
  "Digit6",
  "KeyY",
  "Digit7",
  "KeyU",
  "KeyI",
];

function noteFromKeyboard(e: KeyboardEvent): number | null {
  if (e.metaKey || e.ctrlKey || e.shiftKey || e.altKey) return null;
  const index = pianoLayout.indexOf(e.code);
  if (index === -1) return null;
  return useStore.getState().pianoKeyboardTranspose + index;
}

export const pianoKeyHandler: { handleRealtimePianoInput: KeyHandler } = {
  handleRealtimePianoInput: (e) => {
    const cursorField = new Cursor().currentField();
    if (!patternEvents.editing() || cursorField.field === "note") {
      const pianoKey = noteFromKeyboard(e);
      if (pianoKey !== null) {
        engine.noteOn(pianoKey);
        commands.editNoteField(pianoKey);
        return true;
      }
    }
    return false;
  },
}

export const pianoKeys = {
  shiftOctave: (delta: number) => {
    const pianoKeyboardTranspose = useStore.getState().pianoKeyboardTranspose;
    const setPianoKeyboardTranspose = useStore.getState().setPianoKeyboardTranspose;
    const newTranspose = pianoKeyboardTranspose + delta * 12;
    if (notes.inRange(newTranspose) && notes.inRange(newTranspose + 23))
      setPianoKeyboardTranspose(newTranspose);
  },
};
