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
  | "primary+secondary"
  | "secondary+tertiary";

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
  [
    "ArrowUp",
    [
      { modifiers: "none", execute: () => commands.patternGridUp(false) },
      { modifiers: "secondary", execute: () => commands.patternGridUp(true) },
    ],
  ],
  [
    "ArrowDown",
    [
      { modifiers: "none", execute: () => commands.patternGridDown(false) },
      { modifiers: "secondary", execute: () => commands.patternGridDown(true) },
    ],
  ],
  [
    "PageDown",
    [
      {
        modifiers: "none",
        execute: () => commands.patternGridStrideDown(false),
      },
      {
        modifiers: "secondary",
        execute: () => commands.patternGridStrideDown(true),
      },
    ],
  ],
  [
    "PageUp",
    [
      { modifiers: "none", execute: () => commands.patternGridStrideUp(false) },
      {
        modifiers: "secondary",
        execute: () => commands.patternGridStrideUp(true),
      },
    ],
  ],
  [
    "Home",
    [
      {
        modifiers: "none",
        execute: () => commands.patternGridJumpToTop(false),
      },
      {
        modifiers: "secondary",
        execute: () => commands.patternGridJumpToTop(true),
      },
    ],
  ],
  [
    "End",
    [
      {
        modifiers: "none",
        execute: () => commands.patternGridJumpToBottom(false),
      },
      {
        modifiers: "secondary",
        execute: () => commands.patternGridJumpToBottom(true),
      },
    ],
  ],
  [
    "ArrowRight",
    [
      { modifiers: "none", execute: commands.cursorFieldRight },
      { modifiers: "primary", execute: commands.sequenceSeekForwards },
      {
        modifiers: "secondary",
        execute: () => commands.patternGridRight(true),
      },
    ],
  ],
  [
    "ArrowLeft",
    [
      { modifiers: "none", execute: commands.cursorFieldLeft },
      { modifiers: "primary", execute: commands.sequenceSeekBackwards },
      { modifiers: "secondary", execute: () => commands.patternGridLeft(true) },
    ],
  ],
  [
    "Tab",
    [
      { modifiers: "none", execute: () => commands.patternGridRight(false) },
      {
        modifiers: "secondary",
        execute: () => commands.patternGridLeft(false),
      },
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
  ["KeyX", [{ modifiers: "primary", execute: commands.cutPatternEvents }]],
  ["KeyC", [{ modifiers: "primary", execute: commands.copyPatternEvents }]],
  ["KeyV", [{ modifiers: "primary", execute: commands.pastePatternEvents }]],
  ["F4", [{ modifiers: "secondary", execute: commands.copyTrack }]],
  ["F5", [{ modifiers: "secondary", execute: commands.pasteTrack }]],
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
    case "secondary+tertiary":
      return !primaryModifier(e) && e.shiftKey && e.altKey;
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
