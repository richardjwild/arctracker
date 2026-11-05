import { useStore } from "../../../../store/useStore.ts";
import { useEffect, useRef, useState } from "react";
import Modal from "../../../components/Modal/Modal.tsx";
import { MaxBeatsPerMinute, MaxLinesPerBeat, MinBeatsPerMinute, tempo } from "../../../../editing/tempo.ts";
import { alerting } from "../../../../alerting/alert.ts";
import "./EditTempo.css";
import { commands } from "../../../../control/commands.ts";
import { message } from "../../../../language/messages.ts";
import { primaryModifier } from "../../../../keyboard/keyBinding.ts";
import { KeyHandler } from "../../../../keyboard/keyHandler.ts";
import FieldLabel from "../../../components/FieldLabel/FieldLabel.tsx";
import { useKeyboardHandler } from "../../../../hooks/useKeyboardHandler.ts";
import { getSpinnerKeyboardEventHandler, SpinnerButtons } from "../../../components/SpinnerButtons/SpinnerButtons.tsx";

const LinesPerBeatMaxLength = 3;
const TempoMaxLength = 3;
const emptyInput = { linesPerBeat: "", beatsPerMinute: "" };

export default function EditTempo() {
  const editing = useStore((state) => state.editorState.editMode) === "tempo";
  const [inputState, setInputState] = useState(emptyInput);
  const module = useStore((state) => state.module);
  const draftTempo = useStore((state) => state.draftTempo);
  const setDraftTempo = useStore((state) => state.setDraftTempo);
  const linesPerBeatInputRef = useRef<HTMLInputElement>(null);
  const beatsPerMinuteInputRef = useRef<HTMLInputElement>(null);

  const handleKeys: KeyHandler = (e) => {
    if (e.code === 'Escape') {
      tempo.hideDialog();
      return true;
    } else if (e.code === 'KeyS' && primaryModifier(e)) {
      commands.setTempo();
      return true;
    } else if (e.code === 'Digit1' && primaryModifier(e)) {
      linesPerBeatInputRef.current?.focus();
      return true;
    } else if (e.code === 'Digit2' && primaryModifier(e)) {
      beatsPerMinuteInputRef.current?.focus();
      return true;
    }
    return false;
  };

  useKeyboardHandler(handleKeys, editing);

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
      linesPerBeat <= MaxLinesPerBeat
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

  const incrementLinesPerBeat = () => {
    let linesPerBeat = draftTempo.linesPerBeat;
    if (linesPerBeat === 0 || linesPerBeat === MaxLinesPerBeat) return;
    linesPerBeat += 1;
    setDraftTempo({
      ...draftTempo,
      linesPerBeat,
    });
    setInputState({
      ...inputState,
      linesPerBeat: linesPerBeat.toString(),
    });
  };

  const decrementLinesPerBeat = () => {
    let linesPerBeat = draftTempo.linesPerBeat;
    if (linesPerBeat <= 1) return;
    linesPerBeat -= 1;
    setDraftTempo({
      ...draftTempo,
      linesPerBeat,
    });
    setInputState({
      ...inputState,
      linesPerBeat: linesPerBeat.toString(),
    });
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
    if (beatsPerMinute >= 0 && beatsPerMinute <= MaxBeatsPerMinute) {
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

  const incrementBeatsPerMinute = () => {
    let beatsPerMinute = draftTempo.beatsPerMinute;
    if (beatsPerMinute === 0 || beatsPerMinute === MaxBeatsPerMinute) return;
    beatsPerMinute += 1;
    setDraftTempo({
      ...draftTempo,
      beatsPerMinute,
    });
    setInputState({
      ...inputState,
      beatsPerMinute: beatsPerMinute.toString(),
    });
  };

  const decrementBeatsPerMinute = () => {
    let beatsPerMinute = draftTempo.beatsPerMinute;
    if (beatsPerMinute <= MinBeatsPerMinute) return;
    beatsPerMinute -= 1;
    setDraftTempo({
      ...draftTempo,
      beatsPerMinute,
    });
    setInputState({
      ...inputState,
      beatsPerMinute: beatsPerMinute.toString(),
    });
  };

  if (!editing) return null;

  return (
    <Modal className="editTempo">
      <FieldLabel
        className="linesPerBeatLabel"
        associatedField="linesPerBeatInput"
        text={message("linesPerBeatLabel")}
        hotkey={1}
      />
      <div className="linesPerBeatEdit uiArea padded rounded">
        <input
          type="text"
          id="linesPerBeatInput"
          maxLength={LinesPerBeatMaxLength}
          value={inputState.linesPerBeat}
          onBlur={validateLinesPerBeat}
          onChange={(e) =>
            setInputState({
              linesPerBeat: e.target.value,
              beatsPerMinute: inputState.beatsPerMinute,
            })
          }
          onKeyDown={getSpinnerKeyboardEventHandler(
            incrementLinesPerBeat,
            decrementLinesPerBeat,
          )}
          ref={linesPerBeatInputRef}
        />
        <SpinnerButtons
          incrementFn={incrementLinesPerBeat}
          decrementFn={decrementLinesPerBeat}
        />
      </div>
      <FieldLabel
        className="beatsPerMinuteLabel"
        associatedField="beatsPerMinuteInput"
        text={message("beatsPerMinuteLabel")}
        hotkey={2}
      />
      <div className="beatsPerMinuteEdit uiArea padded rounded">
        <input
          type="text"
          id="beatsPerMinuteInput"
          maxLength={TempoMaxLength}
          value={inputState.beatsPerMinute}
          onBlur={validateBeatsPerMinute}
          onChange={(e) =>
            setInputState({
              linesPerBeat: inputState.linesPerBeat,
              beatsPerMinute: e.target.value,
            })
          }
          onKeyDown={getSpinnerKeyboardEventHandler(
            incrementBeatsPerMinute,
            decrementBeatsPerMinute,
          )}
          ref={beatsPerMinuteInputRef}
        />
        <SpinnerButtons
          incrementFn={incrementBeatsPerMinute}
          decrementFn={decrementBeatsPerMinute}
        />
      </div>
      <div className="saveCloseButtons">
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
