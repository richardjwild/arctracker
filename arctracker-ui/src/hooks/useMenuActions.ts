import { commands } from "../control/commands.ts";
import { useEffect } from "react";
import { listen, UnlistenFn } from "@tauri-apps/api/event";

type MenuAction = {
  eventId: string;
  action: () => void;
};

const menuActions: MenuAction[] = [
  { eventId: "new-module-requested", action: commands.createModule },
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
  { eventId: "undo-requested", action: commands.undoEdit },
  { eventId: "redo-requested", action: commands.redoEdit },
  { eventId: "cut-events-requested", action: commands.cutPatternEvents },
  { eventId: "copy-events-requested", action: commands.copyPatternEvents },
  { eventId: "paste-events-requested", action: commands.pastePatternEvents },
  { eventId: "cut-track-requested", action: commands.cutTrack },
  { eventId: "copy-track-requested", action: commands.copyTrack },
  { eventId: "paste-track-requested", action: commands.pasteTrack },
  { eventId: "cut-pattern-requested", action: commands.cutPattern },
  { eventId: "copy-pattern-requested", action: commands.copyPattern },
  { eventId: "paste-pattern-requested", action: commands.pastePattern },
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
