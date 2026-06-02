import { CommandType, commandQueue } from "./commands.ts";
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
import {sequence} from "../editing/sequence.ts";

function processCommands() {
  const commands = commandQueue.consume();
  for (const command of commands) {
    switch (command.type) {
      case CommandType.CREATE_MODULE:
        void module.create(command.numChannels);
        // Don't execute any more commands if we are creating a new module.
        return;
      case CommandType.LOAD_FILE:
        void module.load();
        // Don't execute any more commands if we are loading a new module.
        return;
      case CommandType.EXPORT_AUDIO:
        void audioExport.start();
        break;
      case CommandType.TOGGLE_PLAY:
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
        transport.sequenceSeek(command.position);
        break;
      case CommandType.SEQUENCE_SEEK_FORWARDS:
        selection.clearPatternSelection();
        transport.sequenceSeekForwards();
        break;
      case CommandType.SEQUENCE_SEEK_BACKWARDS:
        selection.clearPatternSelection();
        transport.sequenceSeekBackwards();
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
        void editor.undo();
        break;
      case CommandType.REDO_EDIT:
        void editor.redo();
        break;
      case CommandType.INCREMENT_PATTERN_AT_CURRENT_POSITION:
        selection.clearPatternSelection();
        sequence.incrementPatternAtCurrentPosition();
        break;
      case CommandType.DECREMENT_PATTERN_AT_CURRENT_POSITION:
        selection.clearPatternSelection();
        sequence.decrementPatternAtCurrentPosition();
        break;
    }
  }
}

export const controller = {
  commandPoller: (() => processCommands()) as AppPoller,
};
