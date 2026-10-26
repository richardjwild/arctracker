import Modal from "./Modal.tsx";
import { message } from "../language/messages.ts";
import "./SetMultipleEffects.css";
import { useEffect, useRef, useState } from "react";
import { editor } from "../editing/editor.ts";
import { hexadecimal } from "../rendering/hexadecimal.ts";
import { commands } from "../control/commands.ts";
import { useStore } from "../store/useStore.ts";
import { alerting } from "../alerting/alert.ts";
import { Effect } from "../editing/patternEvents.ts";
import { KeyHandler } from "../keyboard/keyHandler.ts";
import FieldLabel from "./components/FieldLabel.tsx";
import { DecrementButton, IncrementButton } from "./components/SpinnerButtons.tsx";
import { primaryModifier } from "../keyboard/keyBinding.ts";
import { useKeyboardHandler } from "../hooks/useKeyboardHandler.ts";

export default function SetMultipleEffects() {
  const [effectLane, setEffectLane] = useState(1);
  const [inputEffectLane, setInputEffectLane] = useState("1");
  const [inputEffectValue, setInputEffectValue] = useState("");
  const [onlyNoteOns, setOnlyNoteOns] = useState(false);
  const [effect, setEffect] = useState<Effect>({ effectCode: [0, 0], effectData: [0, 0]});
  const editing = useStore((state) => state.editorState.editMode) === "setMultipleEffects";
  const effectLaneEditRef = useRef<HTMLInputElement>(null);
  const effectValueEditRef = useRef<HTMLInputElement>(null);
  const onlyNoteOnsRef = useRef<HTMLInputElement>(null);

  const apply = () => {
    if (effect.effectCode[0] > 0 || effect.effectCode[1] > 0) {
      commands.setMultipleEffects(effectLane - 1, effect, onlyNoteOns);
    }
    editor.restorePreviousEditMode();
  };

  const cancel = () => {
    editor.restorePreviousEditMode();
  };

  const handleKeys: KeyHandler = (e) => {
    if (e.code === 'Escape') {
      cancel();
      return true;
    } else if (e.code === 'KeyS' && primaryModifier(e)) {
      apply();
      return true;
    } else if (e.code === 'Digit1' && primaryModifier(e)) {
      effectLaneEditRef.current?.focus();
      return true;
    } else if (e.code === 'Digit2' && primaryModifier(e)) {
      effectValueEditRef.current?.focus();
      return true;
    } else if (e.code === 'Digit3' && primaryModifier(e)) {
      onlyNoteOnsRef.current?.focus();
      return true;
    }
    return false;
  };

  useKeyboardHandler(handleKeys, editing);

  useEffect(() => {
    if (!editing) return;
    setInputEffectLane("1");
    setInputEffectValue("");
    setEffect({ effectCode: [0, 0], effectData: [0, 0]});
    setOnlyNoteOns(false);
  }, [editing]);

  if (!editing) return null;

  const validateEffectLane = () => {
    const numberValue = Number(inputEffectLane);
    if (isNaN(numberValue) || numberValue < 1 || numberValue > 4) {
      void alerting.showError(message("invalidEffectLane"));
      setInputEffectLane(effectLane.toString());
      return;
    }
    setEffectLane(numberValue);
  }

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
    setInputEffectValue(inputEffectValue.toUpperCase());
  };

  return (
    <Modal className="setMultipleEffects">
      <FieldLabel
        className="effectLaneLabel"
        associatedField="effectLaneEdit"
        text={message("effectLaneLabel")}
        hotkey={1}
      />
      <div className="effectLaneEdit">
        <div className="uiArea padded rounded spinner">
          <input
            type="text"
            maxLength={1}
            id="effectLaneEdit"
            value={inputEffectLane}
            onChange={(e) => setInputEffectLane(e.target.value)}
            onBlur={validateEffectLane}
            ref={effectLaneEditRef}
          />
          <div className="spinnerButtons">
            <IncrementButton
              onClick={() => {
                if (effectLane == 4) return;
                setInputEffectLane((effectLane + 1).toString());
                setEffectLane(effectLane + 1);
              }}
            />
            <DecrementButton
              onClick={() => {
                if (effectLane == 1) return;
                setInputEffectLane((effectLane - 1).toString());
                setEffectLane(effectLane - 1);
              }}
            />
          </div>
        </div>
      </div>
      <FieldLabel
        className="effectValueLabel"
        associatedField="effectValueEdit"
        text={message("effectValueLabel")}
        hotkey={2}
      />
      <div className="effectValueEdit">
        <div className="uiArea padded rounded">
          <input
            type="text"
            maxLength={4}
            id="effectValueEdit"
            value={inputEffectValue}
            onChange={(e) => setInputEffectValue(e.target.value)}
            onBlur={validateEffectValue}
            ref={effectValueEditRef}
          />
        </div>
      </div>
      <FieldLabel
        className="onlyNoteOnsLabel"
        associatedField={onlyNoteOns ? "onlyNoteOnsYes" : "onlyNoteOnsNo"}
        text={message("onlyNoteOnsLabel")}
        hotkey={3}
      />
      <div className="onlyNoteOnsEdit radio">
        <input
          type="radio"
          id="onlyNoteOnsYes"
          name="onlyNoteOns"
          checked={onlyNoteOns}
          onChange={() => setOnlyNoteOns(true)}
          ref={onlyNoteOns ? onlyNoteOnsRef : null}
        />
        <label htmlFor="onlyNoteOnsYes">{message("yes")}</label>
        <input
          type="radio"
          id="onlyNoteOnsNo"
          name="onlyNoteOns"
          checked={!onlyNoteOns}
          onChange={() => setOnlyNoteOns(false)}
          ref={onlyNoteOns ? null : onlyNoteOnsRef}
        />
        <label htmlFor="onlyNoteOnsNo">{message("no")}</label>
      </div>
      <div className="saveCloseButtons">
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