import { useStore } from "../store/useStore.ts";
import "./EditNameAndAuthor.css";
import Modal from "./Modal.tsx";
import { editor } from "../editing/editor.ts";
import { useEffect } from "react";
import { ModuleTitle, nameAndAuthor } from "../editing/nameAndAuthor.ts";
import { commands } from "../control/commands.ts";

const ModuleNameMaxLength = 65;
const AuthorMaxLength = 65;
const emptyDraft: ModuleTitle = {
  moduleName: "",
  author: "",
};

export default function EditNameAndAuthor() {
  const editing =
    useStore((state) => state.editorState.editMode) === "nameAndAuthor";
  const module = useStore((state) => state.module);
  const draftModuleTitle = useStore((state) => state.draftModuleTitle);
  const setDraftModuleTitle = useStore((state) => state.setDraftModuleTitle);

  useEffect(() => {
    if (!editing) return;
    setDraftModuleTitle({
      ...draftModuleTitle,
      moduleName: module.name,
      author: module.author,
    });
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

  if (!editing) return null;

  return (
    <Modal className="editNameAndAuthor">
      <div className="moduleNameLabel">
        <label htmlFor="moduleNameInput">Module Name:</label>
      </div>
      <div className="moduleNameEdit uiArea padded">
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
        <label htmlFor="authorInput">Author:</label>
      </div>
      <div className="authorEdit uiArea padded">
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
      <div className="saveCloseButtons uiArea padded">
        <button type="button" onClick={commands.setNameAndAuthor}>
          Save
        </button>
        <button type="button" onClick={nameAndAuthor.hideDialog}>
          Cancel
        </button>
      </div>
    </Modal>
  );
}
