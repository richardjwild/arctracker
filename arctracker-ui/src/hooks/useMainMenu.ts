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
import { message } from "../language/messages.ts";

const appMenu = {
  id: "app-menu",
  text: message("menuApp"),
  items: [
    {
      id: "app-about",
      text: message("menuAppAbout"),
      enabled: false,
      action: () => alert("Arctracker is amazing!"),
    },
    { item: "Separator" },
    {
      id: "app-settings",
      text: message("menuAppSettings"),
      enabled: false,
      action: commands.editAppConfig,
    },
    {
      id: "app-command-reference",
      text: message("menuAppCommandReference"),
      enabled: false,
      action: commands.showCommandReference,
    },
    { item: "Separator" },
    {
      id: "app-exit",
      text: message("menuAppQuit"),
      enabled: true,
      accelerator: platform() === "macos" ? "Cmd+Q" : undefined,
      action: exitArctracker,
    },
  ],
};

const fileMenu = {
  id: "file-menu",
  text: message("menuFile"),
  items: [
    {
      id: "file-new",
      text: message("menuFileNewModule"),
      enabled: true,
      action: commands.createModuleUsingDefaults,
    },
    { item: "Separator" },
    {
      id: "file-open-module",
      text: message("menuFileOpenModule"),
      enabled: false,
      action: commands.loadFile,
    },
    {
      id: "file-save-module",
      text: message("menuFileSaveModule"),
      enabled: false,
      action: commands.saveModule,
    },
    {
      id: "file-save-module-as",
      text: message("menuFileSaveModuleAs"),
      enabled: false,
      action: commands.saveModuleAs,
    },
    { item: "Separator" },
    {
      id: "file-export-audio",
      text: message("menuFileExportAudio"),
      enabled: false,
      action: commands.exportAudio,
    },
  ],
}

const editMenu = {
  id: "edit-menu",
  text: message("menuEdit"),
  items: [
    {
      id: "edit-undo",
      text: message("menuEditUndo"),
      enabled: false,
      action: commands.undoEdit,
    },
    {
      id: "edit-redo",
      text: message("menuEditRedo"),
      enabled: false,
      action: commands.redoEdit,
    },
    { item: "Separator" },
    {
      id: "edit-toggle",
      text: message("menuEditToggle"),
      enabled: true,
      action: commands.toggleEdit,
    },
    { item: "Separator" },
    {
      id: "edit-clear-event",
      text: message("menuEditClearEvent"),
      enabled: true,
      action: commands.clearPatternEvent,
    },
    {
      id: "edit-clear-event-field",
      text: message("menuEditClearEventField"),
      enabled: true,
      action: commands.clearPatternEventField,
    },
    { item: "Separator" },
    {
      id: "edit-cut-events",
      text: message("menuEditCutEvents"),
      enabled: false,
      action: commands.cutPatternEvents,
    },
    {
      id: "edit-copy-events",
      text: message("menuEditCopyEvents"),
      enabled: false,
      action: commands.copyPatternEvents,
    },
    {
      id: "edit-paste-events",
      text: message("menuEditPasteEvents"),
      enabled: false,
      action: commands.pastePatternEvents,
    },
    {
      id: "edit-duplicate-events",
      text: message("menuEditDuplicateEvents"),
      enabled: true,
      action: commands.duplicatePatternEvents,
    },
    { item: "Separator" },
    {
      id: "edit-cut-track",
      text: message("menuEditCutTrack"),
      enabled: true,
      action: commands.cutTrack,
    },
    {
      id: "edit-copy-track",
      text: message("menuEditCopyTrack"),
      enabled: true,
      action: commands.copyTrack,
    },
    {
      id: "edit-paste-track",
      text: message("menuEditPasteTrack"),
      enabled: false,
      action: commands.pasteTrack,
    },
    { item: "Separator" },
    {
      id: "edit-cut-pattern",
      text: message("menuEditCutPattern"),
      enabled: true,
      action: commands.cutPattern,
    },
    {
      id: "edit-copy-pattern",
      text: message("menuEditCopyPattern"),
      enabled: true,
      action: commands.copyPattern,
    },
    {
      id: "edit-paste-pattern",
      text: message("menuEditPastePattern"),
      enabled: false,
      action: commands.pastePattern,
    },
    { item: "Separator" },
    {
      id: "edit-set-multiple-effects",
      text: message("menuEditSetMultipleEffects"),
      enabled: true,
      action: commands.openSetMultipleEffects,
    },
  ],
};

const transportMenu = {
  id: "transport-menu",
  text: message("menuTransport"),
  items: [
    {
      id: "transport-toggle-play",
      text: message("menuTransportTogglePlay"),
      enabled: true,
      action: commands.togglePlay,
    },
    {
      id: "transport-toggle-pattern-loop",
      text: message("menuTransportToggleLoop"),
      enabled: true,
      action: commands.toggleLoop,
    },
    { item: "Separator" },
    {
      id: "transport-seek-forwards",
      text: message("menuTransportSeekForwards"),
      enabled: true,
      action: commands.sequenceSeekForwards,
    },
    {
      id: "transport-seek-backwards",
      text: message("menuTransportSeekBackwards"),
      enabled: true,
      action: commands.sequenceSeekBackwards,
    },
    {
      id: "transport-seek-to-start",
      text: message("menuTransportSeekStart"),
      enabled: true,
      action: commands.sequenceSeekToStart,
    },
    {
      id: "transport-seek-to-end",
      text: message("menuTransportSeekEnd"),
      enabled: true,
      action: commands.sequenceSeekToEnd,
    },
    {
      id: "transport-seek-to-position",
      text: message("menuTransportSeekPosition"),
      enabled: false,
      action: commands.openSequenceSeekDialog,
    },
  ],
};

const moduleMenu = {
  id: "module-menu",
  text: message("menuModule"),
  items: [
    {
      id: "module-edit-details",
      text: message("menuModuleEditDetails"),
      enabled: false,
      action: commands.editModuleMetaData,
    },
    {
      id: "module-set-track-count",
      text: message("menuModuleSetTrackCount"),
      enabled: false,
      action: commands.editTrackCount,
    },
    {
      id: "module-edit-tempo",
      text: message("menuModuleEditTempo"),
      enabled: false,
      action: commands.editTempo,
    },
  ],
};

const sequenceMenu = {
  id: "sequence-menu",
  text: message("menuSequence"),
  items: [
    {
      id: "sequence-increment-pattern",
      text: message("menuSequenceIncrement"),
      enabled: true,
      action: commands.incrementPatternAtCurrentPosition,
    },
    {
      id: "sequence-decrement-pattern",
      text: message("menuSequenceDecrement"),
      enabled: true,
      action: commands.decrementPatternAtCurrentPosition,
    },
    { item: "Separator" },
    {
      id: "sequence-insert-before",
      text: message("menuSequenceInsertBefore"),
      enabled: true,
      action: () => commands.insertSequencePositionBefore(false),
    },
    {
      id: "sequence-insert-after",
      text: message("menuSequenceInsertAfter"),
      enabled: true,
      action: () => commands.insertSequencePositionAfter(false),
    },
    {
      id: "sequence-insert-before-new",
      text: message("menuSequenceInsertNewBefore"),
      enabled: true,
      action: () => commands.insertSequencePositionBefore(true),
    },
    {
      id: "sequence-insert-after-new",
      text: message("menuSequenceInsertNewAfter"),
      enabled: true,
      action: () => commands.insertSequencePositionAfter(true),
    },
    { item: "Separator" },
    {
      id: "sequence-delete-position",
      text: message("menuSequenceDelete"),
      enabled: true,
      action: commands.deleteSequencePosition,
    },
  ],
};

const trackMenu = {
  id: "track-menu",
  text: message("menuTrack"),
  items: [
    {
      id: "track-toggle-mute",
      text: message("menuTrackToggleMute"),
      enabled: true,
      action: commands.toggleCurrentTrackMute,
    },
    { item: "Separator" },
    {
      id: "track-increase-effects-displayed",
      text: message("menuTrackIncreaseEffects"),
      enabled: true,
      action: commands.increaseEffectsDisplayed,
    },
    {
      id: "track-decrease-effects-displayed",
      text: message("menuTrackDecreaseEffects"),
      enabled: true,
      action: commands.decreaseEffectsDisplayed,
    },
  ],
};

const patternMenu = {
  id: "pattern-menu",
  text: message("menuPattern"),
  items: [
    {
      id: "pattern-create",
      text: message("menuPatternNew"),
      enabled: true,
      action: commands.createPatternAtCurrentPosition,
    },
    {
      id: "pattern-set-length",
      text: message("menuPatternSetLength"),
      enabled: false,
      action: commands.editCurrentPatternLength,
    },
  ],
};

const instrumentMenu = {
  id: "instrument-menu",
  text: message("menuInstrument"),
  items: [
    {
      id: "instrument-next",
      text: message("menuInstrumentNext"),
      enabled: true,
      action: commands.nextInstrument,
    },
    {
      id: "instrument-previous",
      text: message("menuInstrumentPrevious"),
      enabled: true,
      action: commands.previousInstrument,
    },
    {
      id: "instrument-first",
      text: message("menuInstrumentFirst"),
      enabled: true,
      action: commands.firstInstrument,
    },
    {
      id: "instrument-last",
      text: message("menuInstrumentLast"),
      enabled: true,
      action: commands.lastInstrument,
    },
    { item: "Separator" },
    {
      id: "instrument-add",
      text: message("menuInstrumentAdd"),
      enabled: true,
      action: commands.addInstrument,
    },
    {
      id: "instrument-edit",
      text: message("menuInstrumentEdit"),
      enabled: true,
      action: commands.openInstrumentEditor,
    },
    { item: "Separator" },
    {
      id: "instrument-load-sample",
      text: message("menuInstrumentLoadSample"),
      enabled: true,
      action: commands.loadSample,
    },
    {
      id: "instrument-delete-sample",
      text: message("menuInstrumentDeleteSample"),
      enabled: true,
      action: commands.deleteSample,
    },
    {
      id: "instrument-export-sample",
      text: message("menuInstrumentExportSample"),
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
