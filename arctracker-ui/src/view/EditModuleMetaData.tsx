import { useStore } from "../store/useStore.ts";
import "./EditModuleMetaData.css";
import Modal from "./Modal.tsx";
import { useEffect, useRef, useState } from "react";
import {
  InterpolationType,
  ModuleMetaData,
  moduleMetaData,
  VolumeMappingType
} from "../editing/moduleMetaData.ts";
import { commands } from "../control/commands.ts";
import { alerting } from "../alerting/alert.ts";
import { message } from "../language/messages.ts";
import { primaryModifier } from "../keyboard/keyBinding.ts";
import { KeyHandler } from "../keyboard/keyHandler.ts";
import FieldLabel from "./components/FieldLabel.tsx";
import { useKeyboardHandler } from "../hooks/useKeyboardHandler.ts";

const ModuleNameMaxLength = 65;
const AuthorMaxLength = 65;
const DefaultPatternLengthMaxLength = 3;
const emptyDraft: ModuleMetaData = {
  moduleName: "",
  author: "",
  defaultPatternLength: 64,
  interpolationType: "ARCTRACKER",
  volumeMappingType: "ARCHIMEDES",
  relativePitchBend: true,
};

export default function EditModuleMetaData() {
  const editing =
    useStore((state) => state.editorState.editMode) === "moduleMetaData";
  const module = useStore((state) => state.module);
  const draftModuleMetaData = useStore((state) => state.draftModuleMetaData);
  const setDraftModuleMetaData = useStore((state) => state.setDraftModuleMetaData);
  const [defaultPatternLengthInput, setDefaultPatternLengthInput] =
    useState("");
  const moduleNameInputRef = useRef<HTMLInputElement>(null);
  const authorInputRef = useRef<HTMLInputElement>(null);
  const defaultPatternLengthInputRef = useRef<HTMLInputElement>(null);
  const interpolationTypeInputRef = useRef<HTMLInputElement>(null);
  const volumeMappingInputRef = useRef<HTMLInputElement>(null);
  const pitchBendInputRef = useRef<HTMLInputElement>(null);

  const handleKeys: KeyHandler = (e) => {
    if (e.code === 'Escape') {
      moduleMetaData.hideDialog();
      return true;
    } else if (e.code === 'KeyS' && primaryModifier(e)) {
      commands.setModuleMetaData();
      return true;
    } else if (e.code === 'Digit1' && primaryModifier(e)) {
      moduleNameInputRef.current?.focus();
      return true;
    } else if (e.code === 'Digit2' && primaryModifier(e)) {
      authorInputRef.current?.focus();
      return true;
    } else if (e.code === 'Digit3' && primaryModifier(e)) {
      defaultPatternLengthInputRef.current?.focus();
      return true;
    } else if (e.code === 'Digit4' && primaryModifier(e)) {
      interpolationTypeInputRef.current?.focus();
      return true;
    } else if (e.code === 'Digit5' && primaryModifier(e)) {
      volumeMappingInputRef.current?.focus();
      return true;
    } else if (e.code === 'Digit6' && primaryModifier(e)) {
      pitchBendInputRef.current?.focus();
      return true;
    }
    return false;
  };

  useKeyboardHandler(handleKeys, editing);

  useEffect(() => {
    if (!editing) return;
    setDraftModuleMetaData({
      ...draftModuleMetaData,
      moduleName: module.name,
      author: module.author,
      defaultPatternLength: module.defaultPatternLength,
      interpolationType: module.interpolationType,
      volumeMappingType: module.volumeMapping,
      relativePitchBend: module.relativePitchBend,
    });
    setDefaultPatternLengthInput(module.defaultPatternLength.toString());
  }, [module, editing]);

  const updateDraftModuleName = (moduleName: string) => {
    setDraftModuleMetaData({
      ...(draftModuleMetaData || emptyDraft),
      moduleName,
    });
  };

  const updateDraftAuthor = (author: string) => {
    setDraftModuleMetaData({
      ...(draftModuleMetaData || emptyDraft),
      author,
    });
  };

  const updateDraftDefaultPatternLength = (defaultPatternLength: number) => {
    setDraftModuleMetaData({
      ...(draftModuleMetaData || emptyDraft),
      defaultPatternLength,
    });
  };

  const validateDefaultPatternLength = (): boolean => {
    const defaultPatternLength = Number(defaultPatternLengthInput);
    if (
      Number.isInteger(defaultPatternLength) &&
      defaultPatternLength >= 1 &&
      defaultPatternLength <= 1000
    ) {
      updateDraftDefaultPatternLength(defaultPatternLength);
      return true;
    } else {
      void alerting.showInfo(message("invalidDefaultPatternLength"));
      setDefaultPatternLengthInput(module.defaultPatternLength.toString());
      return false;
    }
  };

  const setInterpolationType = (interpolationType: InterpolationType) => {
    setDraftModuleMetaData({
      ...(draftModuleMetaData || emptyDraft),
      interpolationType,
    });
  }

  const setVolumeMappingType = (volumeMappingType: VolumeMappingType) => {
    setDraftModuleMetaData({
      ...(draftModuleMetaData || emptyDraft),
      volumeMappingType,
    })
  }

  const setRelativePitchBend = (relativePitchBend: boolean) => {
    setDraftModuleMetaData({
      ...(draftModuleMetaData || emptyDraft),
      relativePitchBend,
    })
  }

  if (!editing) return null;

  return (
    <Modal className="editModuleTitle">
      <FieldLabel
        className="moduleNameLabel"
        text={message("moduleNameLabel")}
        associatedField="moduleNameInput"
        hotkey={1}
      />
      <div className="moduleNameEdit uiArea padded rounded">
        <input
          type="text"
          id="moduleNameInput"
          maxLength={ModuleNameMaxLength}
          value={draftModuleMetaData?.moduleName}
          onChange={(e) => updateDraftModuleName(e.target.value)}
          ref={moduleNameInputRef}
        />
      </div>
      <FieldLabel
        className="authorLabel"
        text={message("authorNameLabel")}
        associatedField="moduleNameInput"
        hotkey={2}
      />
      <div className="authorEdit uiArea padded rounded">
        <input
          type="text"
          id="authorInput"
          maxLength={AuthorMaxLength}
          value={draftModuleMetaData?.author}
          onChange={(e) => updateDraftAuthor(e.target.value)}
          ref={authorInputRef}
        />
      </div>
      <FieldLabel
        className="defaultPatternLengthLabel"
        text={message("defaultPatternLengthLabel")}
        associatedField="defaultPatternLengthInput"
        hotkey={3}
      />
      <div className="defaultPatternLengthEdit">
        <div className="uiArea padded rounded">
          <input
            type="text"
            id="defaultPatternLengthInput"
            maxLength={DefaultPatternLengthMaxLength}
            value={defaultPatternLengthInput}
            onBlur={validateDefaultPatternLength}
            onChange={(e) => setDefaultPatternLengthInput(e.target.value)}
            ref={defaultPatternLengthInputRef}
          />
        </div>
      </div>
      <FieldLabel
        className="interpolationTypeLabel"
        text={message("interpolationTypeLabel")}
        associatedField="interpolationTypeInput"
        hotkey={4}
      />
      <div className="interpolationTypeEdit radio">
        <input
          type="radio"
          name="interpolationTypeInput"
          id="interpolationTypeInputArctracker"
          value="arctracker"
          checked={draftModuleMetaData?.interpolationType === "ARCTRACKER"}
          onClick={() => setInterpolationType("ARCTRACKER")}
          ref={draftModuleMetaData?.interpolationType === "ARCTRACKER" ? interpolationTypeInputRef : null}
          />
        <label htmlFor="interpolationTypeInputArctracker">
          {message("arctrackerInterpolationType")}
        </label>
        <input
          type="radio"
          name="interpolationTypeInput"
          id="interpolationTypeInputArchimedes"
          value="archimedes"
          checked={draftModuleMetaData?.interpolationType === "ARCHIMEDES"}
          onClick={() => setInterpolationType("ARCHIMEDES")}
          ref={draftModuleMetaData?.interpolationType === "ARCHIMEDES" ? interpolationTypeInputRef : null}
          />
        <label htmlFor="interpolationTypeInputArchimedes">
          {message("archimedesInterpolationType")}
        </label>
      </div>
      <FieldLabel
        className="volumeMappingLabel"
        text={message("volumeMappingLabel")}
        associatedField="volumeMappingInputArctracker"
        hotkey={5}
      />
      <div className="volumeMappingEdit radio">
        <input
          type="radio"
          name="volumeMappingInput"
          id="volumeMappingInputArctracker"
          value="archimedes"
          checked={draftModuleMetaData?.volumeMappingType === "ARCHIMEDES"}
          onClick={() => setVolumeMappingType("ARCHIMEDES")}
          ref={draftModuleMetaData?.volumeMappingType === "ARCHIMEDES" ? volumeMappingInputRef : null}
          />
        <label htmlFor="volumeMappingInputArctracker">
          {message("arctrackerVolumeMapping")}
        </label>
        <input
          type="radio"
          name="volumeMappingInput"
          id="volumeMappingInputAmiga"
          value="amiga"
          checked={draftModuleMetaData?.volumeMappingType === "AMIGA"}
          onClick={() => setVolumeMappingType("AMIGA")}
          ref={draftModuleMetaData?.volumeMappingType === "AMIGA" ? volumeMappingInputRef : null}
          />
        <label htmlFor="volumeMappingInputAmiga">
          {message("amigaVolumeMapping")}
        </label>
      </div>
      <FieldLabel
        className="pitchBendTypeLabel"
        text={message("pitchBendTypeLabel")}
        associatedField="pitchBendInputRelative"
        hotkey={6}
      />
      <div className="pitchBendTypeEdit radio">
        <input
          type="radio"
          name="pitchBendInput"
          id="pitchBendInputRelative"
          value="relative"
          checked={draftModuleMetaData?.relativePitchBend}
          onClick={() => setRelativePitchBend(true)}
          ref={draftModuleMetaData?.relativePitchBend ? pitchBendInputRef : null}
          />
        <label htmlFor="pitchBendInputRelative">
          {message("relativePitchBend")}
        </label>
        <input
          type="radio"
          name="pitchBendInput"
          id="pitchBendInputFixed"
          value="fixed"
          checked={!draftModuleMetaData?.relativePitchBend}
          onClick={() => setRelativePitchBend(false)}
          ref={draftModuleMetaData?.relativePitchBend ? null : pitchBendInputRef}
          />
        <label htmlFor="pitchBendInputFixed">
          {message("fixedPitchBend")}
        </label>
      </div>
      <div className="saveCloseButtons">
        <button type="button" onClick={commands.setModuleMetaData}>
          {message("saveButtonLabel")}
        </button>
        <button type="button" onClick={moduleMetaData.hideDialog}>
          {message("cancelButtonLabel")}
        </button>
      </div>
    </Modal>
  );
}
