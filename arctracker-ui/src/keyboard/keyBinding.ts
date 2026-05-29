import { commands } from "../control/commands.ts";
import { KeyHandler } from "./keyHandler.ts";

function primaryModifier(e: KeyboardEvent) {
  return navigator.userAgent.includes("Mac")
    ? e.metaKey && !e.ctrlKey
    : e.ctrlKey && !e.metaKey;
}

type ModifierRequirement =
  | "none"
  | "primary"
  | "secondary"
  | "tertiary"
  | "primary+secondary";

interface KeyBinding {
  modifiers: ModifierRequirement;
  execute: () => void;
}

const keyBindings = new Map<string, KeyBinding[]>([
  ["Escape", [{ modifiers: "none", execute: commands.toggleEdit }]],
  ["KeyO", [{ modifiers: "primary", execute: commands.loadFile }]],
  ["KeyB", [{ modifiers: "primary", execute: commands.exportAudio }]],
  ["Space", [{ modifiers: "none", execute: commands.togglePlay }]],
  ["KeyL", [{ modifiers: "primary", execute: commands.toggleLoop }]],
  [
    "KeyZ",
    [
      { modifiers: "primary", execute: commands.undoEdit },
      { modifiers: "primary+secondary", execute: commands.redoEdit },
    ],
  ],
  ["ArrowUp", [{ modifiers: "none", execute: commands.patternGridUp }]],
  ["ArrowDown", [{ modifiers: "none", execute: commands.patternGridDown }]],
  [
    "PageDown",
    [{ modifiers: "none", execute: commands.patternGridStrideDown }],
  ],
  [
    "PageUp",
    [{ modifiers: "none", execute: commands.patternGridStrideUp }],
  ],
  ["Home", [{ modifiers: "none", execute: commands.patternGridJumpToTop }]],
  ["End", [{ modifiers: "none", execute: commands.patternGridJumpToBottom }]],
  [
    "ArrowRight",
    [
      { modifiers: "none", execute: commands.cursorFieldRight },
      { modifiers: "primary", execute: commands.sequenceSeekForwards },
      { modifiers: "tertiary", execute: commands.patternGridRight },
    ],
  ],
  [
    "ArrowLeft",
    [
      { modifiers: "none", execute: commands.cursorFieldLeft },
      { modifiers: "primary", execute: commands.sequenceSeekBackwards },
      { modifiers: "tertiary", execute: commands.patternGridLeft },
    ],
  ],
  [
    "Tab",
    [
      { modifiers: "none", execute: commands.patternGridRight },
      { modifiers: "secondary", execute: commands.patternGridLeft },
    ],
  ],
  [
    "BracketRight",
    [{ modifiers: "secondary", execute: commands.increaseEffectsDisplayed }],
  ],
  [
    "BracketLeft",
    [{ modifiers: "secondary", execute: commands.decreaseEffectsDisplayed }],
  ],
  ["Delete", [{ modifiers: "none", execute: commands.clearPatternEvent }]],
  [
    "Backspace",
    [{ modifiers: "none", execute: commands.clearPatternEventField }],
  ],
]);

function getKeyBindings(key: string): KeyBinding[] {
  return keyBindings.get(key) || [];
}

function modifiersMatch(e: KeyboardEvent, modifier: ModifierRequirement) {
  switch (modifier) {
    case "none":
      return !primaryModifier(e) && !e.shiftKey && !e.altKey;
    case "primary":
      return primaryModifier(e) && !e.shiftKey && !e.altKey;
    case "secondary":
      return !primaryModifier(e) && e.shiftKey && !e.altKey;
    case "tertiary":
      return !primaryModifier(e) && !e.shiftKey && e.altKey;
    case "primary+secondary":
      return primaryModifier(e) && e.shiftKey && !e.altKey;
  }
}

export const keyBinding: { handleKey: KeyHandler } = {
  handleKey: (e) => {
    console.log("handleKey", e.code);
    const keyBinding = getKeyBindings(e.code).find((binding) =>
      modifiersMatch(e, binding.modifiers),
    );
    if (keyBinding) {
      keyBinding.execute();
      return true;
    }
    return false;
  },
};
