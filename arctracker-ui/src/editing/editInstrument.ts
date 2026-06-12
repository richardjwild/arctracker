import { useStore } from "../store/useStore";

export const editInstrument = {
  showDialog: () => {
    const editorState = useStore.getState().editorState;
    useStore.getState().setEditorState({
      ...editorState,
      instrumentEditing: true,
    });
  },

  closeDialog: () => {
    const editorState = useStore.getState().editorState;
    useStore.getState().setEditorState({
      ...editorState,
      instrumentEditing: false,
    });
  }
};
