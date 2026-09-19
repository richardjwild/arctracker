import Modal from "./Modal.tsx";
import "./SampleEditDialog.css";
import { useStore } from "../store/useStore.ts";
import { hexadecimal } from "../rendering/hexadecimal.ts";
import { editor } from "../editing/editor.ts";
import { commands } from "../control/commands.ts";
import React, { useEffect, useRef, useState } from "react";
import { alerting } from "../alerting/alert.ts";
import {
  editInstrument,
  emptyInstrument, Instrument,
  SampleNameMaxLength
} from "../editing/editInstrument.ts";
import { message, messageFn } from "../language/messages.ts";
import { notes } from "../rendering/notes.ts";

type InputState = {
  transpose: string;
  repeatStart: string;
  repeatEnd: string;
};

type SpinnerButtonProps = {
  onClick: React.MouseEventHandler<HTMLButtonElement>;
};

const emptyInputState: InputState = {
  transpose: "0",
  repeatStart: "0",
  repeatEnd: "0",
};

export default function SampleEditDialog() {
  const instrumentIndex = useStore((state) => state.selectedInstrument);
  const instruments = useStore((state) => state.module.instruments);
  const instrumentEditing =
    useStore((state) => state.editorState.editMode) === "instrument";
  const { draftInstrument, setDraftInstrument } = useStore((state) => state);
  const [draftModified, setDraftModified] = useState(false);
  const [inputState, setInputState] = useState(emptyInputState);
  const modalRef = useRef<HTMLDivElement>(null);
  const loseFocus = () => modalRef.current?.focus();

  const syncInputStateWithDraft = () => {
    setInputState({
      transpose: (draftInstrument.transpose).toString(),
      repeatStart: draftInstrument.repeatOffset.toString(),
      repeatEnd: (
        draftInstrument.repeatOffset + draftInstrument.repeatLength
      ).toString(),
    });
  };

  useEffect(() => {
    if (!instrumentEditing) return;
    const instrument =
      instrumentIndex === null || instrumentIndex >= instruments.length
        ? emptyInstrument()
        : instruments[instrumentIndex];
    setDraftInstrument({ ...instrument, sample: { ...instrument.sample } });
    setDraftModified(false);
  }, [instruments, instrumentIndex, instrumentEditing]);

  const updateDraftInstrument = (updatedDraftInstrument: Instrument) => {
    setDraftInstrument(updatedDraftInstrument);
    setDraftModified(true);
  }

  useEffect(() => {
    syncInputStateWithDraft();
    if (draftModified) {
      void editInstrument.auditionInstrument();
    }
  }, [draftInstrument]);

  if (instrumentIndex === null || !instrumentEditing) return null;

  const validateTranspose = () => {
    const transpose = Number(inputState.transpose);
    if (Number.isInteger(transpose) && transpose >= -12 && transpose <= 12) {
      updateDraftInstrument({ ...draftInstrument, transpose });
    } else {
      syncInputStateWithDraft();
      void alerting.showInfo(message("invalidTranspose"));
    }
    loseFocus();
  };

  const setSampleRepeats = () => {
    updateDraftInstrument({
      ...draftInstrument,
      repeats: true,
      repeatOffset: 0,
      repeatLength: draftInstrument.sample.sampleLength,
    });
  };

  const setSampleNoRepeat = () => {
    updateDraftInstrument({
      ...draftInstrument,
      repeats: false,
      repeatOffset: 0,
      repeatLength: 0,
    });
  };

  const setRepeatOffsetAndLength = (repeatStart: number) => {
    if (draftInstrument.repeatLength === 0) {
      updateDraftInstrument({
        ...draftInstrument,
        repeatOffset: repeatStart,
        repeatLength: draftInstrument.sample.sampleLength - (repeatStart + 1),
      });
    } else {
      const delta = repeatStart - draftInstrument.repeatOffset;
      updateDraftInstrument({
        ...draftInstrument,
        repeatOffset: repeatStart,
        repeatLength: draftInstrument.repeatLength - delta,
      });
    }
  };

  const updateRepeatStart = () => {
    const repeatStart = Number(inputState.repeatStart);
    const maxValue =
      draftInstrument.repeatLength === 0
        ? draftInstrument.sample.sampleLength - 2
        : draftInstrument.repeatOffset + draftInstrument.repeatLength - 1;
    if (
      Number.isInteger(repeatStart) &&
      repeatStart >= 0 &&
      repeatStart <= maxValue
    ) {
      setRepeatOffsetAndLength(repeatStart);
    } else {
      syncInputStateWithDraft();
      if (draftInstrument.repeatLength === 0)
        void alerting.showInfo(message("invalidRepeatStartWithoutRepeatEnd"));
      else void alerting.showInfo(message("invalidRepeatStartWithRepeatEnd"));
    }
    loseFocus();
  };

  const updateRepeatEnd = () => {
    const repeatEnd = Number(inputState.repeatEnd);
    if (
      Number.isInteger(repeatEnd) &&
      repeatEnd > draftInstrument.repeatOffset &&
      repeatEnd < draftInstrument.sample.sampleLength
    ) {
      updateDraftInstrument({
        ...draftInstrument,
        repeatLength: repeatEnd - draftInstrument.repeatOffset,
      });
    } else {
      syncInputStateWithDraft();
      void alerting.showInfo(message("invalidRepeatEnd"));
    }
    loseFocus();
  };

  const IncrementButton = ({ onClick }: SpinnerButtonProps) => {
    return (
      <button
        type="button"
        className={"increment"}
        onClick={onClick}
      >
        <svg
          xmlns="http://www.w3.org/2000/svg"
          height="24px"
          viewBox="0 -960 960 960"
          width="24px"
          fill="currentColor"
        >
          <path d="m280-400 200-200 200 200H280Z" />
        </svg>
      </button>
    );
  };

  const DecrementButton = ({ onClick }: SpinnerButtonProps) => {
    return (
      <button
        type="button"
        className={"decrement"}
        onClick={onClick}
      >
        <svg
          xmlns="http://www.w3.org/2000/svg"
          height="24px"
          viewBox="0 -960 960 960"
          width="24px"
          fill="currentColor"
        >
          <path d="M480-360 280-560h400L480-360Z" />
        </svg>
      </button>
    );
  };

  return (
    <Modal ref={modalRef} className="sampleEdit">
      <h1 className="instrumentEditTitle padded">
        {messageFn("instrumentTitle")(
          hexadecimal.toHex(instrumentIndex + 1, 2),
        )}
      </h1>
      <div className="sampleNameLabel padded sampleEditLabel">
        <label htmlFor="sampleNameInput">
          {message("instrumentNameLabel")}
        </label>
      </div>
      <div className="sampleNameEdit uiArea padded rounded sampleEditField">
        <input
          type="text"
          id="sampleNameInput"
          maxLength={SampleNameMaxLength}
          value={draftInstrument.name}
          onFocus={editor.startTextInput}
          onBlur={editor.stopTextInput}
          onChange={(e) => {
            updateDraftInstrument({ ...draftInstrument, name: e.target.value });
          }}
        />
      </div>
      <div className="defaultVolumeLabel padded sampleEditLabel">
        <label htmlFor="defaultVolumeInput">
          {message("instrumentDefaultVolumeLabel")}
        </label>
      </div>
      <div className="defaultVolumeEdit uiArea padded rounded sampleEditField">
        <input
          type="range"
          id="defaultVolumeInput"
          min={0}
          max={255}
          value={draftInstrument.defaultVolume}
          onChange={(e) => {
            updateDraftInstrument({
              ...draftInstrument,
              defaultVolume: e.target.valueAsNumber,
            });
          }}
        />
        <span className="defaultVolumeValue">
          {Math.round((100 * draftInstrument.defaultVolume) / 255) + "%"}
        </span>
      </div>
      <div className="baseNoteLabel padded sampleEditLabel">
        <label htmlFor="baseNoteInput">
          {message("instrumentBaseNoteLabel")}
        </label>
      </div>
      <div className="baseNoteEdit uiArea padded rounded sampleEditField">
        <select
          id="baseNoteInput"
          value={draftInstrument.baseNote}
          onChange={(e) =>
            updateDraftInstrument({
              ...draftInstrument,
              baseNote: Number(e.target.value),
            })
          }
        >
          {notes.allNotes().map((note, index) => (
            <option key={index} value={index}>
              {note}
            </option>
          ))}
        </select>
      </div>
      <div className="transposeLabel padded sampleEditLabel">
        <label htmlFor="transposeInput">
          {message("instrumentTransposeLabel")}
        </label>
      </div>
      <div className="transposeEdit uiArea padded rounded sampleEditField">
        <input
          type="text"
          id="transposeInput"
          value={inputState.transpose}
          onFocus={editor.startTextInput}
          onChange={(e) =>
            setInputState({
              ...inputState,
              transpose: e.target.value,
            })
          }
          onBlur={(e) => {
            e.preventDefault();
            editor.stopTextInput();
            validateTranspose();
          }}
        />
        <div className="spinnerButtons">
          <IncrementButton
            onClick={() => {
              if (draftInstrument.transpose == 12) return;
              updateDraftInstrument({
                ...draftInstrument,
                transpose: draftInstrument.transpose + 1,
              });
            }}
          />
          <DecrementButton
            onClick={() => {
              if (draftInstrument.transpose == -12) return;
              updateDraftInstrument({
                ...draftInstrument,
                transpose: draftInstrument.transpose - 1,
              });
            }}
          />
        </div>
      </div>
      <div className="sampleLengthLabel padded sampleEditLabel">
        <label>{message("instrumentSampleLengthLabel")}</label>
      </div>
      <div className="sampleLengthEdit padded sampleEditField">
        <input
          type="text"
          readOnly
          value={draftInstrument.sample.sampleLength}
        />
      </div>
      <div className="sampleRepeatsLabel padded sampleEditLabel">
        <label>{message("instrumentSampleLoopsLabel")}</label>
      </div>
      <div className="sampleRepeatsEdit padded sampleEditField">
        <input
          type="radio"
          id="sampleRepeatsYes"
          name="sampleRepeats"
          checked={draftInstrument.repeats}
          onChange={setSampleRepeats}
        />
        <label htmlFor="sampleRepeatsYes">{message("yes")}</label>
        <input
          type="radio"
          id="sampleRepeatsNo"
          name="sampleRepeats"
          checked={!draftInstrument.repeats}
          onChange={setSampleNoRepeat}
        />
        <label htmlFor="sampleRepeatsNo">{message("no")}</label>
      </div>
      {draftInstrument.repeats && (
        <>
          <div className="repeatStartLabel padded sampleEditLabel">
            <label htmlFor="repeatStartInput">
              {message("instrumentLoopStartLabel")}
            </label>
          </div>
          <div className="repeatStartEdit uiArea padded rounded sampleEditField">
            <input
              type="text"
              id="repeatStartInput"
              value={inputState.repeatStart}
              onFocus={editor.startTextInput}
              onChange={(e) =>
                setInputState({
                  ...inputState,
                  repeatStart: e.target.value,
                })
              }
              onBlur={(e) => {
                e.preventDefault();
                editor.stopTextInput();
                updateRepeatStart();
              }}
            />
          </div>
          <div className="repeatEndLabel padded sampleEditLabel">
            <label htmlFor="repeatEndInput">
              {message("instrumentLoopEndLabel")}
            </label>
          </div>
          <div className="repeatEndEdit uiArea padded rounded sampleEditField">
            <input
              type="text"
              id="repeatEndInput"
              value={inputState.repeatEnd}
              onFocus={editor.startTextInput}
              onChange={(e) =>
                setInputState({
                  ...inputState,
                  repeatEnd: e.target.value,
                })
              }
              onBlur={(e) => {
                e.preventDefault();
                editor.stopTextInput();
                updateRepeatEnd();
              }}
            />
          </div>
        </>
      )}
      <div className="saveCloseButtons uiArea padded rounded">
        <button type="button" onClick={commands.loadSample}>
          {message("loadSampleButtonLabel")}
        </button>
        <button type="button" onClick={commands.deleteSample}>
          {message("deleteSampleButtonLabel")}
        </button>
        <button type="button" onClick={commands.exportSample}>
          {message("exportSampleButtonLabel")}
        </button>
        <button type="button" onClick={commands.saveAndCloseInstrumentEditor}>
          {message("saveButtonLabel")}
        </button>
        <button
          type="button"
          onClick={commands.restoreAndCloseInstrumentEditor}
        >
          {message("cancelButtonLabel")}
        </button>
      </div>
    </Modal>
  );
}
