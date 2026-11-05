import { useStore } from "../../../../store/useStore.ts";
import "./EditPatternLength.css";
import Modal from "../../../components/Modal/Modal.tsx";
import { useEffect, useRef, useState } from "react";
import { alerting } from "../../../../alerting/alert.ts";
import { commands } from "../../../../control/commands.ts";
import { message } from "../../../../language/messages.ts";
import { getSpinnerKeyboardEventHandler, SpinnerButtons } from "../../../components/SpinnerButtons/SpinnerButtons.tsx";
import { editor } from "../../../../editing/editor.ts";

const PatternLengthMax = 1000;

export default function EditPatternLength() {
  const editingPatternLength =
    useStore((state) => state.editorState.editMode) === "patternLength";
  const [inputPatternLength, setInputPatternLength] = useState("");
  const sequenceIndex = useStore((state) => state.editorState.sequencePosition);
  const patternNo = useStore((state) => state.sequence)[sequenceIndex];
  const module = useStore((state) => state.module);
  const currentLength = module.patternLengths[patternNo];

  const patternLengthInputRef = useRef<HTMLInputElement>(null);
  useEffect(() => {
    if (!editingPatternLength) return;
    setInputPatternLength(currentLength.toString());
    patternLengthInputRef.current?.focus();
  }, [editingPatternLength]);

  const validatePatternLength = (): number | null => {
    const patternLength = Number(inputPatternLength);
    if (Number.isInteger(patternLength) && patternLength >= 1 && patternLength <= PatternLengthMax) {
      return patternLength;
    } else {
      return null;
    }
  };

  const keyHandler = (e: React.KeyboardEvent) => {
    if (e.code === 'Enter') {
      const patternLength = validatePatternLength();
      if (patternLength) {
        if (patternLength !== currentLength) commands.setCurrentPatternLength(patternLength);
        editor.restorePreviousEditMode();
      } else {
        setInputPatternLength(currentLength.toString());
        alerting.showInfo(message("invalidPatternLength")).then(() => patternLengthInputRef.current?.focus());
      }
      return true;
    }
    if (e.code === 'Escape') {
      editor.restorePreviousEditMode();
      return true;
    }
    return false;
  };

  if (!editingPatternLength) return null;

  const incrementPatternLength = () => {
    const patternLength = Number(inputPatternLength);
    if (patternLength === PatternLengthMax) return;
    setInputPatternLength((patternLength + 1).toString());
  };

  const decrementPatternLength = () => {
    const patternLength = Number(inputPatternLength);
    if (patternLength === 1) return;
    setInputPatternLength((patternLength - 1).toString());
  };

  return (
    <Modal className="editPatternLength">
      <label className="patternLengthLabel" htmlFor="patternLength">{message("patternLengthLabel")}</label>
      <div className="patternLengthInput uiArea padded rounded">
        <input
          className="noFocusStyle"
          type="text"
          id="patternLength"
          ref={patternLengthInputRef}
          maxLength={4}
          value={inputPatternLength}
          onKeyDown={getSpinnerKeyboardEventHandler(
            incrementPatternLength,
            decrementPatternLength,
            keyHandler,
          )}
          onChange={(e) => setInputPatternLength(e.target.value)}
        />
        <SpinnerButtons
          incrementFn={incrementPatternLength}
          decrementFn={decrementPatternLength}
        />
      </div>
    </Modal>
  );
}
