import { Cursor } from "./cursor.ts";
import { hexadecimal } from "../rendering/hexadecimal.ts";
import { commands } from "../control/commands.ts";
import { KeyHandler } from "../keyboard/keyHandler.ts";
import { editInstrument } from "./editInstrument.ts";
import { primaryModifier } from "../keyboard/keyBinding.ts";
import { patternEvents } from "./patternEvents.ts";
import { pattern } from "./pattern.ts";
import { editor } from "./editor.ts";
import { useStore } from "../store/useStore.ts";

export const editorKeyHandlers: {
  handleSampleFieldInput: KeyHandler,
  handleEffectFieldInput: KeyHandler,
  handleInstrumentEditorInput: KeyHandler,
  handlePatternLengthInput: KeyHandler,
} = {
  handleSampleFieldInput: (e) => {
    if (!patternEvents.editing()) return false;
    const cursorField = new Cursor().currentField();
    if (cursorField.field !== "sampleHigh" && cursorField.field !== "sampleLow")
      return false;
    const numberValue = hexadecimal.fromHexDigit(e.key);
    if (numberValue === null) return false;
    commands.editSampleField(cursorField, e.key);
    return true;
  },

  handleEffectFieldInput: (e) => {
    if (!patternEvents.editing()) return false;
    const cursorField = new Cursor().currentField();
    if (cursorField.field === "effectCode") {
      if (e.metaKey || e.ctrlKey || !/^[0-9A-Za-z]$/i.test(e.key)) return false;
      commands.editEffectCode(cursorField, e.key);
      return true;
    } else if (
      cursorField.field === "effectData1" ||
      cursorField.field === "effectData2"
    ) {
      const numberValue = hexadecimal.fromHexDigit(e.key);
      if (numberValue === null) return false;
      commands.editEffectData(cursorField, e.key);
      return true;
    }
    return false;
  },

  handleInstrumentEditorInput: (e) => {
    if (!editInstrument.instrumentEditing())
      return false;
    if (e.code === 'Escape') {
      void commands.restoreAndCloseInstrumentEditor();
      return true;
    } else if (e.code === 'KeyL' && primaryModifier(e)) {
      void commands.loadSample();
      return true;
    } else if (e.code === 'KeyD' && primaryModifier(e)) {
      void commands.deleteSample();
      return true;
    } else if (e.code === 'KeyS' && primaryModifier(e)) {
      void commands.saveAndCloseInstrumentEditor();
      return true;
    }
    return false;
  },

  handlePatternLengthInput: (e) => {
    if (!pattern.editingLength())
      return false;
    if (e.code === 'Escape') {
      editor.setEditMode('none');
      return true;
    } else if (e.code === 'Enter') {
      const newPatternLength = useStore.getState().newPatternLength;
      if (newPatternLength)
        void commands.setCurrentPatternLength(newPatternLength)
      editor.setEditMode('none');
    }
    return false;
  },
};
