import { useStore } from "../store/useStore.ts";
import "./EditModuleMetaData.css";
import Modal from "./Modal.tsx";
import { editor } from "../editing/editor.ts";
import { useEffect, useState } from "react";
import { InterpolationType, ModuleMetaData, moduleMetaData, VolumeMappingType } from "../editing/moduleMetaData.ts";
import { commands } from "../control/commands.ts";
import { alerting } from "../alerting/alert.ts";
import { message } from "../language/messages.ts";

const ModuleNameMaxLength = 65;
const AuthorMaxLength = 65;
const DefaultPatternLengthMaxLength = 3;
const emptyDraft: ModuleMetaData = {
  moduleName: "",
  author: "",
  defaultPatternLength: 64,
  interpolationType: "ARCTRACKER",
  volumeMappingType: "ARCHIMEDES",
};

export default function EditModuleMetaData() {
  const editing =
    useStore((state) => state.editorState.editMode) === "moduleMetaData";
  const module = useStore((state) => state.module);
  const draftModuleMetaData = useStore((state) => state.draftModuleMetaData);
  const setDraftModuleMetaData = useStore((state) => state.setDraftModuleMetaData);
  const [defaultPatternLengthInput, setDefaultPatternLengthInput] =
    useState("");

  useEffect(() => {
    if (!editing) return;
    setDraftModuleMetaData({
      ...draftModuleMetaData,
      moduleName: module.name,
      author: module.author,
      defaultPatternLength: module.defaultPatternLength,
      interpolationType: module.interpolationType,
      volumeMappingType: module.volumeMapping,
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

  if (!editing) return null;

  return (
    <Modal className="editModuleTitle">
      <div className="moduleNameLabel">
        <label htmlFor="moduleNameInput">{message("moduleNameLabel")}</label>
      </div>
      <div className="moduleNameEdit uiArea padded rounded">
        <input
          type="text"
          id="moduleNameInput"
          maxLength={ModuleNameMaxLength}
          value={draftModuleMetaData?.moduleName}
          onFocus={editor.startTextInput}
          onBlur={editor.stopTextInput}
          onChange={(e) => updateDraftModuleName(e.target.value)}
        />
      </div>
      <div className="authorLabel">
        <label htmlFor="authorInput">{message("authorNameLabel")}</label>
      </div>
      <div className="authorEdit uiArea padded rounded">
        <input
          type="text"
          id="authorInput"
          maxLength={AuthorMaxLength}
          value={draftModuleMetaData?.author}
          onFocus={editor.startTextInput}
          onBlur={editor.stopTextInput}
          onChange={(e) => updateDraftAuthor(e.target.value)}
        />
      </div>
      <div className="defaultPatternLengthLabel">
        <label htmlFor="defaultPatternLengthInput">
          {message("defaultPatternLengthLabel")}
        </label>
      </div>
      <div className="defaultPatternLengthEdit">
        <div className="uiArea padded rounded">
          <input
            type="text"
            id="defaultPatternLengthInput"
            maxLength={DefaultPatternLengthMaxLength}
            value={defaultPatternLengthInput}
            onFocus={editor.startTextInput}
            onBlur={() => {
              validateDefaultPatternLength();
              editor.stopTextInput();
            }}
            onChange={(e) => setDefaultPatternLengthInput(e.target.value)}
          />
        </div>
      </div>
      <div className="interpolationTypeLabel">
        <label htmlFor="interpolationTypeInput">
          {message("interpolationTypeLabel")}
        </label>
      </div>
      <div className="interpolationTypeEdit">
        <input
          type="radio"
          name="interpolationTypeInput"
          id="interpolationTypeInputArctracker"
          value="arctracker"
          checked={draftModuleMetaData?.interpolationType === "ARCTRACKER"}
          onClick={() => setInterpolationType("ARCTRACKER")}
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
          />
        <label htmlFor="interpolationTypeInputArchimedes">
          {message("archimedesInterpolationType")}
        </label>
      </div>
      <div className="volumeMappingLabel">
        <label htmlFor="volumeMappingInput">
          {message("volumeMappingLabel")}
        </label>
      </div>
      <div className="volumeMappingEdit">
        <input
          type="radio"
          name="volumeMappingInput"
          id="volumeMappingInputArctracker"
          value="archimedes"
          checked={draftModuleMetaData?.volumeMappingType === "ARCHIMEDES"}
          onClick={() => setVolumeMappingType("ARCHIMEDES")}
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
          />
        <label htmlFor="volumeMappingInputAmiga">
          {message("amigaVolumeMapping")}
        </label>
      </div>
      <div className="saveCloseButtons uiArea padded rounded">
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
