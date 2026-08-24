import { PatternSelectionBounds, selection } from "./selection.ts";
import { useStore } from "../store/useStore.ts";
import { cursor } from "./cursor.ts";
import { PasteBufferObjectType } from "./pasteBuffer.ts";
import { EventLocation, PatternEvent, patternEvents } from "./patternEvents.ts";
import { patternGrid, PatternGridPosition } from "./patternGrid.ts";
import { userMessages } from "../messages/userMessages.ts";
import { messageFn, messageFn2 } from "../language/messages.ts";

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

function getPasteLocation(
  providedLocation: PatternGridPosition | null,
): PatternGridPosition {
  if (providedLocation) return providedLocation;
  const cursorPosition = cursor.currentPosition();
  return {
    track: cursorPosition.track,
    patternIndex: cursorPosition.patternIndex,
  };
}

function getFocusPatternNo() {
  const { sequence } = useStore.getState();
  const { sequencePosition } = useStore.getState().editorState;
  return sequence[sequencePosition];
}

async function copyEvents(providedBounds: PatternSelectionBounds | null): Promise<EventLocation[]> {
  const selectionBounds = getSelectionBounds(providedBounds);
  const patternNo = getFocusPatternNo();
  const currentPattern = useStore.getState().currentPattern;
  const numTracks = useStore.getState().module.numTracks;
  const blockHeight = Math.min(
    selectionBounds.bottom - selectionBounds.top + 1,
    currentPattern.lines.length - selectionBounds.top,
  );
  const blockWidth = Math.min(
    selectionBounds.right - selectionBounds.left + 1,
    numTracks - selectionBounds.left,
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
      track <= selectionBounds.right && track < numTracks;
      track++
    ) {
      const bufferTrack = track - selectionBounds.left;
      pasteBuffer.block.events[bufferLine][bufferTrack] =
        await patternEvents.getEvent(patternNo, patternIndex, track);
      cutLocations.push({
        patternNo,
        patternIndex,
        track,
      });
    }
  }
  useStore.getState().setPasteBuffer(pasteBuffer);
  return cutLocations;
}

export const copyPaste = {
  copyPatternEvents: async (
    providedBounds: PatternSelectionBounds | null,
  ): Promise<EventLocation[]> => {
    const cutLocations: EventLocation[] = await copyEvents(providedBounds);
    userMessages.logMessage({
      type: "info",
      message: messageFn("copiedEvents")(cutLocations.length.toString()),
    });
    return cutLocations;
  },

  cutPatternEvents: async () => {
    const cutLocations = await copyEvents(null);
    if (cutLocations.length > 0) await patternEvents.clearEvents(cutLocations);
  },

  pastePatternEvents: async (providedLocation: PatternGridPosition | null) => {
    const pasteBuffer = useStore.getState().pasteBuffer;
    if (!pasteBuffer || pasteBuffer.type !== "patternEvents") return;
    const pasteLocation = getPasteLocation(providedLocation);
    const currentPattern = useStore.getState().currentPattern;
    const cursorPosition = cursor.currentPosition();
    const numTracks = useStore.getState().module.numTracks;
    const blockWidth = Math.min(
      pasteBuffer.block.width,
      numTracks - pasteLocation.track,
    );
    const blockHeight = Math.min(
      pasteBuffer.block.height,
      currentPattern.lines.length - pasteLocation.patternIndex,
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
            patternNo: getFocusPatternNo(),
            patternIndex: destLine,
            track: destTrack,
          },
          event: pasteBuffer.block.events[line][track],
        });
      }
    }
    await patternEvents.setEvents(pastedEvents);
    if (!providedLocation) {
      patternGrid.moveTo({
        track: cursorPosition.track,
        patternIndex: cursorPosition.patternIndex + blockHeight - 1,
      });
    }
  },

  copyTrack: async (): Promise<EventLocation[]> => {
    const track = patternGrid.currentPosition().track;
    const currentPattern = useStore.getState().currentPattern;
    const patternLength = currentPattern.lines.length;
    const copiedEventLocations = await copyEvents({
      top: 0,
      bottom: patternLength - 1,
      left: track,
      right: track,
    });
    userMessages.logMessage({
      type: "info",
      message: messageFn2("copiedTrack")(
        (track + 1).toString(),
        currentPattern.patternNo.toString(),
      ),
    });
    return copiedEventLocations;
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
    const numTracks = useStore.getState().module.numTracks;
    const currentPattern = useStore.getState().currentPattern;
    const patternLength = currentPattern.lines.length;
    const copiedEventLocations = await copyEvents({
      top: 0,
      bottom: patternLength - 1,
      left: 0,
      right: numTracks - 1,
    });
    userMessages.logMessage({
      type: "info",
      message: messageFn("copiedPattern")(currentPattern.patternNo.toString()),
    });
    return copiedEventLocations;
  },

  cutPattern: async () => {
    const cutLocations = await copyPaste.copyPattern();
    await patternEvents.clearEvents(cutLocations);
  },

  pastePattern: async () => {
    await copyPaste.pastePatternEvents({ track: 0, patternIndex: 0 });
  },
};
