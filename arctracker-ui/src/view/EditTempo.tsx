import { useStore } from "../store/useStore.ts";
import { useEffect, useState } from "react";
import Modal from "./Modal.tsx";
import { editor } from "../editing/editor.ts";
import { tempo } from "../editing/tempo.ts";
import { alerting } from "../alerting/alert.ts";
import "./EditTempo.css";
import { commands } from "../control/commands.ts";
import { message } from "../language/messages.ts";

const LinesPerBeatMaxLength = 2;
const TempoMaxLength = 3;
const emptyInput = { linesPerBeat: "", beatsPerMinute: "" };

export default function EditTempo() {
  const editing = useStore((state) => state.editorState.editMode) === "tempo";
  const [inputState, setInputState] = useState(emptyInput);
  const module = useStore((state) => state.module);
  const draftTempo = useStore((state) => state.draftTempo);
  const setDraftTempo = useStore((state) => state.setDraftTempo);

  useEffect(() => {
    if (!editing) return;
    setDraftTempo({
      linesPerBeat: module.linesPerBeat,
      beatsPerMinute: module.beatsPerMinute,
    });
    setInputState({
      linesPerBeat: module.linesPerBeat ? module.linesPerBeat.toString() : "",
      beatsPerMinute: module.beatsPerMinute
        ? module.beatsPerMinute.toString()
        : "",
    });
  }, [module, editing]);

  const validateLinesPerBeat = () => {
    const linesPerBeat =
      inputState.linesPerBeat === "" ? 0 : Number(inputState.linesPerBeat);
    if (
      Number.isInteger(linesPerBeat) &&
      linesPerBeat >= 0 &&
      linesPerBeat <= 255
    ) {
      setDraftTempo({
        linesPerBeat,
        beatsPerMinute: linesPerBeat === 0 ? 0 : draftTempo.beatsPerMinute,
      });
      setInputState({
        linesPerBeat: linesPerBeat === 0 ? "" : linesPerBeat.toString(),
        beatsPerMinute:
          (linesPerBeat === 0 || draftTempo.beatsPerMinute === 0) ? "" : draftTempo.beatsPerMinute.toString(),
      });
    } else {
      void alerting.showInfo(message("invalidLinesPerBeat"));
      setInputState({
        ...inputState,
        linesPerBeat: "",
      });
    }
  };

  const validateBeatsPerMinute = () => {
    const beatsPerMinute =
      inputState.beatsPerMinute === "" ? 0 : Number(inputState.beatsPerMinute);
    if (draftTempo.linesPerBeat === 0 && beatsPerMinute !== 0) {
      void alerting.showInfo(message("tempoUndefinedWithoutLinesPerBeat"));
      setInputState({
        ...inputState,
        beatsPerMinute: "",
      });
      return;
    }
    if (beatsPerMinute >= 0 && beatsPerMinute <= 255) {
      setDraftTempo({
        ...draftTempo,
        beatsPerMinute,
      });
      setInputState({
        ...inputState,
        beatsPerMinute: beatsPerMinute.toString(),
      });
    } else {
      void alerting.showInfo(message("invalidTempo"));
      setInputState({
        ...inputState,
        beatsPerMinute: "",
      });
    }
  };

  if (!editing) return null;

  return (
    <Modal className="editTempo">
      <div className="linesPerBeatLabel">
        <label htmlFor="linesPerBeatInput">{message("linesPerBeatLabel")}</label>
      </div>
      <div className="linesPerBeatEdit uiArea padded rounded">
        <input
          type="text"
          id="linesPerBeatInput"
          maxLength={LinesPerBeatMaxLength}
          value={inputState.linesPerBeat}
          onFocus={editor.startTextInput}
          onBlur={() => {
            validateLinesPerBeat();
            editor.stopTextInput();
          }}
          onChange={(e) =>
            setInputState({
              linesPerBeat: e.target.value,
              beatsPerMinute: inputState.beatsPerMinute,
            })
          }
        />
      </div>
      <div className="beatsPerMinuteLabel">
        <label htmlFor="beatsPerMinuteInput">{message("beatsPerMinuteLabel")}</label>
      </div>
      <div className="beatsPerMinuteEdit uiArea padded rounded">
        <input
          type="text"
          id="beatsPerMinuteInput"
          maxLength={TempoMaxLength}
          value={inputState.beatsPerMinute}
          onFocus={editor.startTextInput}
          onBlur={() => {
            validateBeatsPerMinute();
            editor.stopTextInput();
          }}
          onChange={(e) =>
            setInputState({
              linesPerBeat: inputState.linesPerBeat,
              beatsPerMinute: e.target.value,
            })
          }
        />
      </div>
      <div className="saveCloseButtons uiArea padded rounded">
        <button type="button" onClick={commands.setTempo}>
          {message("saveButtonLabel")}
        </button>
        <button type="button" onClick={tempo.hideDialog}>
          {message("cancelButtonLabel")}
        </button>
      </div>
    </Modal>
  );
}
