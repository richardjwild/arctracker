import { commands } from "../control/commands.ts";
import { KeyHandler } from "./keyHandler.ts";

export function primaryModifier(e: KeyboardEvent) {
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
  | "primary+tertiary"
  | "secondary+tertiary";

type KeyBinding = {
  modifiers: ModifierRequirement;
  execute: () => void;
}

const keyBindings = new Map<string, KeyBinding[]>([
  ["Escape", [{ modifiers: "none", execute: commands.toggleEdit }]],
  [
    "ArrowUp",
    [
      { modifiers: "none", execute: () => commands.patternGridUp(false) },
      { modifiers: "secondary", execute: () => commands.patternGridUp(true) },
      { modifiers: "tertiary", execute: commands.previousInstrument },
      { modifiers: "primary+tertiary", execute: commands.firstInstrument },
    ],
  ],
  [
    "ArrowDown",
    [
      { modifiers: "none", execute: () => commands.patternGridDown(false) },
      { modifiers: "secondary", execute: () => commands.patternGridDown(true) },
      { modifiers: "tertiary", execute: commands.nextInstrument },
      { modifiers: "primary+tertiary", execute: commands.lastInstrument },
    ],
  ],
  [
    "ArrowLeft",
    [
      { modifiers: "none", execute: commands.cursorFieldLeft },
      { modifiers: "secondary", execute: () => commands.patternGridLeft(true) },
      { modifiers: "tertiary", execute: commands.sequenceSeekBackwards },
      { modifiers: "primary+tertiary", execute: commands.sequenceSeekToStart },
    ],
  ],
  [
    "ArrowRight",
    [
      { modifiers: "none", execute: commands.cursorFieldRight },
      {
        modifiers: "secondary",
        execute: () => commands.patternGridRight(true),
      },
      { modifiers: "tertiary", execute: commands.sequenceSeekForwards },
      { modifiers: "primary+tertiary", execute: commands.sequenceSeekToEnd },
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
  [
    "Space",
    [
      { modifiers: "none", execute: commands.togglePlay },
      { modifiers: "primary", execute: commands.toggleLoop },
    ]
  ],
  ["KeyB", [{ modifiers: "primary", execute: commands.exportAudio }]],
  ["KeyC", [{ modifiers: "primary", execute: commands.copyPatternEvents }]],
  ["KeyI", [{ modifiers: "primary", execute: commands.openInstrumentEditor }]],
  ["KeyL", [{ modifiers: "primary", execute: () => commands.setCurrentPatternLength(128) }]],
  ["KeyO", [{ modifiers: "primary", execute: commands.loadFile }]],
  ["KeyV", [{ modifiers: "primary", execute: commands.pastePatternEvents }]],
  ["KeyX", [{ modifiers: "primary", execute: commands.cutPatternEvents }]],
  [
    "KeyZ",
    [
      { modifiers: "primary", execute: commands.undoEdit },
      { modifiers: "primary+secondary", execute: commands.redoEdit },
    ],
  ],
  [
    "F1",
    [
      { modifiers: "none", execute: commands.insertSequencePositionBefore },
      {
        modifiers: "secondary",
        execute: () => commands.insertSequencePositionBefore(true),
      },
    ],
  ],
  [
    "F2",
    [
      { modifiers: "none", execute: commands.insertSequencePositionAfter },
      {
        modifiers: "secondary",
        execute: () => commands.insertSequencePositionAfter(true),
      },
    ],
  ],
  [
    "F3",
    [
      { modifiers: "primary", execute: commands.cutPattern },
      { modifiers: "secondary", execute: commands.cutTrack },
    ],
  ],
  [
    "F4",
    [
      { modifiers: "primary", execute: commands.copyPattern },
      { modifiers: "secondary", execute: commands.copyTrack },
    ],
  ],
  [
    "F5",
    [
      { modifiers: "primary", execute: commands.pastePattern },
      { modifiers: "secondary", execute: commands.pasteTrack },
    ],
  ],
  [
    "Equal",
    [
      {
        modifiers: "secondary",
        execute: commands.incrementPatternAtCurrentPosition,
      },
    ],
  ],
  [
    "Minus",
    [
      {
        modifiers: "secondary",
        execute: commands.decrementPatternAtCurrentPosition,
      },
    ],
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
    case "primary+tertiary":
      return primaryModifier(e) && !e.shiftKey && e.altKey;
    case "secondary+tertiary":
      return !primaryModifier(e) && e.shiftKey && e.altKey;
  }
}

export const keyBinding: { handleKey: KeyHandler } = {
  handleKey: (e) => {
    // console.log("handleKey", e.code);
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
