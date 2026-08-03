import { Cursor } from "./cursor.ts";
import { hexadecimal } from "../rendering/hexadecimal.ts";
import { commands } from "../control/commands.ts";
import { KeyHandler } from "../keyboard/keyHandler.ts";
import { editInstrument } from "./editInstrument.ts";
import { primaryModifier } from "../keyboard/keyBinding.ts";
import { patternEvents } from "./patternEvents.ts";
import { moduleTitle } from "./moduleTitle.ts";
import { tempo } from "./tempo.ts";

export const editorKeyHandlers: {
  handleSampleFieldInput: KeyHandler,
  handleEffectFieldInput: KeyHandler,
  handleInstrumentEditorInput: KeyHandler,
  handleModuleTitleInput: KeyHandler,
  handleTempoInput: KeyHandler,
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

  handleModuleTitleInput: (e) => {
    if (!moduleTitle.editing())
      return false;
    if (e.code === 'Escape') {
      moduleTitle.hideDialog();
      return true;
    } else if (e.code === 'KeyS' && primaryModifier(e)) {
      commands.setModuleTitle();
      return true;
    }
    return false;
  },

  handleTempoInput: (e) => {
    if (!tempo.editing())
      return false;
    if (e.code === 'Escape') {
      tempo.hideDialog();
      return true;
    } else if (e.code === 'KeyS' && primaryModifier(e)) {
      commands.setTempo();
      return true;
    }
    return false;
  },
};
