import { PatternSelectionBounds, selection } from "./selection.ts";
import { useStore } from "../store/useStore.ts";
import { cursor } from "./cursor.ts";
import { PasteBufferObjectType } from "./pasteBuffer.ts";
import { engine, PatternEvent } from "../engine/engine.ts";
import { EventLocation, patternEvents } from "./patternEvents.ts";
import { patternGrid, PatternGridPosition } from "./patternGrid.ts";

function getSelectionBounds(
  providedBounds: PatternSelectionBounds | null,
): PatternSelectionBounds {
  if (providedBounds) return providedBounds;
  const selectedBounds = selection.patternSelectionBounds();
  if (selectedBounds) return selectedBounds;
  const cursorPosition = cursor.currentPosition();
  return {
    top: cursorPosition.patternIndex,
    bottom: cursorPosition.patternIndex,
    left: cursorPosition.track,
    right: cursorPosition.track,
  };
}

export const copyPaste = {
  copyPatternEvents: async (
    providedBounds: PatternSelectionBounds | null,
  ): Promise<EventLocation[]> => {
    const selectionBounds = getSelectionBounds(providedBounds);
    const { transportState } = useStore.getState();
    const currentPattern = useStore.getState().currentPattern;
    const numChannels = useStore.getState().module.numChannels;
    const blockHeight = Math.min(
      selectionBounds.bottom - selectionBounds.top + 1,
      currentPattern.lines.length - selectionBounds.top,
    );
    const blockWidth = Math.min(
      selectionBounds.right - selectionBounds.left + 1,
      numChannels - selectionBounds.left,
    );
    if (blockHeight === 0 || blockWidth === 0) return []; // Shouldn't really be possible, but belt & braces.
    let pasteBuffer: PasteBufferObjectType = {
      type: "patternEvents",
      block: {
        width: blockWidth,
        height: blockHeight,
        events: [],
      },
    };
    let cutLocations: EventLocation[] = [];
    for (
      let patternIndex = selectionBounds.top;
      patternIndex <= selectionBounds.bottom &&
      patternIndex < currentPattern.lines.length;
      patternIndex++
    ) {
      const bufferLine = patternIndex - selectionBounds.top;
      pasteBuffer.block.events[bufferLine] = [];
      for (
        let track = selectionBounds.left;
        track <= selectionBounds.right && track < numChannels;
        track++
      ) {
        const bufferTrack = track - selectionBounds.left;
        pasteBuffer.block.events[bufferLine][bufferTrack] =
          await engine.getEvent(transportState.patternNo, patternIndex, track);
        cutLocations.push({
          patternNo: transportState.patternNo,
          patternIndex: patternIndex,
          track,
        });
      }
    }
    useStore.getState().setPasteBuffer(pasteBuffer);
    return cutLocations;
  },

  cutPatternEvents: async () => {
    const cutLocations = await copyPaste.copyPatternEvents(null);
    if (cutLocations.length > 0) await patternEvents.clearEvents(cutLocations);
  },

  pastePatternEvents: async (pasteLocation: PatternGridPosition | null) => {
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
      const destLine =
        (pasteLocation?.patternIndex ?? cursorPosition.patternIndex) + line;
      for (let track = 0; track < blockWidth; track++) {
        const destTrack =
          (pasteLocation?.track ?? cursorPosition.track) + track;
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
    if (!pasteLocation) {
      patternGrid.moveTo({
        track: cursorPosition.track,
        patternIndex: cursorPosition.patternIndex + blockHeight - 1,
      });
    }
  },

  copyTrack: async (): Promise<EventLocation[]> => {
    const track = patternGrid.currentPosition().track;
    const patternLength = useStore.getState().currentPattern.lines.length;
    return await copyPaste.copyPatternEvents({
      top: 0,
      bottom: patternLength - 1,
      left: track,
      right: track,
    });
  },

  cutTrack: async () => {
    const cutLocations = await copyPaste.copyTrack();
    await patternEvents.clearEvents(cutLocations);
  },

  pasteTrack: async () => {
    const track = patternGrid.currentPosition().track;
    await copyPaste.pastePatternEvents({ track, patternIndex: 0 });
  },

  copyPattern: async (): Promise<EventLocation[]> => {
    const numChannels = useStore.getState().module.numChannels;
    const patternLength = useStore.getState().currentPattern.lines.length;
    return await copyPaste.copyPatternEvents({
      top: 0,
      bottom: patternLength - 1,
      left: 0,
      right: numChannels - 1,
    });
  },

  cutPattern: async () => {
    const cutLocations = await copyPaste.copyPattern();
    await patternEvents.clearEvents(cutLocations);
  },

  pastePattern: async () => {
    await copyPaste.pastePatternEvents({ track: 0, patternIndex: 0 });
  }
};
