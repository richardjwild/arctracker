import { commands } from "../control/commands.ts";
import { useEffect } from "react";
import { listen, UnlistenFn } from "@tauri-apps/api/event";

type MenuAction = {
  eventId: string;
  action: () => void;
};

const menuActions: MenuAction[] = [
  { eventId: "open-settings-requested", action: commands.editAppConfig },
  { eventId: "new-module-requested", action: commands.createModule },
  {
    eventId: "new-module-using-defaults-requested",
    action: commands.createModuleUsingDefaults,
  },
  { eventId: "open-module-requested", action: commands.loadFile },
  { eventId: "save-module-requested", action: commands.saveModule },
  { eventId: "save-module-as-requested", action: commands.saveModuleAs },
  { eventId: "export-audio-requested", action: commands.exportAudio },
  {
    eventId: "edit-module-details-requested",
    action: commands.editModuleTitle,
  },
  { eventId: "set-track-count-requested", action: commands.editTrackCount },
  { eventId: "set-tempo-requested", action: commands.editTempo },
  { eventId: "toggle-mute-requested", action: commands.toggleCurrentTrackMute },
  { eventId: "increase-effects-displayed-requested", action: commands.increaseEffectsDisplayed },
  { eventId: "decrease-effects-displayed-requested", action: commands.decreaseEffectsDisplayed },
  { eventId: "undo-requested", action: commands.undoEdit },
  { eventId: "redo-requested", action: commands.redoEdit },
  { eventId: "toggle-edit-requested", action: commands.toggleEdit },
  { eventId: "clear-event-requested", action: commands.clearPatternEvent },
  { eventId: "clear-field-requested", action: commands.clearPatternEventField },
  { eventId: "cut-events-requested", action: commands.cutPatternEvents },
  { eventId: "copy-events-requested", action: commands.copyPatternEvents },
  { eventId: "paste-events-requested", action: commands.pastePatternEvents },
  { eventId: "cut-track-requested", action: commands.cutTrack },
  { eventId: "copy-track-requested", action: commands.copyTrack },
  { eventId: "paste-track-requested", action: commands.pasteTrack },
  { eventId: "cut-pattern-requested", action: commands.cutPattern },
  { eventId: "copy-pattern-requested", action: commands.copyPattern },
  { eventId: "paste-pattern-requested", action: commands.pastePattern },
  { eventId: "play-pause-requested", action: commands.togglePlay },
  { eventId: "toggle-loop-requested", action: commands.toggleLoop },
  { eventId: "seek-forwards-requested", action: commands.sequenceSeekForwards },
  { eventId: "seek-backwards-requested", action: commands.sequenceSeekBackwards },
  { eventId: "seek-to-start-requested", action: commands.sequenceSeekToStart },
  { eventId: "seek-to-end-requested", action: commands.sequenceSeekToEnd },
  { eventId: "increment-pattern-requested", action: commands.incrementPatternAtCurrentPosition },
  { eventId: "decrement-pattern-requested", action: commands.decrementPatternAtCurrentPosition },
  {
    eventId: "insert-sequence-before-requested",
    action: commands.insertSequencePositionBefore,
  },
  {
    eventId: "insert-sequence-after-requested",
    action: commands.insertSequencePositionAfter,
  },
  {
    eventId: "insert-sequence-before-with-new-requested",
    action: () => commands.insertSequencePositionBefore(true),
  },
  {
    eventId: "insert-sequence-after-with-new-requested",
    action: () => commands.insertSequencePositionAfter(true),
  },
  {
    eventId: "delete-sequence-position-requested",
    action: commands.deleteSequencePosition,
  },
  {
    eventId: "set-pattern-length-requested",
    action: commands.editCurrentPatternLength,
  },
  { eventId: "next-instrument-requested", action: commands.nextInstrument },
  { eventId: "previous-instrument-requested", action: commands.previousInstrument },
  { eventId: "first-instrument-requested", action: commands.firstInstrument },
  { eventId: "last-instrument-requested", action: commands.lastInstrument },
  { eventId: "add-instrument-requested", action: commands.addInstrument },
  {
    eventId: "edit-instrument-requested",
    action: commands.openInstrumentEditor,
  },
  { eventId: "load-sample-requested", action: commands.loadSample },
  { eventId: "delete-sample-requested", action: commands.deleteSample },
  { eventId: "export-sample-requested", action: commands.exportSample },
];

export function useMenuActions() {
  useEffect(() => {
    const unlisteners: UnlistenFn[] = [];
    let disposed = false;
    void Promise.all(
      menuActions.map(async ({ eventId, action }) => {
        const unlisten = await listen(eventId, () => {
          void Promise.resolve(action()).catch((error) => {
            console.error(`Menu action failed: ${eventId}`, error);
          });
        });
        if (disposed) {
          unlisten();
        } else {
          unlisteners.push(unlisten);
        }
      }),
    ).catch((error) => {
      console.error("Failed to install menu listeners", error);
    });
    return () => {
      disposed = true;
      unlisteners.forEach((unlisten) => unlisten());
    };
  }, []);
}
