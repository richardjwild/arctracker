import Modal from "../../../components/Modal/Modal.tsx";
import { commands } from "../../../../control/commands.ts";
import { useEffect, useRef, useState } from "react";
import { alerting } from "../../../../alerting/alert.ts";
import { useStore } from "../../../../store/useStore.ts";
import "./EditTrackCount.css";
import { message } from "../../../../language/messages.ts";
import {
  getSpinnerKeyboardEventHandler,
  SpinnerButtons,
} from "../../../components/SpinnerButtons/SpinnerButtons.tsx";
import { editor } from "../../../../editing/editor.ts";
import { MaxTrackCount } from "../../../../module/module.ts";

export default function EditTrackCount() {
  const editingTrackCount =
    useStore((state) => state.editorState.editMode) === "trackCount";
  const [inputTrackCount, setInputTrackCount] = useState("");
  const module = useStore((state) => state.module);
  const trackCountInputRef = useRef<HTMLInputElement>(null);
  useEffect(() => {
    if (!editingTrackCount) return;
    setInputTrackCount(module.numTracks.toString());
    trackCountInputRef.current?.focus();
  }, [editingTrackCount]);

  const validateTrackCount = (): number | null => {
    const trackCount = Number(inputTrackCount);
    if (
      Number.isInteger(trackCount) &&
      trackCount >= 1 &&
      trackCount <= MaxTrackCount
    ) {
      return trackCount;
    } else {
      return null;
    }
  };

  const incrementTrackCount = () => {
    const trackCount = Number(inputTrackCount);
    if (trackCount === MaxTrackCount) return;
    setInputTrackCount((trackCount + 1).toString());
  };

  const decrementTrackCount = () => {
    const trackCount = Number(inputTrackCount);
    if (trackCount === 1) return;
    setInputTrackCount((trackCount - 1).toString());
  };

  const keyHandler = (e: React.KeyboardEvent) => {
    if (e.code === 'Enter') {
      const trackCount = validateTrackCount();
      if (trackCount) {
        if (trackCount !== module.numTracks) commands.setTrackCount(trackCount);
        editor.restorePreviousEditMode();
      } else {
        setInputTrackCount(module.numTracks.toString());
        void alerting.showInfo(message("invalidTrackCount")).then(() => trackCountInputRef.current?.focus());
      }
      return true;
    }
    if (e.code === 'Escape') {
      editor.restorePreviousEditMode();
      return true;
    }
    return false;
  };

  if (!editingTrackCount) return null;

  return (
    <Modal className="editTrackCount">
      <label htmlFor="trackCount">{message("trackCountLabel")}</label>
      <div className="editTrackCountInput uiArea padded rounded">
        <input
          className="noFocusStyle"
          type="text"
          id="patternLength"
          ref={trackCountInputRef}
          maxLength={3}
          value={inputTrackCount}
          onKeyDown={getSpinnerKeyboardEventHandler(
            incrementTrackCount,
            decrementTrackCount,
            keyHandler,
          )}
          onChange={(e) => setInputTrackCount(e.target.value)}
        />
        <SpinnerButtons
          incrementFn={incrementTrackCount}
          decrementFn={decrementTrackCount}
        />
      </div>
    </Modal>
  );
}
