import { useStore } from "../store/useStore.ts";
import "./GoToPosition.css"
import { useEffect, useRef, useState } from "react";
import { alerting } from "../alerting/alert.ts";
import { commands } from "../control/commands.ts";
import { message } from "../language/messages.ts";
import Modal from "./Modal.tsx";
import { editor } from "../editing/editor.ts";
import FieldLabel from "./components/FieldLabel.tsx";

export default function GoToPosition() {
  const dialogVisible = useStore((state) => state.editorState.editMode) === "goToPosition";
  const [inputNewPosition, setInputNewPosition] = useState("");
  const sequenceLength = useStore((state) => state.sequence.length);

  const newPositionInputRef = useRef<HTMLInputElement>(null);
  useEffect(() => {
    if (!dialogVisible) return;
    setInputNewPosition("");
    newPositionInputRef.current?.focus();
  }, [dialogVisible]);

  const validateNewPosition = (): number | null => {
    const newPosition = Number(inputNewPosition);
    if (Number.isInteger(newPosition) && newPosition >= 1 && newPosition <= sequenceLength) {
      return newPosition;
    } else {
      setInputNewPosition("");
      void alerting.showInfo(message("invalidSequencePosition"));
      return null;
    }
  }

  if (!dialogVisible) return null;

  return (
    <Modal className="goToPosition">
      <FieldLabel className="newPositionLabel" text={message("newPositionLabel")} associatedField="newPosition" />
      <div className="uiArea padded rounded">
        <input
          className="noFocusStyle"
          type="text"
          id="newPosition"
          ref={newPositionInputRef}
          maxLength={5}
          value={inputNewPosition}
          onBlur={(e) => {
            e.preventDefault();
            const newPosition = validateNewPosition();
            if (newPosition) {
              commands.sequenceSeek(newPosition - 1);
              editor.restorePreviousEditMode();
            } else {
              newPositionInputRef.current?.focus();
            }
          }}
          onChange={(e) => setInputNewPosition(e.target.value)}
        />
      </div>
    </Modal>
  );
}