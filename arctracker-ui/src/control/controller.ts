import { commandQueue, CommandType } from "./commands.ts";
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
import { moduleTitle } from "../editing/moduleTitle.ts";

function processCommands() {
  const commands = commandQueue.consume();
  for (const command of commands) {
    switch (command.type) {
      case CommandType.CREATE_MODULE:
        let moduleCreated = false;
        module.create(command.numTracks).then((success) => {
          editor.clearUndoBuffer();
          moduleCreated = success;
        });
        // Don't execute any more commands if we have created a new module.
        if (moduleCreated) return;
        break;
      case CommandType.LOAD_FILE:
        let moduleLoaded = false;
        module.load().then((success) => {
          editor.clearUndoBuffer();
          moduleLoaded = success;
        });
        // Don't execute any more commands if we have loaded a new module.
        if (moduleLoaded) return;
        break;
      case CommandType.SAVE_MODULE_AS:
        void module.saveAs();
        break;
      case CommandType.SAVE_MODULE:
        void module.save();
        break;
      case CommandType.EXPORT_AUDIO:
        void audioExport.start();
        break;
      case CommandType.TOGGLE_PLAY:
        if (editInstrument.instrumentEditing()) break;
        selection.clearPatternSelection();
        editor.cancelPatternEdit();
        transport.togglePlay();
        break;
      case CommandType.TOGGLE_LOOP:
        transport.toggleLoop();
        break;
      case CommandType.TOGGLE_EDIT:
        editor.togglePatternEdit();
        break;
      case CommandType.SEQUENCE_SEEK:
        selection.clearPatternSelection();
        if (transport.playing()) transport.sequenceSeek(command.position);
        else sequence.updatePosition(command.position);
        break;
      case CommandType.SEQUENCE_SEEK_FORWARDS:
        selection.clearPatternSelection();
        if (transport.playing()) transport.sequenceSeekForwards();
        else sequence.advance();
        break;
      case CommandType.SEQUENCE_SEEK_BACKWARDS:
        selection.clearPatternSelection();
        if (transport.playing()) transport.sequenceSeekBackwards();
        else sequence.reverse();
        break;
      case CommandType.SEQUENCE_SEEK_TO_START:
        selection.clearPatternSelection();
        if (transport.playing()) transport.sequenceSeekToStart();
        else sequence.goToStart();
        break;
      case CommandType.SEQUENCE_SEEK_TO_END:
        selection.clearPatternSelection();
        if (transport.playing()) transport.sequenceSeekToEnd();
        else sequence.goToEnd();
        break;
      case CommandType.NEXT_INSTRUMENT:
        editInstrument.nextInstrument();
        break;
      case CommandType.PREVIOUS_INSTRUMENT:
        editInstrument.previousInstrument();
        break;
      case CommandType.FIRST_INSTRUMENT:
        editInstrument.firstInstrument();
        break;
      case CommandType.LAST_INSTRUMENT:
        editInstrument.lastInstrument();
        break;
      case CommandType.PATTERN_GRID_DOWN:
        selection.navigateGrid(
          () => patternGrid.moveDown(command.wrap),
          command.extendSelection,
        );
        break;
      case CommandType.PATTERN_GRID_UP:
        selection.navigateGrid(
          () => patternGrid.moveUp(),
          command.extendSelection,
        );
        break;
      case CommandType.PATTERN_GRID_LEFT:
        selection.navigateGrid(
          () => patternGrid.moveLeft(),
          command.extendSelection,
        );
        break;
      case CommandType.PATTERN_GRID_RIGHT:
        selection.navigateGrid(
          () => patternGrid.moveRight(),
          command.extendSelection,
        );
        break;
      case CommandType.PATTERN_GRID_STRIDE_DOWN:
        selection.navigateGrid(
          () => patternGrid.strideDown(),
          command.extendSelection,
        );
        break;
      case CommandType.PATTERN_GRID_STRIDE_UP:
        selection.navigateGrid(
          () => patternGrid.strideUp(),
          command.extendSelection,
        );
        break;
      case CommandType.PATTERN_GRID_JUMP_TO_TOP:
        selection.navigateGrid(
          () => patternGrid.jumpToTop(),
          command.extendSelection,
        );
        break;
      case CommandType.PATTERN_GRID_JUMP_TO_BOTTOM:
        selection.navigateGrid(
          () => patternGrid.jumpToBottom(),
          command.extendSelection,
        );
        break;
      case CommandType.PATTERN_GRID_JUMP_TO_LOCATION:
        selection.navigateGrid(
          () => patternGrid.moveTo({ track: command.track, patternIndex: command.patternIndex }),
          command.extendSelection,
        );
        break;
      case CommandType.CURSOR_FIELD_LEFT:
        cursor.moveFieldLeft();
        break;
      case CommandType.CURSOR_FIELD_RIGHT:
        cursor.moveFieldRight();
        break;
      case CommandType.INCREASE_EFFECTS_DISPLAYED:
        editor.increaseEffectsDisplayed();
        break;
      case CommandType.DECREASE_EFFECTS_DISPLAYED:
        editor.decreaseEffectsDisplayed();
        break;
      case CommandType.EDIT_NOTE_FIELD:
        void patternEvents.setEventNote(command.note);
        break;
      case CommandType.EDIT_SAMPLE_FIELD:
        void patternEvents.setEventSample(command.field, command.value);
        break;
      case CommandType.EDIT_EFFECT_CODE:
        void patternEvents.setEventEffectCode(command.field, command.value);
        break;
      case CommandType.EDIT_EFFECT_DATA:
        void patternEvents.setEventEffectData(command.field, command.value);
        break;
      case CommandType.CLEAR_PATTERN_EVENT_FIELD:
        void patternEvents.clearEventField();
        break;
      case CommandType.CLEAR_PATTERN_EVENT:
        void patternEvents.clearEvent();
        break;
      case CommandType.COPY_PATTERN_EVENTS:
        void copyPaste.copyPatternEvents(null);
        break;
      case CommandType.CUT_PATTERN_EVENTS:
        void copyPaste.cutPatternEvents();
        break;
      case CommandType.PASTE_PATTERN_EVENTS:
        void copyPaste.pastePatternEvents(null);
        break;
      case CommandType.COPY_TRACK:
        void copyPaste.copyTrack();
        break;
      case CommandType.CUT_TRACK:
        void copyPaste.cutTrack();
        break;
      case CommandType.PASTE_TRACK:
        void copyPaste.pasteTrack();
        break;
      case CommandType.COPY_PATTERN:
        void copyPaste.copyPattern();
        break;
      case CommandType.CUT_PATTERN:
        void copyPaste.cutPattern();
        break;
      case CommandType.PASTE_PATTERN:
        void copyPaste.pastePattern();
        break;
      case CommandType.UNDO_EDIT:
        void editor.undoEdit();
        break;
      case CommandType.REDO_EDIT:
        void editor.redoEdit();
        break;
      case CommandType.INCREMENT_PATTERN_AT_CURRENT_POSITION:
        selection.clearPatternSelection();
        sequence.incrementPatternAtCurrentPosition();
        break;
      case CommandType.DECREMENT_PATTERN_AT_CURRENT_POSITION:
        selection.clearPatternSelection();
        sequence.decrementPatternAtCurrentPosition();
        break;
      case CommandType.INSERT_SEQUENCE_POSITION_BEFORE:
        selection.clearPatternSelection();
        void sequence.insertBefore(command.createNewPattern);
        break;
      case CommandType.INSERT_SEQUENCE_POSITION_AFTER:
        selection.clearPatternSelection();
        void sequence.insertAfter(command.createNewPattern);
        break;
      case CommandType.DELETE_SEQUENCE_POSITION:
        selection.clearPatternSelection();
        void sequence.delete();
        break;
      case CommandType.OPEN_INSTRUMENT_EDITOR:
        if (transport.playing()) transport.togglePlay();
        editInstrument.showDialog();
        break;
      case CommandType.SAVE_AND_CLOSE_INSTRUMENT_EDITOR:
        void editInstrument.updateInstrument();
        editInstrument.closeDialog();
        break;
      case CommandType.RESTORE_AND_CLOSE_INSTRUMENT_EDITOR:
        void editInstrument.restoreInstrument();
        editInstrument.closeDialog();
        break;
      case CommandType.LOAD_SAMPLE:
        void editInstrument.loadSample();
        break;
      case CommandType.DELETE_SAMPLE:
        editInstrument.deleteSample();
        break;
      case CommandType.EDIT_CURRENT_PATTERN_LENGTH:
        if (transport.playing()) transport.togglePlay();
        pattern.editCurrentPatternLength();
        break;
      case CommandType.SET_CURRENT_PATTERN_LENGTH:
        void pattern.setCurrentPatternLength(command.newLength);
        break;
      case CommandType.EDIT_MODULE_TITLE:
        moduleTitle.showDialog();
        break;
      case CommandType.SET_MODULE_TITLE:
        void moduleTitle.setModuleTitle();
        moduleTitle.hideDialog();
        break;
      case CommandType.EDIT_TRACK_COUNT:
        if (transport.playing()) transport.togglePlay();
        module.editTrackCount();
        break;
      case CommandType.SET_TRACK_COUNT:
        void module.setTrackCount(command.trackCount);
        break;
    }
  }
}

export const controller = {
  commandPoller: (() => processCommands()) as AppPoller,
};
