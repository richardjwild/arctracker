import Modal from "./Modal.tsx";
import { editor } from "../editing/editor.ts";
import { commands } from "../control/commands.ts";
import { useEffect, useRef, useState } from "react";
import { alerting } from "../alerting/alert.ts";
import { useStore } from "../store/useStore.ts";
import './EditTrackCount.css';
import { message } from "../language/messages.ts";

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
    if (Number.isInteger(trackCount) && trackCount >= 1 && trackCount <= 256) {
      return trackCount;
    } else {
      setInputTrackCount(module.numTracks.toString());
      void alerting.showInfo(message("invalidTrackCount"));
      return null;
    }
  };

  if (!editingTrackCount) return null;

  return (
    <Modal className="editTrackCount">
      <label htmlFor="trackCount">{message("trackCountLabel")}</label>
      <div className="uiArea padded rounded">
        <input
          type="text"
          id="patternLength"
          ref={trackCountInputRef}
          maxLength={4}
          value={inputTrackCount}
          onFocus={editor.startTextInput}
          onBlur={(e) => {
            e.preventDefault();
            const trackCount = validateTrackCount();
            if (trackCount) {
              commands.setTrackCount(trackCount);
              editor.stopTextInput();
            } else {
              trackCountInputRef.current?.focus();
            }
          }}
          onChange={(e) => setInputTrackCount(e.target.value)}
        />
      </div>
    </Modal>
  );
}
