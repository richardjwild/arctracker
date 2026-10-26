import { pianoKeyHandler } from "./pianoKeys.ts";
import { KeyHandler } from "./keyHandler.ts";
import { keyBinding } from "./keyBinding.ts";

const registeredHandlers = new Set<KeyHandler>();

export const registerKeyHandler = (handler: KeyHandler) => {
  registeredHandlers.add(handler);
  return () => {
    registeredHandlers.delete(handler);
  };
}

export const keyboardEventListener = (e: KeyboardEvent) => {
  //
  // Make sure we don't prevent <select>, <input type='text'>, <input type='range'>, <input type='radio'> from being
  // focused. Also, disable all key bindings while the element has focus, except for Enter, Tab, and Escape, which we
  // are configuring here to blur out of the focussed element.
  //
  if (
    e.target instanceof HTMLSelectElement ||
    (e.target instanceof HTMLInputElement &&
      (e.target.type === "text" || e.target.type === "range" || e.target.type === "radio"))
  ) {
    if (e.code === "Enter" || e.code === "Escape" || e.code === "Tab") {
      e.preventDefault();
      e.target.blur();
    }
    return;
  }
  //
  // Handle piano key presses on the keyboard. These are: ZSXDCVGBHNJM (lower octave), Q2W3ER5T6Y7U (upper octave).
  //
  if (pianoKeyHandler.handleRealtimePianoInput(e)) {
    e.preventDefault();
    return;
  }
  //
  // Now test the contextual key handlers that are registered/deregistered on the fly by dialogue boxes.
  //
  for (const registeredHandler of registeredHandlers) {
    if (registeredHandler(e)) {
      e.preventDefault();
      return;
    }
  }
  //
  // Finally, if none of the above handled the keyboard event, test whether any of the global bindings match it.
  //
  if (keyBinding.handleKey(e)) {
    e.preventDefault();
    return;
  }
};
