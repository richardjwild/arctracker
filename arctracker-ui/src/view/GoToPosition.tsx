import { useStore } from "../store/useStore.ts";
import "./GoToPosition.css"
import { useEffect, useRef, useState } from "react";
import { alerting } from "../alerting/alert.ts";
import { commands } from "../control/commands.ts";
import { message } from "../language/messages.ts";
import Modal from "./Modal.tsx";
import { editor } from "../editing/editor.ts";
import FieldLabel from "./components/FieldLabel.tsx";
import { KeyHandler } from "../keyboard/keyHandler.ts";
import { useKeyboardHandler } from "../hooks/useKeyboardHandler.ts";

export default function GoToPosition() {
  const dialogVisible = useStore((state) => state.editorState.editMode) === "goToPosition";
  const sequenceLength = useStore((state) => state.sequence.length);
  const currentPosition = useStore((state) => state.editorState.sequencePosition) + 1;
  const [inputNewPosition, setInputNewPosition] = useState("");

  const handleKeys: KeyHandler = (e) => {
    if (e.code === 'Escape') {
      editor.restorePreviousEditMode();
      return true;
    }
    return false;
  }

  useKeyboardHandler(handleKeys, dialogVisible);

  const newPositionInputRef = useRef<HTMLInputElement>(null);
  useEffect(() => {
    if (!dialogVisible) return;
    setInputNewPosition("");
    newPositionInputRef.current?.focus();
  }, [dialogVisible]);

  const validateNewPosition = (): number | null => {
    if (inputNewPosition === "") {
      setInputNewPosition(currentPosition.toString());
      return currentPosition;
    }
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