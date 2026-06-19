import Modal from "./Modal.tsx";
import "./SampleEditDialog.css";
import { useStore } from "../store/useStore.ts";
import { hexadecimal } from "../rendering/hexadecimal.ts";
import { editor } from "../editing/editor.ts";
import { commands } from "../control/commands.ts";
import { useEffect, useRef, useState } from "react";
import { alerting } from "../alerting/alert.ts";
import { editInstrument, emptyInstrument, SampleNameMaxLength } from "../editing/editInstrument.ts";

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

export default function SampleEditDialog() {
  const instrumentIndex = useStore((state) => state.selectedInstrument);
  const instruments = useStore((state) => state.module.instruments);
  const instrumentEditing = useStore((state) => state.editorState.instrumentEditing);
  const { draftInstrument, setDraftInstrument } = useStore((state) => state);
  const [ inputState, setInputState ] = useState(emptyInputState);
  const modalRef = useRef<HTMLDivElement>(null);
  const loseFocus = () => modalRef.current?.focus();

  const syncInputStateWithDraft = () => {
    setInputState({
      transpose: (draftInstrument.transpose - 13).toString(),
      repeatStart: draftInstrument.repeatOffset.toString(),
      repeatEnd: (draftInstrument.repeatOffset + draftInstrument.repeatLength).toString()
    });
  };

  useEffect(() => {
    if (!instrumentEditing) return;
    const instrument =
      instrumentIndex === null || instrumentIndex >= instruments.length
        ? emptyInstrument()
        : instruments[instrumentIndex];
    setDraftInstrument({ ...instrument, sample: { ...instrument.sample } });
  }, [instruments, instrumentIndex, instrumentEditing]);

  useEffect(() => {
    syncInputStateWithDraft();
    void editInstrument.auditionInstrument();
  }, [draftInstrument]);

  if (instrumentIndex === null || !instrumentEditing) return null;

  const validateTranspose = () => {
    const transpose = Number(inputState.transpose);
    if (Number.isInteger(transpose) && transpose >= -12 && transpose <= 12) {
      setDraftInstrument({ ...draftInstrument, transpose: transpose + 13 });
    } else {
      syncInputStateWithDraft();
      void alerting.showInfo("Transpose must be between -12 and 12.");
    }
    loseFocus();
  };

  const setSampleRepeats = () => {
    setDraftInstrument({
      ...draftInstrument,
      repeats: true,
      repeatOffset: 0,
      repeatLength: draftInstrument.sample.sampleLength,
    });
  };

  const setSampleNoRepeat = () => {
    setDraftInstrument({
      ...draftInstrument,
      repeats: false,
      repeatOffset: 0,
      repeatLength: 0,
    })
  };

  const setRepeatOffsetAndLength = (repeatStart: number) => {
    if (draftInstrument.repeatLength === 0) {
      setDraftInstrument({
        ...draftInstrument,
        repeatOffset: repeatStart,
        repeatLength: draftInstrument.sample.sampleLength - (repeatStart + 1)
      });
    } else {
      const delta = repeatStart - draftInstrument.repeatOffset;
      setDraftInstrument({
        ...draftInstrument,
        repeatOffset: repeatStart,
        repeatLength: draftInstrument.repeatLength - delta
      });
    }
  };

  const updateRepeatStart = () => {
    const repeatStart = Number(inputState.repeatStart);
    const maxValue =
      draftInstrument.repeatLength === 0
        ? draftInstrument.sample.sampleLength - 2
        : draftInstrument.repeatOffset + draftInstrument.repeatLength - 1;
    if (Number.isInteger(repeatStart) && repeatStart >= 0 && repeatStart <= maxValue) {
      setRepeatOffsetAndLength(repeatStart);
    } else {
      syncInputStateWithDraft();
      void alerting.showInfo(
        `Repeat start must be between 0 and ${draftInstrument.repeatLength === 0 ? "sample length" : "repeat end"}.`
      );
    }
    loseFocus();
  };

  const updateRepeatEnd = () => {
    const repeatEnd = Number(inputState.repeatEnd);
    if (Number.isInteger(repeatEnd) && repeatEnd > draftInstrument.repeatOffset && repeatEnd < draftInstrument.sample.sampleLength) {
      setDraftInstrument({ ...draftInstrument, repeatLength: repeatEnd - draftInstrument.repeatOffset });
    } else {
      syncInputStateWithDraft();
      void alerting.showInfo(
        "Repeat end must be between repeat start and sample length."
      );
    }
    loseFocus();
  };

  return (
    <Modal ref={modalRef} className="sampleEdit">
      <h1 className="instrumentEditTitle padded">
        Instrument {hexadecimal.toHex(instrumentIndex + 1, 2)}
      </h1>
      <div className="sampleNameLabel padded sampleEditLabel">
        <label>Name:</label>
      </div>
      <div className="sampleNameEdit uiArea padded sampleEditField">
        <input
          type="text"
          maxLength={SampleNameMaxLength}
          value={draftInstrument.name}
          onFocus={editor.startTextInput}
          onBlur={editor.stopTextInput}
          onChange={(e) => {
            setDraftInstrument({ ...draftInstrument, name: e.target.value });
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
          value={draftInstrument.defaultVolume}
          onChange={(e) => {
            setDraftInstrument({
              ...draftInstrument,
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
        <input type="text" readOnly value={draftInstrument.sample.sampleLength} />
      </div>
      <div className="sampleRepeatsLabel padded sampleEditLabel">
        <label>Sample Loops:</label>
      </div>
      <div className="sampleRepeatsEdit padded sampleEditField">
        <input
          type="radio"
          id="sampleRepeatsYes"
          name="sampleRepeats"
          checked={draftInstrument.repeats}
          onChange={setSampleRepeats}
        />
        <label htmlFor="sampleRepeatsYes">Yes</label>
        <input
          type="radio"
          id="sampleRepeatsNo"
          name="sampleRepeats"
          checked={!draftInstrument.repeats}
          onChange={setSampleNoRepeat}
        />
        <label htmlFor="sampleRepeatsNo">No</label>
      </div>
      <div className="repeatStartLabel padded sampleEditLabel">
        <label>Loop Start:</label>
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
        <label>Loop End:</label>
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
        <button
          type="button"
          onClick={commands.loadSample}
        >
          Load Sample
        </button>
        <button type="button">Delete Sample</button>
        <button
          type="button"
          onClick={commands.saveAndCloseInstrumentEditor}
        >
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
