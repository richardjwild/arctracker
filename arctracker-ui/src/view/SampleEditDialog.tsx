import Modal from "./Modal.tsx";
import "./SampleEditDialog.css";
import { useStore } from "../store/useStore.ts";
import { hexadecimal } from "../rendering/hexadecimal.ts";
import { editor } from "../editing/editor.ts";
import { commands } from "../control/commands.ts";
import { useEffect, useRef, useState } from "react";
import { Instrument } from "../engine/engine.ts";
import { alerting } from "../alerting/alert.ts";

type InputState = {
  transpose: string;
  repeatStart: string;
  repeatEnd: string;
};

const emptyInputState: InputState = {
  transpose: "0",
  repeatStart: "0",
  repeatEnd: "0"
};

const emptyInstrument: Instrument = {
  name: "",
  assigned: false,
  defaultVolume: 255,
  transpose: 13,
  repeats: false,
  repeatOffset: 0,
  repeatLength: 0,
  sampleIndex: 0,
  sample: { sampleLength: 0 }
};

const SampleNameMaxLength = 33;

export default function SampleEditDialog() {
  const instrumentIndex = useStore((state) => state.selectedInstrument);
  const instruments = useStore((state) => state.module.instruments);
  const instrumentEditing = useStore((state) => state.editorState.instrumentEditing);
  const modalRef = useRef<HTMLDivElement>(null);
  const [draft, setDraft] = useState(emptyInstrument);
  const [inputState, setInputState] = useState(emptyInputState);
  const loseFocus = () => modalRef.current?.focus();

  const syncInputStateWithDraft = () => {
    setInputState({
      transpose: (draft.transpose - 13).toString(),
      repeatStart: draft.repeatOffset.toString(),
      repeatEnd: (draft.repeatOffset + draft.repeatLength).toString()
    });
  };

  useEffect(() => {
    const instrument =
      instrumentIndex === null || instrumentIndex >= instruments.length
        ? emptyInstrument
        : instruments[instrumentIndex];
    setDraft({ ...instrument, sample: { ...instrument.sample } });
  }, [instruments, instrumentIndex]);

  useEffect(() => {
    syncInputStateWithDraft();
  }, [draft]);

  const validateTranspose = () => {
    const transpose = Number(inputState.transpose);
    if (Number.isInteger(transpose) && transpose >= -12 && transpose <= 12) {
      setDraft({ ...draft, transpose: transpose + 13 });
    } else {
      syncInputStateWithDraft();
      void alerting.showInfo("Transpose must be between -12 and 12.");
    }
    loseFocus();
  };

  const setRepeatOffsetAndLength = (repeatStart: number) => {
    if (draft.repeatLength === 0) {
      setDraft({
        ...draft,
        repeatOffset: repeatStart,
        repeatLength: draft.sample.sampleLength - (repeatStart + 1)
      });
    } else {
      const delta = repeatStart - draft.repeatOffset;
      setDraft({
        ...draft,
        repeatOffset: repeatStart,
        repeatLength: draft.repeatLength - delta
      });
    }
  };

  const updateRepeatStart = () => {
    const repeatStart = Number(inputState.repeatStart);
    const maxValue =
      draft.repeatLength === 0
        ? draft.sample.sampleLength - 2
        : draft.repeatOffset + draft.repeatLength - 1;
    if (Number.isInteger(repeatStart) && repeatStart >= 0 && repeatStart <= maxValue) {
      setRepeatOffsetAndLength(repeatStart);
    } else {
      syncInputStateWithDraft();
      void alerting.showInfo(
        `Repeat start must be between 0 and ${draft.repeatLength === 0 ? "sample length" : "repeat end"}.`
      );
    }
    loseFocus();
  };

  const updateRepeatEnd = () => {
    const repeatEnd = Number(inputState.repeatEnd);
    if (Number.isInteger(repeatEnd) && repeatEnd > draft.repeatOffset && repeatEnd < draft.sample.sampleLength) {
      setDraft({ ...draft, repeatLength: repeatEnd - draft.repeatOffset });
    } else {
      syncInputStateWithDraft();
      void alerting.showInfo(
        "Repeat end must be between repeat start and sample length."
      );
    }
    loseFocus();
  };

  const saveAndClose = () => {
    // TODO: Construct edit command and issue it to editor.applyEdit().
    console.log("save instrument", draft);
    commands.saveAndCloseInstrumentEditor();
  };

  if (!instrumentEditing) return null;
  return (
    <Modal ref={modalRef} className="sampleEdit">
      <h1 className="instrumentEditTitle padded">
        Instrument {hexadecimal.toHex((instrumentIndex || 0) + 1, 2)}
      </h1>
      <div className="sampleNameLabel padded sampleEditLabel">
        <label>Name:</label>
      </div>
      <div className="sampleNameEdit uiArea padded sampleEditField">
        <input
          type="text"
          maxLength={SampleNameMaxLength}
          value={draft.name}
          onFocus={editor.startTextInput}
          onBlur={editor.stopTextInput}
          onChange={(e) => {
            setDraft({ ...draft, name: e.target.value });
          }}
        />
      </div>
      <div className="defaultVolumeLabel padded sampleEditLabel">
        <label>Default Volume:</label>
      </div>
      <div className="defaultVolumeEdit uiArea padded sampleEditField">
        <input
          type="range"
          min={0}
          max={255}
          value={draft.defaultVolume}
          onChange={(e) => {
            setDraft({
              ...draft,
              defaultVolume: e.target.valueAsNumber
            });
          }}
        />
      </div>
      <div className="transposeLabel padded sampleEditLabel">
        <label>Transpose:</label>
      </div>
      <div className="transposeEdit uiArea padded sampleEditField">
        <input
          type="text"
          value={inputState.transpose}
          onFocus={editor.startTextInput}
          onChange={(e) =>
            setInputState({
              ...inputState,
              transpose: e.target.value
            })
          }
          onBlur={(e) => {
            e.preventDefault();
            editor.stopTextInput();
            validateTranspose();
          }}
        />
      </div>
      <div className="sampleLengthLabel padded sampleEditLabel">
        <label>Sample Length:</label>
      </div>
      <div className="sampleLengthEdit padded sampleEditField">
        <input type="text" readOnly defaultValue={draft.sample.sampleLength} />
      </div>
      <div className="repeatStartLabel padded sampleEditLabel">
        <label>Repeat Start:</label>
      </div>
      <div className="repeatStartEdit uiArea padded sampleEditField">
        <input
          type="text"
          value={inputState.repeatStart}
          onFocus={editor.startTextInput}
          onChange={(e) => setInputState({
            ...inputState,
            repeatStart: e.target.value
          })}
          onBlur={(e) => {
            e.preventDefault();
            editor.stopTextInput();
            updateRepeatStart();
          }}
        />
      </div>
      <div className="repeatEndLabel padded sampleEditLabel">
        <label>Repeat End:</label>
      </div>
      <div className="repeatEndEdit uiArea padded sampleEditField">
        <input
          type="text"
          value={inputState.repeatEnd}
          onFocus={editor.startTextInput}
          onChange={(e) => setInputState({
            ...inputState,
            repeatEnd: e.target.value
          })}
          onBlur={(e) => {
            e.preventDefault();
            editor.stopTextInput();
            updateRepeatEnd();
          }}
        />
      </div>
      <div className="saveCloseButtons uiArea padded">
        <button type="button">Load Sample</button>
        <button type="button">Delete Sample</button>
        <button type="button" onClick={saveAndClose}>
          Save Changes
        </button>
        <button
          type="button"
          onClick={commands.restoreAndCloseInstrumentEditor}
        >
          Close
        </button>
      </div>
    </Modal>
  );
}
