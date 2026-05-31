import { selection } from "./selection.ts";
import { useStore } from "../store/useStore.ts";
import { cursor } from "./cursor.ts";
import { PasteBufferObjectType } from "./pasteBuffer.ts";
import { engine, PatternEvent } from "../engine/engine.ts";
import { EventLocation, patternEvents } from "./patternEvents.ts";

export const copyPaste = {
  copyPatternEvents: async () => {
    const cursorPosition = cursor.currentPosition();
    const patternSelectionBounds = selection.patternSelectionBounds() || {
      top: cursorPosition.patternIndex,
      bottom: cursorPosition.patternIndex,
      left: cursorPosition.track,
      right: cursorPosition.track,
    };
    const { transportState } = useStore.getState();
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
    let pasteBuffer: PasteBufferObjectType = {
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
      const bufferLine = line - patternSelectionBounds.top;
      pasteBuffer.block.events[bufferLine] = [];
      for (
        let track = patternSelectionBounds.left;
        track <= patternSelectionBounds.right && track < numChannels;
        track++
      ) {
        const bufferTrack = track - patternSelectionBounds.left;
        pasteBuffer.block.events[bufferLine][bufferTrack] =
          await engine.getEvent(transportState.patternNo, line, track);
      }
    }
    useStore.getState().setPasteBuffer(pasteBuffer);
  },

  pastePatternEvents: async () => {
    const pasteBuffer = useStore.getState().pasteBuffer;
    if (!pasteBuffer || pasteBuffer.type !== "patternEvents") return;
    const currentPattern = useStore.getState().currentPattern;
    const cursorPosition = cursor.currentPosition();
    const numChannels = useStore.getState().module.numChannels;
    const blockWidth = Math.min(
      pasteBuffer.block.width,
      numChannels - cursorPosition.track,
    );
    const blockHeight = Math.min(
      pasteBuffer.block.height,
      currentPattern.lines.length - cursorPosition.patternIndex,
    );
    const pastedEvents: { location: EventLocation; event: PatternEvent }[] = [];
    for (let line = 0; line < blockHeight; line++) {
      const destLine = cursorPosition.patternIndex + line;
      for (let track = 0; track < blockWidth; track++) {
        const destTrack = cursorPosition.track + track;
        pastedEvents.push({
          location: {
            patternNo: useStore.getState().transportState.patternNo,
            patternIndex: destLine,
            track: destTrack,
          },
          event: pasteBuffer.block.events[line][track],
        });
      }
    }
    await patternEvents.setEvents(pastedEvents);
  },
};
