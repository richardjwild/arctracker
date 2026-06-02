import { useStore } from "../store/useStore.ts";
import { editor, EditType, SequenceEditCommand } from "./editor.ts";
import { transport } from "../transport/transport.ts";

async function setPattern(sequenceIndex: number, patternNo: number) {
  if (transport.playing()) return;
  const { sequence } = useStore.getState();
  if (sequenceIndex < 0 || sequenceIndex >= sequence.length) return;
  const { numPatterns } = useStore.getState().module;
  if (patternNo < 0 || patternNo >= numPatterns) return;
  const updatedSequence = [ ...sequence ];
  updatedSequence[sequenceIndex] = patternNo;
  const command: SequenceEditCommand = {
    type: EditType.SequenceEdit,
    before: sequence,
    after: updatedSequence,
  };
  await editor.applyEdit(command);
}

export const sequence = {
  updatePosition: (sequencePosition: number) => {
    const { editorState, setEditorState } = useStore.getState();
    const { sequence } = useStore.getState();
    if (sequencePosition < 0 || sequencePosition >= sequence.length) return;
    setEditorState({
      ...editorState,
      sequencePosition,
    })
  },

  incrementPatternAtCurrentPosition: () => {
    const { sequencePosition } = useStore.getState().editorState;
    const moduleSequence = useStore.getState().sequence;
    const pattern = moduleSequence[sequencePosition];
    void setPattern(sequencePosition, pattern + 1);
  },

  decrementPatternAtCurrentPosition: () => {
    const { sequencePosition } = useStore.getState().editorState;
    const moduleSequence = useStore.getState().sequence;
    const pattern = moduleSequence[sequencePosition];
    void setPattern(sequencePosition, pattern - 1);
  },
};
