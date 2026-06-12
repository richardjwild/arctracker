import { pianoKeys } from "./pianoKeys.ts";
import { editorKeyHandlers } from "../editing/keyHandlers.ts";
import { KeyHandler } from "./keyHandler.ts";
import { keyBinding } from "./keyBinding.ts";
import { editor } from "../editing/editor.ts";

const handlers: KeyHandler[] = [
  pianoKeys.handleRealtimePianoInput,
  editorKeyHandlers.handleSampleFieldInput,
  editorKeyHandlers.handleEffectFieldInput,
  keyBinding.handleKey,
];

export const keyboardEventListener = (e: KeyboardEvent) => {
  if (editor.inputtingText()) return;
  for (const handler of handlers) {
    if (handler(e)) {
      e.preventDefault();
      return;
    }
  }
};
