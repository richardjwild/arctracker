import Modal from "./Modal.tsx";
import { message } from "../language/messages.ts";
import "./SetMultipleEffects.css";
import { useEffect, useState } from "react";
import { editor } from "../editing/editor.ts";
import { hexadecimal } from "../rendering/hexadecimal.ts";
import { commands } from "../control/commands.ts";
import { useStore } from "../store/useStore.ts";
import { alerting } from "../alerting/alert.ts";
import { Effect } from "../editing/patternEvents.ts";

export default function SetMultipleEffects() {
  const [effectLane, setEffectLane] = useState(1);
  const [inputEffectValue, setInputEffectValue] = useState("");
  const [onlyNoteOns, setOnlyNoteOns] = useState(false);
  const [effect, setEffect] = useState<Effect>({ effectCode: [0, 0], effectData: [0, 0]});
  const editing = useStore((state) => state.editorState.editMode) === "setMultipleEffects";

  useEffect(() => {
    if (!editing) return;
    setEffectLane(1);
    setInputEffectValue("");
    setEffect({ effectCode: [0, 0], effectData: [0, 0]});
    setOnlyNoteOns(false);
  }, [editing])

  if (!editing) return null;

  const validateEffectValue = () => {
    const numberValue = hexadecimal.fromHex(inputEffectValue);
    if (numberValue === null) {
      void alerting.showError(message("invalidEffectValue"));
      setInputEffectValue("");
      return;
    }
    const effectCode = [ numberValue >>> 12, (numberValue >>> 8) & 0xF ];
    const effectData = [ (numberValue >>> 4) & 0xF, numberValue & 0xF];
    setEffect({ effectCode, effectData });
    editor.stopTextInput();
  };

  const apply = () => {
    commands.setMultipleEffects(effectLane - 1, effect, onlyNoteOns);
    editor.setEditMode("patternEvents");
  };

  const cancel = () => {
    editor.setEditMode("none");
  };

  return (
    <Modal className="setMultipleEffects">
      <div className="effectLaneLabel">
        <label htmlFor="effectLaneEdit">{message("effectLaneLabel")}</label>
      </div>
      <div className="effectLaneEdit">
        <div className="uiArea padded rounded">
          <input
            type="number"
            id="effectLaneEdit"
            min="1"
            max="4"
            value={effectLane}
            onFocus={editor.startTextInput}
            onChange={(e) => setEffectLane(Number(e.target.value))}
            onBlur={editor.stopTextInput}
          />
        </div>
      </div>
      <div className="effectValueLabel">
        <label htmlFor="effectValueEdit">{message("effectValueLabel")}</label>
      </div>
      <div className="effectValueEdit">
        <div className="uiArea padded rounded">
          <input
            type="text"
            maxLength={4}
            id="effectValueEdit"
            value={inputEffectValue}
            onFocus={editor.startTextInput}
            onChange={(e) => setInputEffectValue(e.target.value)}
            onBlur={validateEffectValue}
          />
        </div>
      </div>
      <div className="onlyNoteOnsLabel">
        <label>{message("onlyNoteOnsLabel")}</label>
      </div>
      <div className="onlyNoteOnsEdit">
        <div className="uiArea padded rounded">
          <input
            type="radio"
            id="onlyNoteOnsYes"
            name="onlyNoteOns"
            checked={onlyNoteOns}
            onChange={() => setOnlyNoteOns(true)}
          />
          <label htmlFor="onlyNoteOnsYes">{message("yes")}</label>
          <input
            type="radio"
            id="onlyNoteOnsNo"
            name="onlyNoteOns"
            checked={!onlyNoteOns}
            onChange={() => setOnlyNoteOns(false)}
          />
          <label htmlFor="onlyNoteOnsNo">{message("no")}</label>
        </div>
      </div>
      <div className="saveCloseButtons uiArea padded rounded">
        <button type="button" onClick={apply}>
          {message("applyButtonLabel")}
        </button>
        <button type="button" onClick={cancel}>
          {message("cancelButtonLabel")}
        </button>
      </div>
    </Modal>
  );
}