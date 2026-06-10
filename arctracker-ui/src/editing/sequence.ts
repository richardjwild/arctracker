import { useStore } from "../store/useStore.ts";
import { editor, EditCommand } from "./editor.ts";
import { transport } from "../transport/transport.ts";
import { cursor } from "./cursor.ts";
import { pattern } from "./pattern.ts";
import { engine } from "../engine/engine.ts";

function ensureSequencePosition() {
  const moduleSequence = useStore.getState().sequence;
  if (sequence.currentPosition() >= moduleSequence.length)
    sequence.updatePosition(moduleSequence.length - 1);
}

function basicEditCommand(before: number[], after: number[]): EditCommand {
  return {
    apply: async () => {
      await engine.setSequence(after);
      useStore.getState().setSequence(after);
      ensureSequencePosition();
      return true;
    },
    undo: async () => {
      await engine.setSequence(before);
      useStore.getState().setSequence(before);
      ensureSequencePosition();
    },
    postApply: ensureSequencePosition,
  };
}

async function setPattern(sequenceIndex: number, patternNo: number) {
  if (transport.playing()) return;
  const { sequence } = useStore.getState();
  if (sequenceIndex < 0 || sequenceIndex >= sequence.length) return;
  const { numPatterns } = useStore.getState().module;
  if (patternNo < 0 || patternNo >= numPatterns) return;
  const updatedSequence = [...sequence];
  updatedSequence[sequenceIndex] = patternNo;
  const command = basicEditCommand(sequence, updatedSequence);
  await editor.applyEdit(command);
}

export const sequence = {
  currentPosition: () => {
    return useStore.getState().editorState.sequencePosition;
  },

  advance: () => {
    sequence.updatePosition(sequence.currentPosition() + 1);
  },

  reverse: () => {
    sequence.updatePosition(sequence.currentPosition() - 1);
  },

  updatePosition: (sequencePosition: number) => {
    const { editorState, setEditorState } = useStore.getState();
    const { sequence } = useStore.getState();
    if (sequencePosition < 0 || sequencePosition >= sequence.length) return;
    const patternLengths = useStore.getState().module.patternLengths;
    const patternNo = sequence[sequencePosition];
    const patternLength = patternLengths[patternNo];
    let patternIndex = cursor.currentPosition().patternIndex;
    if (!patternLength) patternIndex = 0;
    else if (patternIndex >= patternLength) patternIndex = patternLength - 1;
    setEditorState({
      ...editorState,
      sequencePosition,
      cursorPosition: {
        ...editorState.cursorPosition,
        patternIndex,
      },
    });
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

  insertBefore: async (createNewPattern: boolean = false) => {
    if (transport.playing()) return;
    const sequencePosition = sequence.currentPosition();
    const command: EditCommand = {
      apply: async () => {
        const moduleSequence = useStore.getState().sequence;
        const patternNo = createNewPattern
          ? await pattern.createPattern(64) // TODO: Implement default length.
          : moduleSequence[sequencePosition];
        const updatedSequence = [...moduleSequence];
        updatedSequence.splice(sequencePosition, 0, patternNo);
        useStore.getState().setSequence(updatedSequence);
        await engine.setSequence(updatedSequence);
        return true;
      },
      undo: async () => {
        const moduleSequence = useStore.getState().sequence;
        const updatedSequence = [...moduleSequence];
        updatedSequence.splice(sequencePosition, 1);
        await engine.setSequence(updatedSequence);
        useStore.getState().setSequence(updatedSequence);
        if (createNewPattern) {
          const createdPattern = moduleSequence[sequencePosition];
          await pattern.deletePattern(createdPattern);
          ensureSequencePosition();
        }
      },
    }
    await editor.applyEdit(command);
  },

  insertAfter: async (createNewPattern: boolean = false) => {
    if (transport.playing()) return;
    const sequencePosition = sequence.currentPosition();
    const command: EditCommand = {
      apply: async () => {
        const moduleSequence = useStore.getState().sequence;
        const patternNo = createNewPattern
          ? await pattern.createPattern(64) // TODO: Implement default length.
          : moduleSequence[sequencePosition];
        const updatedSequence = [...moduleSequence];
        updatedSequence.splice(sequencePosition + 1, 0, patternNo);
        useStore.getState().setSequence(updatedSequence);
        await engine.setSequence(updatedSequence);
        sequence.advance();
        return true;
      },
      undo: async () => {
        const moduleSequence = useStore.getState().sequence;
        const updatedSequence = [...moduleSequence];
        updatedSequence.splice(sequencePosition + 1, 1);
        await engine.setSequence(updatedSequence);
        useStore.getState().setSequence(updatedSequence);
        if (createNewPattern) {
          const createdPattern = moduleSequence[sequencePosition + 1];
          await pattern.deletePattern(createdPattern);
          ensureSequencePosition();
        }
      },
    }
    await editor.applyEdit(command);
  },

  delete: async () => {
    if (transport.playing()) return;
    const moduleSequence = useStore.getState().sequence;
    if (moduleSequence.length === 1) return;
    const sequencePosition = sequence.currentPosition();
    const updatedSequence = [...moduleSequence];
    updatedSequence.splice(sequencePosition, 1);
    const command = basicEditCommand(moduleSequence, updatedSequence);
    await editor.applyEdit(command);
  },
};
