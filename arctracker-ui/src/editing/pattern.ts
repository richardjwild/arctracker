import { engine } from "../engine/engine.ts";
import { useStore } from "../store/useStore.ts";
import { editor } from "./editor.ts";
import { sequence } from "./sequence.ts";
import { cursor } from "./cursor.ts";
import { messageFn2 } from "../language/messages.ts";

function ensurePatternIndex() {
  const patternNo = useStore.getState().sequence[sequence.currentPosition()];
  const module = useStore.getState().module;
  const currentLength = module.patternLengths[patternNo];
  if (cursor.currentPosition().patternIndex >= currentLength)
    cursor.updatePatternIndex(currentLength - 1);
}

export const pattern = {
  editCurrentPatternLength: () => {
    editor.setEditMode("patternLength");
  },

  createPattern: async (): Promise<number> => {
    const module = useStore.getState().module;
    const patternNo = await engine.createPattern(module.defaultPatternLength);
    const patternLengths = [...module.patternLengths];
    patternLengths.push(module.defaultPatternLength);
    useStore.getState().updatePatterns(module.numPatterns + 1, patternLengths);
    return patternNo;
  },

  deletePattern: async (patternNo: number) => {
    await engine.deletePattern(patternNo);
    const module = useStore.getState().module;
    const patternLengths = [...module.patternLengths];
    patternLengths.splice(patternNo, 1);
    useStore.getState().updatePatterns(module.numPatterns - 1, patternLengths);
  },

  setCurrentPatternLength: async (newLength: number) => {
    const sequenceIndex = sequence.currentPosition();
    const patternNo = useStore.getState().sequence[sequenceIndex];
    const module = useStore.getState().module;
    const currentLength = module.patternLengths[patternNo];
    if (currentLength !== newLength) {
      void editor.applyEdit({
        apply: async () => {
          await engine.setPatternLength(patternNo, newLength);
          const patternLengths = [...module.patternLengths];
          patternLengths[patternNo] = newLength;
          useStore.getState().updatePatterns(module.numPatterns, patternLengths);
          ensurePatternIndex();
          return true;
        },
        undo: async () => {
          await engine.setPatternLength(patternNo, currentLength);
          const patternLengths = [...module.patternLengths];
          patternLengths[patternNo] = currentLength;
          useStore.getState().updatePatterns(module.numPatterns, patternLengths);
          ensurePatternIndex();
        },
        undoMessage: messageFn2("setPatternLength")(patternNo.toString(), newLength.toString()),
      });
    }
    editor.restorePreviousEditMode();
  }
};
