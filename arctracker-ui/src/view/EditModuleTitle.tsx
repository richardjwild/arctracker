import { useStore } from "../store/useStore.ts";
import "./EditModuleTitle.css";
import Modal from "./Modal.tsx";
import { editor } from "../editing/editor.ts";
import { useEffect, useState } from "react";
import { ModuleTitle, moduleTitle } from "../editing/moduleTitle.ts";
import { commands } from "../control/commands.ts";
import { alerting } from "../alerting/alert.ts";
import { message } from "../language/messages.ts";

const ModuleNameMaxLength = 65;
const AuthorMaxLength = 65;
const DefaultPatternLengthMaxLength = 3;
const emptyDraft: ModuleTitle = {
  moduleName: "",
  author: "",
  defaultPatternLength: 64,
};

export default function EditModuleTitle() {
  const editing =
    useStore((state) => state.editorState.editMode) === "nameAndAuthor";
  const module = useStore((state) => state.module);
  const draftModuleTitle = useStore((state) => state.draftModuleTitle);
  const setDraftModuleTitle = useStore((state) => state.setDraftModuleTitle);
  const [defaultPatternLengthInput, setDefaultPatternLengthInput] =
    useState("");

  useEffect(() => {
    if (!editing) return;
    setDraftModuleTitle({
      ...draftModuleTitle,
      moduleName: module.name,
      author: module.author,
      defaultPatternLength: module.defaultPatternLength,
    });
    setDefaultPatternLengthInput(module.defaultPatternLength.toString());
  }, [module, editing]);

  const updateDraftModuleName = (moduleName: string) => {
    setDraftModuleTitle({
      ...(draftModuleTitle || emptyDraft),
      moduleName,
    });
  };

  const updateDraftAuthor = (author: string) => {
    setDraftModuleTitle({
      ...(draftModuleTitle || emptyDraft),
      author,
    });
  };

  const updateDraftDefaultPatternLength = (defaultPatternLength: number) => {
    setDraftModuleTitle({
      ...(draftModuleTitle || emptyDraft),
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
          value={draftModuleTitle?.moduleName}
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
          value={draftModuleTitle?.author}
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
      <div className="saveCloseButtons uiArea padded rounded">
        <button type="button" onClick={commands.setModuleTitle}>
          {message("saveButtonLabel")}
        </button>
        <button type="button" onClick={moduleTitle.hideDialog}>
          {message("cancelButtonLabel")}
        </button>
      </div>
    </Modal>
  );
}
