import { useStore } from "../../../../store/useStore.ts";
import "./GoToPosition.css"
import { useEffect, useRef, useState } from "react";
import { alerting } from "../../../../alerting/alert.ts";
import { commands } from "../../../../control/commands.ts";
import { message } from "../../../../language/messages.ts";
import Modal from "../../../components/Modal/Modal.tsx";
import { editor } from "../../../../editing/editor.ts";
import FieldLabel from "../../../components/FieldLabel/FieldLabel.tsx";
import { KeyHandler } from "../../../../keyboard/keyHandler.ts";
import { useKeyboardHandler } from "../../../../hooks/useKeyboardHandler.ts";
import { getSpinnerKeyboardEventHandler, SpinnerButtons } from "../../../components/SpinnerButtons/SpinnerButtons.tsx";

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
    setInputNewPosition(currentPosition.toString());
    newPositionInputRef.current?.focus();
  }, [dialogVisible]);

  const validateNewPosition = (): number | null => {
    const newPosition = Number(inputNewPosition);
    if (Number.isInteger(newPosition) && newPosition >= 1 && newPosition <= sequenceLength) {
      return newPosition;
    } else {
      return null;
    }
  }

  const keyHandler = (e: React.KeyboardEvent) => {
    if (e.code === 'Enter') {
      const newPosition = validateNewPosition();
      if (newPosition) {
        commands.sequenceSeek(newPosition - 1);
        editor.restorePreviousEditMode();
      } else {
        setInputNewPosition(currentPosition.toString());
        alerting.showInfo(message("invalidSequencePosition")).then(() => newPositionInputRef.current?.focus());
      }
      return true;
    }
    if (e.code === 'Escape') {
      editor.restorePreviousEditMode();
      return true;
    }
    return false;
  };

  const incrementNewPosition = () => {
    const newPosition = Number(inputNewPosition);
    if (newPosition === sequenceLength) return;
    setInputNewPosition((newPosition + 1).toString());
  };

  const decrementNewPosition = () => {
    const newPosition = Number(inputNewPosition);
    if (newPosition === 1) return;
    setInputNewPosition((newPosition - 1).toString());
  };

  if (!dialogVisible) return null;

  return (
    <Modal className="goToPosition">
      <FieldLabel className="newPositionLabel" text={message("newPositionLabel")} associatedField="newPosition" />
      <div className="newPositionInput uiArea padded rounded">
        <input
          className="noFocusStyle"
          type="text"
          id="newPosition"
          ref={newPositionInputRef}
          maxLength={5}
          value={inputNewPosition}
          onKeyDown={getSpinnerKeyboardEventHandler(
            incrementNewPosition,
            decrementNewPosition,
            keyHandler,
          )}
          onChange={(e) => setInputNewPosition(e.target.value)}
        />
        <SpinnerButtons
          incrementFn={incrementNewPosition}
          decrementFn={decrementNewPosition}
        />
      </div>
    </Modal>
  );
}