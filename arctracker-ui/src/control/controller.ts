import { commandQueue } from "./commands.ts";
import { transport } from "../transport/transport.ts";
import { module } from "../module/module.ts";
import { editor } from "../editing/editor.ts";
import { cursor } from "../editing/cursor.ts";
import { patternEvents } from "../editing/patternEvents.ts";
import { AppPoller } from "../polling/poller.ts";
import { audioExport } from "../audioExport/audioExport.ts";
import { patternGrid } from "../editing/patternGrid.ts";
import { selection } from "../editing/selection.ts";
import { copyPaste } from "../editing/copyPaste.ts";
import { sequence } from "../editing/sequence.ts";
import { editInstrument } from "../editing/editInstrument.ts";
import { pattern } from "../editing/pattern.ts";
import { moduleMetaData } from "../editing/moduleMetaData.ts";
import { engine } from "../engine/engine.ts";
import { tempo } from "../editing/tempo.ts";
import { appConfig } from "../config/appConfig.ts";
import { useStore } from "../store/useStore.ts";
import { pianoKeys } from "../keyboard/pianoKeys.ts";

async function processCommands() {
  const commands = commandQueue.consume();
  for (const command of commands) {
    console.log("command", JSON.stringify(command)); // TODO: Add an event log for this.
    switch (command.type) {
      case "Edit application config":
        if (transport.playing()) transport.togglePlay();
        appConfig.showDialog();
        break;
      case "Set application config":
        void appConfig.apply(command.newConfig);
        break;
      case "Create module":
        void module.create(false).then((success) => {
          if (success) editor.newModuleLoaded();
        });
        return; // Don't execute any more commands if we have created a new module.
      case "Create module using defaults":
        void module.create(true).then((success) => {
          if (success) editor.newModuleLoaded();
        });
        return; // Don't execute any more commands if we have created a new module.
      case "Load module":
        void module.load().then((success) => {
          if (success) editor.newModuleLoaded();
        });
        return; // Don't execute any more commands if we have loaded a new module.
      case "Save module as":
        void module.saveAs();
        break;
      case "Save module":
        void module.save();
        break;
      case "Export audio":
        void audioExport.start();
        break;
      case "Export sample":
        void editInstrument.exportSample();
        break;
      case "Toggle play":
        if (editInstrument.instrumentEditing()) break;
        selection.clearPatternSelection();
        editor.cancelPatternEdit();
        transport.togglePlay();
        break;
      case "Toggle loop mode":
        transport.toggleLoop();
        break;
      case "Toggle pattern edit mode":
        editor.togglePatternEdit();
        break;
      case "Sequence seek":
        selection.clearPatternSelection();
        transport.sequenceSeek(command.position);
        sequence.updatePosition(command.position);
        break;
      case "Sequence seek forwards":
        selection.clearPatternSelection();
        transport.sequenceSeekForwards();
        sequence.advance();
        break;
      case "Sequence seek backwards":
        selection.clearPatternSelection();
        transport.sequenceSeekBackwards();
        sequence.reverse();
        break;
      case "Sequence seek to start":
        selection.clearPatternSelection();
        transport.sequenceSeekToStart();
        sequence.goToStart();
        break;
      case "Sequence seek to end":
        selection.clearPatternSelection();
        transport.sequenceSeekToEnd();
        sequence.goToEnd();
        break;
      case "Next instrument":
        editInstrument.nextInstrument();
        break;
      case "Previous instrument":
        editInstrument.previousInstrument();
        break;
      case "First instrument":
        editInstrument.firstInstrument();
        break;
      case "Last instrument":
        editInstrument.lastInstrument();
        break;
      case "Pattern grid down":
        selection.navigateGrid(
          () => patternGrid.moveDown(command.wrap),
          command.extendSelection,
        );
        break;
      case "Pattern grid up":
        selection.navigateGrid(
          () => patternGrid.moveUp(),
          command.extendSelection,
        );
        break;
      case "Pattern grid left":
        selection.navigateGrid(
          () => patternGrid.moveLeft(),
          command.extendSelection,
        );
        break;
      case "Pattern grid right":
        selection.navigateGrid(
          () => patternGrid.moveRight(),
          command.extendSelection,
        );
        break;
      case "Pattern grid stride down":
        selection.navigateGrid(
          () => patternGrid.strideDown(),
          command.extendSelection,
        );
        break;
      case "Pattern grid stride up":
        selection.navigateGrid(
          () => patternGrid.strideUp(),
          command.extendSelection,
        );
        break;
      case "Pattern grid jump to top":
        selection.navigateGrid(
          () => patternGrid.jumpToTop(),
          command.extendSelection,
        );
        break;
      case "Pattern grid jump to bottom":
        selection.navigateGrid(
          () => patternGrid.jumpToBottom(),
          command.extendSelection,
        );
        break;
      case "Pattern grid jump to location":
        selection.navigateGrid(
          () =>
            patternGrid.moveTo({
              track: command.track,
              patternIndex: command.patternIndex,
            }),
          command.extendSelection,
        );
        break;
      case "Cursor field left":
        cursor.moveFieldLeft();
        break;
      case "Cursor field right":
        cursor.moveFieldRight();
        break;
      case "Increase effects displayed":
        editor.increaseEffectsDisplayed();
        break;
      case "Decrease effects displayed":
        editor.decreaseEffectsDisplayed();
        break;
      case "Edit note field":
        void patternEvents.setEventNote(command.note);
        break;
      case "Edit sample field":
        void patternEvents.setEventSample(command.field, command.value);
        break;
      case "Edit effect code":
        void patternEvents.setEventEffectCode(command.field, command.value);
        break;
      case "Edit effect data":
        void patternEvents.setEventEffectData(command.field, command.value);
        break;
      case "Clear pattern event field":
        void patternEvents.clearEventField();
        break;
      case "Clear pattern event":
        void patternEvents.clearEvent();
        break;
      case "Copy pattern events":
        void copyPaste.copyPatternEvents(null);
        break;
      case "Cut pattern events":
        void copyPaste.cutPatternEvents();
        break;
      case "Paste pattern events":
        void copyPaste.pastePatternEvents(null);
        break;
      case "Copy track":
        void copyPaste.copyTrack();
        break;
      case "Cut track":
        void copyPaste.cutTrack();
        break;
      case "Paste track":
        void copyPaste.pasteTrack();
        break;
      case "Copy pattern":
        void copyPaste.copyPattern();
        break;
      case "Cut pattern":
        void copyPaste.cutPattern();
        break;
      case "Paste pattern":
        void copyPaste.pastePattern();
        break;
      case "Undo edit":
        void editor.undoEdit();
        break;
      case "Redo edit":
        void editor.redoEdit();
        break;
      case "Increment pattern at current position":
        selection.clearPatternSelection();
        sequence.incrementPatternAtCurrentPosition();
        break;
      case "Decrement pattern at current position":
        selection.clearPatternSelection();
        sequence.decrementPatternAtCurrentPosition();
        break;
      case "Insert sequence position before":
        selection.clearPatternSelection();
        void sequence.insertBefore(command.createNewPattern);
        break;
      case "Insert sequence position after":
        selection.clearPatternSelection();
        void sequence.insertAfter(command.createNewPattern);
        break;
      case "Delete sequence position":
        selection.clearPatternSelection();
        void sequence.delete();
        break;
      case "Add instrument":
        const instruments = useStore.getState().module.instruments;
        const setSelectedInstrument = useStore.getState().setSelectedInstrument;
        setSelectedInstrument(instruments.length);
        if (transport.playing()) transport.togglePlay();
        editInstrument.showDialog();
        break;
      case "Open instrument editor":
        if (transport.playing()) transport.togglePlay();
        editInstrument.showDialog();
        break;
      case "Save and close instrument editor":
        void editInstrument.updateInstrument();
        editInstrument.closeDialog();
        break;
      case "Restore and close instrument editor":
        void editInstrument.restoreInstrument();
        editInstrument.closeDialog();
        break;
      case "Load sample":
        void editInstrument.loadSample();
        break;
      case "Delete sample":
        editInstrument.deleteSample();
        break;
      case "Edit current pattern length":
        if (transport.playing()) transport.togglePlay();
        pattern.editCurrentPatternLength();
        break;
      case "Set current pattern length":
        void pattern.setCurrentPatternLength(command.newLength);
        break;
      case "Edit module metadata":
        moduleMetaData.showDialog();
        break;
      case "Set module metadata":
        void moduleMetaData.setModuleMetaData();
        moduleMetaData.hideDialog();
        break;
      case "Edit track count":
        if (transport.playing()) transport.togglePlay();
        module.editTrackCount();
        break;
      case "Set track count":
        void module.setTrackCount(command.trackCount);
        break;
      case "Edit tempo":
        if (transport.playing()) transport.togglePlay();
        tempo.showDialog();
        break;
      case "Set tempo":
        void tempo.setTempo();
        break;
      case "Toggle current track mute":
        const track = cursor.currentPosition().track;
        void engine.toggleTrackMute(track);
        break;
      case "Toggle track mute":
        void engine.toggleTrackMute(command.track);
        break;
      case "Shift keyboard octave up":
        pianoKeys.shiftOctave(1);
        break;
      case "Shift keyboard octave down":
        pianoKeys.shiftOctave(-1);
        break;
      case "Open hex calculator":
        useStore.getState().setHexCalculatorActive(true);
        break;
      case "Close hex calculator":
        useStore.getState().setHexCalculatorActive(false);
        break;
    }
  }
}

export const controller = {
  commandPoller: (() => processCommands()) as AppPoller,
};
