import Modal from "./Modal.tsx";
import "./SampleEditDialog.css";
import { useStore } from "../store/useStore.ts";
import { hexadecimal } from "../rendering/hexadecimal.ts";
import { editor } from "../editing/editor.ts";
import { commands } from "../control/commands.ts";
import { useEffect, useState } from "react";
import { Instrument } from "../engine/engine.ts";
import { alerting } from "../alerting/alert.ts";

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

export default function SampleEditDialog() {
  const instrumentIndex = useStore((state) => state.selectedInstrument);
  const instruments = useStore((state) => state.module.instruments);
  const instrumentEditing = useStore(
    (state) => state.editorState.instrumentEditing
  );
  const [draft, setDraft] = useState(emptyInstrument);

  useEffect(() => {
    const instrument =
      instrumentIndex === null || instrumentIndex >= instruments.length
        ? emptyInstrument
        : instruments[instrumentIndex];
    setDraft({ ...instrument, sample: { ...instrument.sample } });
  }, [instruments, instrumentIndex]);

  const amendTranspose = (newValue: number) => {
    if (newValue >= -12 && newValue <= 12)
      setDraft({
        ...draft,
        transpose: newValue + 13
      });
    else
      void alerting.showInfo("Transpose must be between -12 and 12.");
  };

  const saveAndClose = () => {
    // TODO: Construct edit command and issue it to editor.applyEdit().
    console.log("save instrument", draft);
    commands.saveAndCloseInstrumentEditor();
  };

  if (!instrumentEditing) return null;
  return (
    <Modal className="sampleEdit">
      <h1 className="instrumentEditTitle padded">
        Instrument {hexadecimal.toHex((instrumentIndex || 0) + 1, 2)}
      </h1>
      <div className="sampleNameLabel padded sampleEditLabel">
        <label>Name:</label>
      </div>
      <div className="sampleNameEdit uiArea padded sampleEditField">
        <input
          type="text"
          maxLength={36}
          defaultValue={draft.name}
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
          defaultValue={draft.defaultVolume}
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
          type="number"
          value={draft.transpose - 13}
          min={-12}
          max={12}
          onFocus={editor.startTextInput}
          onBlur={editor.stopTextInput}
          onChange={(e) =>
            amendTranspose(parseInt(e.target.value))
          }
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
          type="number"
          min={0}
          max={draft.sample.sampleLength}
          defaultValue={draft.repeatOffset}
          onFocus={editor.startTextInput}
          onBlur={editor.stopTextInput}
          onChange={(e) => {
            setDraft({
              ...draft,
              repeatOffset: parseInt(e.target.value)
            });
          }}
        />
      </div>
      <div className="repeatEndLabel padded sampleEditLabel">
        <label>Repeat End:</label>
      </div>
      <div className="repeatEndEdit uiArea padded sampleEditField">
        <input
          type="number"
          min={0}
          max={draft.sample.sampleLength}
          defaultValue={draft.repeatOffset + draft.repeatLength}
          onFocus={editor.startTextInput}
          onBlur={editor.stopTextInput}
          onChange={(e) => {
            setDraft({
              ...draft,
              repeatLength: parseInt(e.target.value)
            });
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
