import { useStore } from "../store/useStore.ts";
import { Cursor } from "./cursor.ts";
import { hexadecimal } from "../rendering/hexadecimal.ts";
import { commands } from "../control/commands.ts";
import { KeyHandler } from "../keyboard/keyHandler.ts";

export const editorKeyHandlers: {
  handleSampleFieldInput: KeyHandler,
  handleEffectFieldInput: KeyHandler,
} = {
  handleSampleFieldInput: (e) => {
    const { editorState } = useStore.getState();
    if (!editorState.patternEditing) return false;
    const cursorField = new Cursor().currentField();
    if (cursorField.field !== "sampleHigh" && cursorField.field !== "sampleLow")
      return false;
    const numberValue = hexadecimal.fromHexDigit(e.key);
    if (numberValue === null) return false;
    commands.editSampleField(cursorField, e.key);
    return true;
  },

  handleEffectFieldInput: (e) => {
    const { editorState } = useStore.getState();
    if (!editorState.patternEditing) return false;
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
};
