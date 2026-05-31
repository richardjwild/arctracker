import { selection } from "./selection.ts";
import { useStore } from "../store/useStore.ts";
import { cursor } from "./cursor.ts";
import { PasteBufferObjectType } from "./pasteBuffer.ts";

export const copyPaste = {
  copyPatternEvents: () => {
    const cursorPosition = cursor.currentPosition();
    const patternSelectionBounds = selection.patternSelectionBounds() || {
      top: cursorPosition.patternIndex,
      bottom: cursorPosition.patternIndex,
      left: cursorPosition.track,
      right: cursorPosition.track,
    };
    const currentPattern = useStore.getState().currentPattern;
    const numChannels = useStore.getState().module.numChannels;
    const blockHeight = Math.min(
      patternSelectionBounds.bottom - patternSelectionBounds.top + 1,
      currentPattern.lines.length - patternSelectionBounds.top,
    );
    const blockWidth = Math.min(
      patternSelectionBounds.right - patternSelectionBounds.left + 1,
      numChannels - patternSelectionBounds.left,
    );
    if (blockHeight === 0 || blockWidth === 0) return; // Shouldn't really be possible, but belt & braces.
    let pasteBufferObject: PasteBufferObjectType = {
      type: "patternEvents",
      block: {
        width: blockWidth,
        height: blockHeight,
        events: [],
      },
    };
    for (
      let line = patternSelectionBounds.top;
      line <= patternSelectionBounds.bottom &&
      line < currentPattern.lines.length;
      line++
    ) {
      const destLine = line - patternSelectionBounds.top;
      pasteBufferObject.block.events[destLine] = [];
      for (
        let track = patternSelectionBounds.left;
        track <= patternSelectionBounds.right && track < numChannels;
        track++
      ) {
        const destTrack = track - patternSelectionBounds.left;
        pasteBufferObject.block.events[destLine][destTrack] =
          structuredClone(currentPattern.lines[line].events[track]);
      }
    }
    useStore.getState().setPasteBuffer(pasteBufferObject);
  },
};
