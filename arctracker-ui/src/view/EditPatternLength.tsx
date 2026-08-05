import { useStore } from "../store/useStore.ts";
import "./EditPatternLength.css";
import Modal from "./Modal.tsx";
import { useEffect, useRef, useState } from "react";
import { editor } from "../editing/editor.ts";
import { alerting } from "../alerting/alert.ts";
import { commands } from "../control/commands.ts";
import { message } from "../language/messages.ts";

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
    if (Number.isInteger(patternLength) && patternLength >= 1 && patternLength <= 1000) {
      return patternLength;
    } else {
      setInputPatternLength(currentLength.toString());
      void alerting.showInfo(message("invalidPatternLength"));
      return null;
    }
  };

  if (!editingPatternLength) return null;

  return (
    <Modal className="editPatternLength">
      <label htmlFor="patternLength">{message("patternLengthLabel")}</label>
      <div className="uiArea padded rounded">
        <input
          type="text"
          id="patternLength"
          ref={patternLengthInputRef}
          maxLength={4}
          value={inputPatternLength}
          onFocus={editor.startTextInput}
          onBlur={(e) => {
            e.preventDefault();
            const patternLength = validatePatternLength();
            if (patternLength) {
              commands.setCurrentPatternLength(patternLength);
              editor.stopTextInput();
            } else {
              patternLengthInputRef.current?.focus();
            }
          }}
          onChange={(e) => setInputPatternLength(e.target.value)}
        />
      </div>
    </Modal>
  );
}
