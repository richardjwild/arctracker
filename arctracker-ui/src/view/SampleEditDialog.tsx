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
import { primaryModifier } from "../keyboard/keyBinding.ts";
import { KeyHandler } from "../keyboard/keyHandler.ts";
import { registerKeyHandler } from "../keyboard/keyboardEventListener.ts";
import FieldLabel from "./components/FieldLabel.tsx";

type InputState = {
  transpose: string;
  fineTuning: string;
  repeatStart: string;
  repeatEnd: string;
  baseNote: string;
};

type SpinnerButtonProps = {
  onClick: React.MouseEventHandler<HTMLButtonElement>;
};

const emptyInputState: InputState = {
  transpose: "0",
  fineTuning: "0",
  repeatStart: "0",
  repeatEnd: "0",
  baseNote: "C0",
};

export default function SampleEditDialog() {
  const instrumentIndex = useStore((state) => state.selectedInstrument);
  const instruments = useStore((state) => state.module.instruments);
  const instrumentEditing =
    useStore((state) => state.editorState.editMode) === "instrument";
  const { draftInstrument, setDraftInstrument } = useStore((state) => state);
  const [inputState, setInputState] = useState(emptyInputState);
  const modalRef = useRef<HTMLDivElement>(null);
  const sampleNameRef = useRef<HTMLInputElement>(null);
  const defaultVolumeInputRef = useRef<HTMLInputElement>(null);
  const transposeInputRef = useRef<HTMLInputElement>(null);
  const baseNoteInputRef = useRef<HTMLInputElement>(null);
  const fineTuneInputRef = useRef<HTMLInputElement>(null);
  const sampleRepeatsInputRef = useRef<HTMLInputElement>(null);
  const repeatStartInputRef = useRef<HTMLInputElement>(null);
  const repeatEndInputRef = useRef<HTMLInputElement>(null);
  const loseFocus = () => modalRef.current?.focus();

  const syncInputStateWithDraft = () => {
    setInputState({
      transpose: (draftInstrument.transpose).toString(),
      fineTuning: (draftInstrument.sample.fineTuning).toString(),
      repeatStart: draftInstrument.repeatStart.toString(),
      repeatEnd: draftInstrument.repeatEnd.toString(),
      baseNote: notes.toString(draftInstrument.sample.baseNote + 1),
    });
  };

  const handleInstrumentEditorInput: KeyHandler = (e) => {
    if (e.code === 'Escape') {
      void commands.restoreAndCloseInstrumentEditor();
      return true;
    } else if (e.code === 'KeyL' && primaryModifier(e)) {
      void commands.loadSample();
      return true;
    } else if (e.code === 'KeyD' && primaryModifier(e)) {
      void commands.deleteSample();
      return true;
    } else if (e.code === 'KeyS' && primaryModifier(e)) {
      void commands.saveAndCloseInstrumentEditor();
      return true;
    } else if (e.code === 'Digit1' && primaryModifier(e)) {
      sampleNameRef.current?.focus();
      return true;
    } else if (e.code === 'Digit2' && primaryModifier(e)) {
      defaultVolumeInputRef.current?.focus();
      return true;
    } else if (e.code === 'Digit3' && primaryModifier(e)) {
      transposeInputRef.current?.focus();
      return true;
    } else if (e.code === 'Digit4' && primaryModifier(e)) {
      baseNoteInputRef.current?.focus();
      return true;
    } else if (e.code === 'Digit5' && primaryModifier(e)) {
      fineTuneInputRef.current?.focus();
      return true;
    } else if (e.code === 'Digit6' && primaryModifier(e)) {
      sampleRepeatsInputRef.current?.focus();
      return true;
    } else if (e.code === 'Digit7' && primaryModifier(e)) {
      repeatStartInputRef.current?.focus();
      return true;
    } else if (e.code === 'Digit8' && primaryModifier(e)) {
      repeatEndInputRef.current?.focus();
      return true;
    }
    return false;
  };

  useEffect(() => {
    if (!instrumentEditing) return;
    const unregisterKeyHandler = registerKeyHandler(handleInstrumentEditorInput);
    return () => {
      unregisterKeyHandler();
    }
  }, [instrumentEditing])

  useEffect(() => {
    if (!instrumentEditing) return;
    const instrument =
      instrumentIndex === null || instrumentIndex >= instruments.length
        ? emptyInstrument()
        : instruments[instrumentIndex];
    setDraftInstrument({ ...instrument, sample: { ...instrument.sample } });
  }, [instruments, instrumentIndex, instrumentEditing]);

  const updateDraftInstrument = (updatedDraftInstrument: Instrument) => {
    setDraftInstrument(updatedDraftInstrument);
  }

  useEffect(() => {
    syncInputStateWithDraft();
    if (instrumentEditing) {
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

  const getOctave = (baseNote: string): number | null => {
    const octave = Number(baseNote.length === 3 ? baseNote.substring(2) : baseNote.substring(1));
    return octave >= 0 && octave <= 4 ? octave : null;
  }

  const validateBaseNote = () => {
    const baseNote = inputState.baseNote;
    if (baseNote === "") {
      syncInputStateWithDraft();
      void alerting.showInfo(message("baseNoteNotSet"));
      return;
    }
    if (!baseNote.match(/[a-gA-G][#♯]?[0-9]/g)) {
      syncInputStateWithDraft();
      void alerting.showInfo(message("invalidBaseNote"));
      return;
    }
    let octave = getOctave(baseNote);
    if (octave === null) {
      syncInputStateWithDraft();
      void alerting.showInfo(message("invalidBaseNoteOctave"));
      return;
    }
    const note = (baseNote.length === 3 ? baseNote.substring(0, 2) : baseNote.substring(0, 1)).replace("♯", "#").toUpperCase();
    const recognisedNotes = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"];
    let relativeNoteIndex = recognisedNotes.indexOf(note);
    if (relativeNoteIndex === -1) {
      if (note === "E#") {
        // E# is enharmonic with F.
        relativeNoteIndex = recognisedNotes.indexOf("F");
      } else if (note === "B#") {
        if (octave === 4) {
          syncInputStateWithDraft();
          void alerting.showInfo(message("invalidBaseNoteOctave"));
          return;
        }
        // B# is enharmonic with C.
        relativeNoteIndex = recognisedNotes.indexOf("C");
        octave += 1;
      } else {
        // This should not happen, but if it does, I would very much like to see what it is.
        syncInputStateWithDraft();
        void alerting.showInfo(messageFn("unrecognisedNote")(note));
        return;
      }
    }
    setDraftInstrument({
      ...draftInstrument,
      sample: {
        ...draftInstrument.sample,
        baseNote: relativeNoteIndex + octave * 12,
      },
    });
  };

  const validateFineTune = () => {
    const fineTune = Number(inputState.fineTuning);
    if (Number.isInteger(fineTune) && fineTune >= -128 && fineTune <= 127) {
      updateDraftInstrument({
        ...draftInstrument,
        sample: {
          ...draftInstrument.sample,
          fineTuning: fineTune,
        }
      })
    } else {
      syncInputStateWithDraft();
      void alerting.showInfo(message("invalidFineTune"));
    }
  }

  const setSampleRepeats = () => {
    updateDraftInstrument({
      ...draftInstrument,
      repeats: true,
      repeatStart: 0,
      repeatEnd: Math.max(draftInstrument.sample.sampleLength - 1, 0),
    });
  };

  const setSampleNoRepeat = () => {
    updateDraftInstrument({
      ...draftInstrument,
      repeats: false,
      repeatStart: 0,
      repeatEnd: 0,
    });
  };

  const updateRepeatStart = () => {
    const repeatStart = Number(inputState.repeatStart);
    if (
      Number.isInteger(repeatStart) &&
      repeatStart >= 0 &&
      repeatStart <= draftInstrument.repeatEnd - 1
    ) {
      updateDraftInstrument({
        ...draftInstrument,
        repeatStart,
      });
    } else {
      syncInputStateWithDraft();
      void alerting.showInfo(message("invalidRepeatStart"));
    }
    loseFocus();
  };

  const updateRepeatEnd = () => {
    const repeatEnd = Number(inputState.repeatEnd);
    if (
      Number.isInteger(repeatEnd) &&
      repeatEnd > draftInstrument.repeatStart &&
      repeatEnd <= draftInstrument.sample.sampleLength - 1
    ) {
      updateDraftInstrument({
        ...draftInstrument,
        repeatEnd,
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
      <FieldLabel
        text={message("instrumentNameLabel")}
        className="sampleNameLabel"
        associatedField="sampleNameInput"
        hotkey={1}
      />
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
          ref={sampleNameRef}
        />
      </div>
      <FieldLabel
        text={message("instrumentDefaultVolumeLabel")}
        className="defaultVolumeLabel"
        associatedField="defaultVolumeInput"
        hotkey={2}
      />
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
          ref={defaultVolumeInputRef}
        />
        <span className="defaultVolumeValue">
          {Math.round((100 * draftInstrument.defaultVolume) / 255) + "%"}
        </span>
      </div>
      <FieldLabel
        text={message("instrumentTransposeLabel")}
        className="transposeLabel"
        associatedField="transposeInput"
        hotkey={3}
      />
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
          ref={transposeInputRef}
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
          className="readonly"
          readOnly
          value={draftInstrument.sample.sampleLength}
        />
      </div>
      <div className="sampleRateLabel padded sampleEditLabel">
        <label>{message("sampleRateLabel")}</label>
      </div>
      <div className="sampleRateEdit padded sampleEditField">
        <input
          type="text"
          className="readonly"
          readOnly
          value={
            draftInstrument.sample.sampleRate === 0
              ? ""
              : `${draftInstrument.sample.sampleRate}Hz`
          }
        />
      </div>
      {draftInstrument.sample.sampleLength > 0 && (
        <>
          <FieldLabel
            text={message("instrumentBaseNoteLabel")}
            className="baseNoteLabel"
            associatedField="baseNoteInput"
            hotkey={4}
          />
          <div className="baseNoteEdit uiArea padded rounded sampleEditField">
            <input
              type="text"
              id="baseNoteInput"
              value={inputState.baseNote}
              maxLength={3}
              onFocus={editor.startTextInput}
              onChange={(e) =>
                setInputState({
                  ...inputState,
                  baseNote: e.target.value,
                })
              }
              onBlur={(e) => {
                e.preventDefault();
                editor.stopTextInput();
                validateBaseNote();
                loseFocus();
              }}
              ref={baseNoteInputRef}
            />
            <div className="spinnerButtons">
              <IncrementButton
                onClick={() => {
                  if (
                    draftInstrument.sample.baseNote ==
                    notes.allNotes().length - 1
                  )
                    return;
                  updateDraftInstrument({
                    ...draftInstrument,
                    sample: {
                      ...draftInstrument.sample,
                      baseNote: draftInstrument.sample.baseNote + 1,
                    },
                  });
                }}
              />
              <DecrementButton
                onClick={() => {
                  if (draftInstrument.sample.baseNote == 0) return;
                  updateDraftInstrument({
                    ...draftInstrument,
                    sample: {
                      ...draftInstrument.sample,
                      baseNote: draftInstrument.sample.baseNote - 1,
                    },
                  });
                }}
              />
            </div>
          </div>
          <FieldLabel
            text={message("instrumentFineTuneLabel")}
            className="fineTuneLabel"
            associatedField="fineTuneInput"
            hotkey={5}
          />
          <div className="fineTuneEdit uiArea padded rounded sampleEditField">
            <input
              type="text"
              id="fineTuneInput"
              value={inputState.fineTuning}
              onFocus={editor.startTextInput}
              maxLength={3}
              onChange={(e) =>
                setInputState({
                  ...inputState,
                  fineTuning: e.target.value,
                })
              }
              onBlur={(e) => {
                e.preventDefault();
                editor.stopTextInput();
                validateFineTune();
              }}
              ref={fineTuneInputRef}
            />
            <div className="spinnerButtons">
              <IncrementButton
                onClick={() => {
                  if (draftInstrument.sample.fineTuning == 127) return;
                  updateDraftInstrument({
                    ...draftInstrument,
                    sample: {
                      ...draftInstrument.sample,
                      fineTuning: draftInstrument.sample.fineTuning + 1,
                    },
                  });
                }}
              />
              <DecrementButton
                onClick={() => {
                  if (draftInstrument.sample.fineTuning == -128) return;
                  updateDraftInstrument({
                    ...draftInstrument,
                    sample: {
                      ...draftInstrument.sample,
                      fineTuning: draftInstrument.sample.fineTuning - 1,
                    },
                  });
                }}
              />
            </div>
          </div>
          <FieldLabel
            text={message("instrumentSampleLoopsLabel")}
            className="sampleRepeatsLabel"
            associatedField={draftInstrument.repeats ? "sampleRepeatsYes" : "sampleRepeatsNo"}
            hotkey={6}
          />
          <div className="sampleRepeatsEdit padded sampleEditField">
            <input
              type="radio"
              id="sampleRepeatsYes"
              name="sampleRepeats"
              checked={draftInstrument.repeats}
              onChange={setSampleRepeats}
              ref={draftInstrument.repeats ? sampleRepeatsInputRef : null}
            />
            <label htmlFor="sampleRepeatsYes">{message("yes")}</label>
            <input
              type="radio"
              id="sampleRepeatsNo"
              name="sampleRepeats"
              checked={!draftInstrument.repeats}
              onChange={setSampleNoRepeat}
              ref={draftInstrument.repeats ? null : sampleRepeatsInputRef}
            />
            <label htmlFor="sampleRepeatsNo">{message("no")}</label>
          </div>
        </>
      )}
      {draftInstrument.repeats && (
        <>
          <FieldLabel
            text={message("instrumentLoopStartLabel")}
            className="repeatStartLabel"
            associatedField="repeatStartInput"
            hotkey={7}
          />
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
              ref={repeatStartInputRef}
            />
          </div>
          <FieldLabel
            text={message("instrumentLoopEndLabel")}
            className="repeatEndLabel"
            associatedField="repeatEndInput"
            hotkey={8}
          />
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
              ref={repeatEndInputRef}
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
