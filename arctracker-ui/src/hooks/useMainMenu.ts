import { commands } from "../control/commands.ts";
import { useEffect } from "react";
import { Menu, MenuItem, Submenu } from "@tauri-apps/api/menu";
import { platform } from "@tauri-apps/plugin-os";
import exitArctracker from "../exit/exit.ts";
import { SelectionType } from "../editing/pasteBuffer.ts";
import { useStore } from "../store/useStore.ts";
import { error } from "@tauri-apps/plugin-log";
import { EditHistoryState } from "../editing/editor.ts";
import { Instrument, MaxInstrumentSlots } from "../editing/editInstrument.ts";

const appMenu = {
  id: "app-menu",
  text: "Arctracker",
  items: [
    {
      id: "app-about",
      text: "About Arctracker",
      enabled: false,
      action: () => alert("Arctracker is amazing!"),
    },
    { item: "Separator" },
    {
      id: "app-settings",
      text: "Settings...",
      enabled: false,
      action: commands.editAppConfig,
    },
    {
      id: "app-command-reference",
      text: "Command Reference...",
      enabled: false,
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
      enabled: false,
      action: commands.loadFile,
    },
    {
      id: "file-save-module",
      text: "Save Module",
      enabled: false,
      action: commands.saveModule,
    },
    {
      id: "file-save-module-as",
      text: "Save Module As...",
      enabled: false,
      action: commands.saveModuleAs,
    },
    { item: "Separator" },
    {
      id: "file-export-audio",
      text: "Export Audio...",
      enabled: false,
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
      enabled: false,
      action: commands.undoEdit,
    },
    {
      id: "edit-redo",
      text: "Redo",
      enabled: false,
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
      id: "edit-clear-event-field",
      text: "Clear Field",
      enabled: true,
      action: commands.clearPatternEventField,
    },
    { item: "Separator" },
    {
      id: "edit-cut-events",
      text: "Cut Events",
      enabled: false,
      action: commands.cutPatternEvents,
    },
    {
      id: "edit-copy-events",
      text: "Copy Events",
      enabled: false,
      action: commands.copyPatternEvents,
    },
    {
      id: "edit-paste-events",
      text: "Paste Events",
      enabled: false,
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
      enabled: false,
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
      enabled: false,
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
      enabled: false,
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
      enabled: false,
      action: commands.editModuleMetaData,
    },
    {
      id: "module-set-track-count",
      text: "Set Track Count...",
      enabled: false,
      action: commands.editTrackCount,
    },
    {
      id: "module-edit-tempo",
      text: "Edit Tempo...",
      enabled: false,
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
      enabled: false,
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

let menuPromise: Promise<Menu> | null = null;

function initialiseMenu(): Promise<Menu> {
  if (!menuPromise) {
    menuPromise = Menu.new({
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
    }).then(async (menu) => {
      await menu.setAsAppMenu();
      return menu;
    }).catch((error) => {
      menuPromise = null;
      throw error;
    });
  }
  return menuPromise;
}

async function updateAppMenuItems(menu: Menu, dialogOpen: boolean) {
  const appMenu = await menu.get("app-menu") as Submenu;
  const [aboutItem, appSettingsItem, commandReferenceItem] = await Promise.all([
    appMenu.get("app-about"),
    appMenu.get("app-settings"),
    appMenu.get("app-command-reference"),
  ]) as MenuItem[];
  await Promise.all([
    aboutItem.setEnabled(!dialogOpen),
    appSettingsItem.setEnabled(!dialogOpen),
    commandReferenceItem.setEnabled(!dialogOpen),
  ])
}

async function updateFileMenuItems(menu: Menu, dialogOpen: boolean, fileAlreadySaved: boolean) {
  const fileMenu = await menu.get("file-menu") as Submenu;
  const [openItem, saveItem, saveAsItem, exportItem] = await Promise.all([
    fileMenu.get("file-open-module"),
    fileMenu.get("file-save-module"),
    fileMenu.get("file-save-module-as"),
    fileMenu.get("file-export-audio"),
  ]) as MenuItem[];
  await Promise.all([
    openItem.setEnabled(!dialogOpen),
    saveItem.setEnabled(!dialogOpen && fileAlreadySaved),
    saveAsItem.setEnabled(!dialogOpen),
    exportItem.setEnabled(!dialogOpen),
  ]);
}

async function updatePatternEditMenuItems(menu: Menu, patternEditing: boolean, cursorField: number) {
  const editMenu = await menu.get("edit-menu") as Submenu;
  const [
    clearEventItem,
    clearFieldItem,
    cutEventsItem,
    copyEventsItem,
    duplicateEventsItem,
    cutTrackItem,
    copyTrackItem,
    cutPatternItem,
    copyPatternItem,
    setMultipleEffectsItem
  ] =
    await Promise.all([
      editMenu.get("edit-clear-event"),
      editMenu.get("edit-clear-event-field"),
      editMenu.get("edit-cut-events"),
      editMenu.get("edit-copy-events"),
      editMenu.get("edit-duplicate-events"),
      editMenu.get("edit-cut-track"),
      editMenu.get("edit-copy-track"),
      editMenu.get("edit-cut-pattern"),
      editMenu.get("edit-copy-pattern"),
      editMenu.get("edit-set-multiple-effects"),
    ]) as MenuItem[];
  await Promise.all([
    clearEventItem.setEnabled(patternEditing),
    clearFieldItem.setEnabled(patternEditing && cursorField > 0),
    cutEventsItem.setEnabled(patternEditing),
    copyEventsItem.setEnabled(patternEditing),
    duplicateEventsItem.setEnabled(patternEditing),
    cutTrackItem.setEnabled(patternEditing),
    copyTrackItem.setEnabled(patternEditing),
    cutPatternItem.setEnabled(patternEditing),
    copyPatternItem.setEnabled(patternEditing),
    setMultipleEffectsItem.setEnabled(patternEditing),
  ]);
}

async function updatePasteMenuItems(
  menu: Menu,
  patternEditing: boolean,
  currentSelectionType: SelectionType | null,
) {
  const editMenu = await menu.get("edit-menu") as Submenu;
  const [pasteEventsItem, pasteTrackItem, pastePatternItem] =
    await Promise.all([
      editMenu.get("edit-paste-events"),
      editMenu.get("edit-paste-track"),
      editMenu.get("edit-paste-pattern"),
    ]) as MenuItem[];
  await Promise.all([
    pasteEventsItem.setEnabled(patternEditing && currentSelectionType === "patternEventSelection"),
    pasteTrackItem.setEnabled(patternEditing && currentSelectionType === "track"),
    pastePatternItem.setEnabled(patternEditing && currentSelectionType === "pattern"),
  ]);
}

async function updateUndoRedoMenuItems(menu: Menu, editHistoryState: EditHistoryState) {
  const editMenu = await menu.get("edit-menu") as Submenu;
  const [undoItem, redoItem] =
    await Promise.all([
      editMenu.get("edit-undo"),
      editMenu.get("edit-redo"),
    ]) as MenuItem[];
  await Promise.all([
    undoItem.setEnabled(editHistoryState.canUndo),
    redoItem.setEnabled(editHistoryState.canRedo),
  ]);
}

async function updateTransportMenuItems(menu: Menu, dialogOpen: boolean) {
  const transportMenu = await menu.get("transport-menu") as Submenu;
  const [goToPositionItem] = await Promise.all([transportMenu.get("transport-seek-to-position")]) as MenuItem[];
  await Promise.all([goToPositionItem.setEnabled(!dialogOpen)]);
}

async function updateModuleMenuItems(menu: Menu, dialogOpen: boolean) {
  const moduleMenu = await menu.get("module-menu") as Submenu;
  const [editModuleDetailsItem, setTrackCountItem, editTempoItem] = await Promise.all([
    moduleMenu.get("module-edit-details"),
    moduleMenu.get("module-set-track-count"),
    moduleMenu.get("module-edit-tempo"),
  ]) as MenuItem[];
  await Promise.all([
    editModuleDetailsItem.setEnabled(!dialogOpen),
    setTrackCountItem.setEnabled(!dialogOpen),
    editTempoItem.setEnabled(!dialogOpen),
  ]);
}

async function updatePatternMenuItems(menu: Menu, dialogOpen: boolean) {
  const patternMenu = await menu.get("pattern-menu") as Submenu;
  const [setLengthItem] = await Promise.all([patternMenu.get("pattern-set-length")]) as MenuItem[];
  await Promise.all([setLengthItem.setEnabled(!dialogOpen)]);
}

async function updateInstrumentMenuItems(
  menu: Menu,
  dialogOpen: boolean,
  instrumentCount: number,
  instrumentSelected: boolean,
  draftInstrumentAssigned: boolean,
) {
  const instrumentMenu = await menu.get("instrument-menu") as Submenu;
  const [addInstrumentItem, editInstrumentItem, loadSampleItem, deleteSampleItem, exportSampleItem] = await Promise.all([
    instrumentMenu.get("instrument-add"),
    instrumentMenu.get("instrument-edit"),
    instrumentMenu.get("instrument-load-sample"),
    instrumentMenu.get("instrument-delete-sample"),
    instrumentMenu.get("instrument-export-sample"),
  ]) as MenuItem[];
  await Promise.all([
    addInstrumentItem.setEnabled(!dialogOpen && instrumentCount < MaxInstrumentSlots),
    editInstrumentItem.setEnabled(!dialogOpen && instrumentSelected),
    loadSampleItem.setEnabled(dialogOpen),
    deleteSampleItem.setEnabled(dialogOpen && draftInstrumentAssigned),
    exportSampleItem.setEnabled(dialogOpen && draftInstrumentAssigned),
  ]);
}

function getSelectedInstrument(instruments: Instrument[], selectedInstrumentIndex: number | null) {
  if (instruments.length === 0)
    return null;
  if (selectedInstrumentIndex === null)
    return null;
  return instruments[selectedInstrumentIndex];
}

export function useMainMenu() {
  const editMode = useStore((state) => state.editorState.editMode);
  const cursorField = useStore((state) => state.editorState.cursorPosition.field);
  const currentSelectionType = useStore((state) => state.pasteBuffer?.type ?? null);
  const editHistoryState = useStore((state) => state.editHistoryState);
  const savedFileName = useStore((state) => state.module.fileName);
  const instruments = useStore((state) => state.module.instruments);
  const selectedInstrumentIndex = useStore((state) => state.selectedInstrument);
  const draftInstrumentAssigned = useStore((state) => state.draftInstrument.assigned);
  const instrumentCount = instruments.length;
  const instrumentSelected = getSelectedInstrument(instruments, selectedInstrumentIndex) !== null;
  const patternEditing = editMode === "patternEvents";
  const dialogOpen = (editMode !== "none" && editMode !== "patternEvents");
  const fileAlreadySaved = (savedFileName !== null && savedFileName !== undefined && savedFileName !== "");

  useEffect(() => {
    void initialiseMenu().catch((e) => error(e));
  }, []);

  useEffect(() => {
    void initialiseMenu()
      .then((menu) => updateAppMenuItems(menu, dialogOpen))
      .catch((e) => error(e));
  }, [dialogOpen]);

  useEffect(() => {
    void initialiseMenu()
      .then((menu) => updatePatternEditMenuItems(menu, patternEditing, cursorField))
      .catch((e) => error(e));
  }, [patternEditing, cursorField]);

  useEffect(() => {
    void initialiseMenu()
      .then((menu) => updatePasteMenuItems(menu, patternEditing, currentSelectionType))
      .catch((e) => error(e));
  }, [patternEditing, currentSelectionType]);

  useEffect(() => {
    void initialiseMenu()
      .then((menu) => updateUndoRedoMenuItems(menu, editHistoryState))
      .catch((e) => error(e));
  }, [editHistoryState])

  useEffect(() => {
    void initialiseMenu()
      .then((menu) => updateFileMenuItems(menu, dialogOpen, fileAlreadySaved))
      .catch((e) => error(e));
  }, [dialogOpen, fileAlreadySaved])

  useEffect(() => {
    void initialiseMenu()
      .then((menu) =>
        Promise.all([
          updateTransportMenuItems(menu, dialogOpen),
          updateModuleMenuItems(menu, dialogOpen),
          updatePatternMenuItems(menu, dialogOpen),
        ])
      )
      .catch((e) => error(e));
  }, [dialogOpen])

  useEffect(() => {
    void initialiseMenu()
      .then((menu) => updateInstrumentMenuItems(menu, dialogOpen, instrumentCount, instrumentSelected, draftInstrumentAssigned))
      .catch((e) => error(e));
  }, [dialogOpen, instruments, selectedInstrumentIndex, draftInstrumentAssigned]);
}
