import { commands } from "../control/commands.ts";
import { useEffect } from "react";
import { Menu } from "@tauri-apps/api/menu";
import { platform } from "@tauri-apps/plugin-os";
import exitArctracker from "../exit/exit.ts";

const appMenu = {
  id: "app-menu",
  text: "Arctracker",
  items: [
    {
      id: "app-about",
      text: "About Arctracker",
      enabled: true,
      action: () => alert("Arctracker is amazing!"),
    },
    { item: "Separator" },
    {
      id: "app-settings",
      text: "Settings...",
      enabled: true,
      action: commands.editAppConfig,
    },
    {
      id: "app-command-reference",
      text: "Command Reference...",
      enabled: true,
      action: commands.showCommandReference,
    },
    { item: "Separator" },
    {
      id: "app-exit",
      text: "Quit",
      enabled: true,
      accelerator: platform() === "macos" ? "Cmd+Q" : undefined,
      action: exitArctracker,
    },
  ],
};

const fileMenu = {
  id: "file-menu",
  text: "File",
  items: [
    {
      id: "file-new",
      text: "New Module",
      enabled: true,
      action: commands.createModuleUsingDefaults,
    },
    { item: "Separator" },
    {
      id: "file-open-module",
      text: "Open Module...",
      enabled: true,
      action: commands.loadFile,
    },
    {
      id: "file-save-module",
      text: "Save Module",
      enabled: true,
      action: commands.saveModule,
    },
    {
      id: "file-save-module-as",
      text: "Save Module As...",
      enabled: true,
      action: commands.saveModuleAs,
    },
    { item: "Separator" },
    {
      id: "file-export-audio",
      text: "Export Audio...",
      enabled: true,
      action: commands.exportAudio,
    },
  ],
}

const editMenu = {
  id: "edit-menu",
  text: "Edit",
  items: [
    {
      id: "edit-undo",
      text: "Undo",
      enabled: true,
      action: commands.undoEdit,
    },
    {
      id: "edit-redo",
      text: "Redo",
      enabled: true,
      action: commands.redoEdit,
    },
    { item: "Separator" },
    {
      id: "edit-toggle",
      text: "Toggle Edit Mode",
      enabled: true,
      action: commands.toggleEdit,
    },
    { item: "Separator" },
    {
      id: "edit-clear-event",
      text: "Clear Event",
      enabled: true,
      action: commands.clearPatternEvent,
    },
    {
      id: "edit-clear-event",
      text: "Clear Field",
      enabled: true,
      action: commands.clearPatternEventField,
    },
    { item: "Separator" },
    {
      id: "edit-cut-events",
      text: "Cut Events",
      enabled: true,
      action: commands.cutPatternEvents,
    },
    {
      id: "edit-copy-events",
      text: "Copy Events",
      enabled: true,
      action: commands.copyPatternEvents,
    },
    {
      id: "edit-paste-events",
      text: "Paste Events",
      enabled: true,
      action: commands.pastePatternEvents,
    },
    {
      id: "edit-duplicate-events",
      text: "Duplicate Events",
      enabled: true,
      action: commands.duplicatePatternEvents,
    },
    { item: "Separator" },
    {
      id: "edit-cut-track",
      text: "Cut Track",
      enabled: true,
      action: commands.cutTrack,
    },
    {
      id: "edit-copy-track",
      text: "Copy Track",
      enabled: true,
      action: commands.copyTrack,
    },
    {
      id: "edit-paste-track",
      text: "Paste Track",
      enabled: true,
      action: commands.pasteTrack,
    },
    { item: "Separator" },
    {
      id: "edit-cut-pattern",
      text: "Cut Pattern",
      enabled: true,
      action: commands.cutPattern,
    },
    {
      id: "edit-copy-pattern",
      text: "Copy Pattern",
      enabled: true,
      action: commands.copyPattern,
    },
    {
      id: "edit-paste-pattern",
      text: "Paste Pattern",
      enabled: true,
      action: commands.pastePattern,
    },
    { item: "Separator" },
    {
      id: "edit-set-multiple-effects",
      text: "Set Multiple Effects",
      enabled: true,
      action: commands.openSetMultipleEffects,
    },
  ],
};

const transportMenu = {
  id: "transport-menu",
  text: "Transport",
  items: [
    {
      id: "transport-toggle-play",
      text: "Toggle Play/Pause",
      enabled: true,
      action: commands.togglePlay,
    },
    {
      id: "transport-toggle-pattern-loop",
      text: "Toggle Pattern Loop",
      enabled: true,
      action: commands.toggleLoop,
    },
    { item: "Separator" },
    {
      id: "transport-seek-forwards",
      text: "Seek Forwards",
      enabled: true,
      action: commands.sequenceSeekForwards,
    },
    {
      id: "transport-seek-backwards",
      text: "Seek Backwards",
      enabled: true,
      action: commands.sequenceSeekBackwards,
    },
    {
      id: "transport-seek-to-start",
      text: "Seek To Start",
      enabled: true,
      action: commands.sequenceSeekToStart,
    },
    {
      id: "transport-seek-to-end",
      text: "Seek To End",
      enabled: true,
      action: commands.sequenceSeekToEnd,
    },
    {
      id: "transport-seek-to-position",
      text: "Seek To Position...",
      enabled: true,
      action: commands.openSequenceSeekDialog,
    },
  ],
};

const moduleMenu = {
  id: "module-menu",
  text: "Module",
  items: [
    {
      id: "module-edit-details",
      text: "Edit Module Details...",
      enabled: true,
      action: commands.editModuleMetaData,
    },
    {
      id: "module-set-track-count",
      text: "Set Track Count...",
      enabled: true,
      action: commands.editTrackCount,
    },
    {
      id: "module-edit-tempo",
      text: "Edit Tempo...",
      enabled: true,
      action: commands.editTempo,
    },
  ],
};

const sequenceMenu = {
  id: "sequence-menu",
  text: "Sequence",
  items: [
    {
      id: "sequence-increment-pattern",
      text: "Increment Pattern",
      enabled: true,
      action: commands.incrementPatternAtCurrentPosition,
    },
    {
      id: "sequence-decrement-pattern",
      text: "Decrement Pattern",
      enabled: true,
      action: commands.decrementPatternAtCurrentPosition,
    },
    { item: "Separator" },
    {
      id: "sequence-insert-before",
      text: "Insert Before",
      enabled: true,
      action: () => commands.insertSequencePositionBefore(false),
    },
    {
      id: "sequence-insert-before-new",
      text: "Insert Before With New Pattern",
      enabled: true,
      action: () => commands.insertSequencePositionBefore(true),
    },
    {
      id: "sequence-insert-after",
      text: "Insert After",
      enabled: true,
      action: () => commands.insertSequencePositionAfter(false),
    },
    {
      id: "sequence-insert-after-new",
      text: "Insert After With New Pattern",
      enabled: true,
      action: () => commands.insertSequencePositionAfter(true),
    },
    { item: "Separator" },
    {
      id: "sequence-delete-position",
      text: "Delete Current Position",
      enabled: true,
      action: commands.deleteSequencePosition,
    },
  ],
};

const trackMenu = {
  id: "track-menu",
  text: "Track",
  items: [
    {
      id: "track-toggle-mute",
      text: "Toggle Track Mute",
      enabled: true,
      action: commands.toggleCurrentTrackMute,
    },
    { item: "Separator" },
    {
      id: "track-increase-effects-displayed",
      text: "Increase Effects Displayed",
      enabled: true,
      action: commands.increaseEffectsDisplayed,
    },
    {
      id: "track-decrease-effects-displayed",
      text: "Decrease Effects Displayed",
      enabled: true,
      action: commands.decreaseEffectsDisplayed,
    },
  ],
};

const patternMenu = {
  id: "pattern-menu",
  text: "Pattern",
  items: [
    {
      id: "pattern-create",
      text: "New Pattern",
      enabled: true,
      action: commands.createPatternAtCurrentPosition,
    },
    {
      id: "pattern-set-length",
      text: "Set Length...",
      enabled: true,
      action: commands.editCurrentPatternLength,
    },
  ],
};

const instrumentMenu = {
  id: "instrument-menu",
  text: "Instrument",
  items: [
    {
      id: "instrument-next",
      text: "Next",
      enabled: true,
      action: commands.nextInstrument,
    },
    {
      id: "instrument-previous",
      text: "Previous",
      enabled: true,
      action: commands.previousInstrument,
    },
    {
      id: "instrument-first",
      text: "First",
      enabled: true,
      action: commands.firstInstrument,
    },
    {
      id: "instrument-last",
      text: "Last",
      enabled: true,
      action: commands.lastInstrument,
    },
    { item: "Separator" },
    {
      id: "instrument-add",
      text: "Add Instrument...",
      enabled: true,
      action: commands.addInstrument,
    },
    {
      id: "instrument-edit",
      text: "Edit Instrument...",
      enabled: true,
      action: commands.openInstrumentEditor,
    },
    { item: "Separator" },
    {
      id: "instrument-load-sample",
      text: "Load Sample...",
      enabled: true,
      action: commands.loadSample,
    },
    {
      id: "instrument-delete-sample",
      text: "Delete Sample...",
      enabled: true,
      action: commands.deleteSample,
    },
    {
      id: "instrument-export-sample",
      text: "Export Sample...",
      enabled: true,
      action: commands.exportSample,
    },
  ],
};

let menuInitialised = false;

export function useMainMenu() {
  useEffect(() => {
    if (menuInitialised) return;
    Menu.new({
      items: [
        appMenu,
        fileMenu,
        editMenu,
        transportMenu,
        moduleMenu,
        sequenceMenu,
        trackMenu,
        patternMenu,
        instrumentMenu,
      ],
    }).then((menu) => void menu.setAsAppMenu());
    menuInitialised = true;
  }, [menuInitialised]);
}
