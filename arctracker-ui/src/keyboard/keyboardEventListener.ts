import { pianoKeyHandler } from "./pianoKeys.ts";
import { editorKeyHandlers } from "../editing/keyHandlers.ts";
import { KeyHandler } from "./keyHandler.ts";
import { keyBinding } from "./keyBinding.ts";
import { editor } from "../editing/editor.ts";

const handlers: KeyHandler[] = [
  editorKeyHandlers.handleModuleTitleInput,
  editorKeyHandlers.handleSampleFieldInput,
  editorKeyHandlers.handleEffectFieldInput,
  editorKeyHandlers.handleTempoInput,
  editorKeyHandlers.handleAppConfigInput,
  editorKeyHandlers.handleHexCalculatorInput,
  editorKeyHandlers.handleSetMultipleEffectsInput,
  editorKeyHandlers.handleCommandReferenceInput,
  keyBinding.handleKey,
];

const registeredHandlers = new Set<KeyHandler>();

export const registerKeyHandler = (handler: KeyHandler) => {
  registeredHandlers.add(handler);
  return () => registeredHandlers.delete(handler);
}

export const keyboardEventListener = (e: KeyboardEvent) => {
  if (pianoKeyHandler.handleRealtimePianoInput(e)) {
    e.preventDefault();
    return;
  }
  if (e.target instanceof HTMLInputElement && (e.target.type === "range" || e.target.type === "radio")) {
    if (e.code === "Enter" || e.code === "Escape") {
      e.preventDefault();
      e.target.blur();
    }
    return;
  }
  if (editor.inputtingText()) {
    if (e.code === "Enter" || e.code === "Escape") {
      e.preventDefault();
      (document.activeElement as HTMLElement)?.blur();
    }
    return;
  }
  for (const registeredHandler of registeredHandlers) {
    if (registeredHandler(e)) {
      e.preventDefault();
      return;
    }
  }
  for (const handler of handlers) {
    if (handler(e)) {
      e.preventDefault();
      return;
    }
  }
};
