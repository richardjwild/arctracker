import { useStore } from "../store/useStore.ts";
import "./EditNameAndAuthor.css";
import Modal from "./Modal.tsx";
import { editor } from "../editing/editor.ts";
import { useEffect, useState } from "react";
import { nameAndAuthor } from "../editing/nameAndAuthor.ts";
import { commands } from "../control/commands.ts";

type InputState = {
  moduleName: string;
  author: string;
};

const emptyInputState: InputState = {
  moduleName: "",
  author: "",
};

const ModuleNameMaxLength = 65;
const AuthorMaxLength = 65;

export default function EditNameAndAuthor() {
  const editing =
    useStore((state) => state.editorState.editMode) === "nameAndAuthor";
  const module = useStore((state) => state.module);
  const [inputState, setInputState] = useState(emptyInputState);

  useEffect(() => {
    if (!editing) return;
    setInputState({
      ...inputState,
      moduleName: module.name,
      author: module.author,
    });
  }, [module, editing]);

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
          value={inputState.moduleName}
          onFocus={editor.startTextInput}
          onBlur={editor.stopTextInput}
          onChange={(e) => {
            setInputState({ ...inputState, moduleName: e.target.value });
          }}
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
          value={inputState.author}
          onFocus={editor.startTextInput}
          onBlur={editor.stopTextInput}
          onChange={(e) => {
            setInputState({ ...inputState, author: e.target.value });
          }}
        />
      </div>
      <div className="saveCloseButtons uiArea padded">
        <button
          type="button"
          onClick={() => {
            commands.setNameAndAuthor(inputState.moduleName, inputState.author);
          }}
        >
          Save
        </button>
        <button type="button" onClick={() => nameAndAuthor.hideDialog()}>
          Cancel
        </button>
      </div>
    </Modal>
  );
}
