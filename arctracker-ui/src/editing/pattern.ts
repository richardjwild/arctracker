import { engine } from "../engine/engine.ts";
import { useStore } from "../store/useStore.ts";

export const pattern = {
  createPattern: async (length: number): Promise<number> => {
    const patternNo = await engine.createPattern(length);
    const module = useStore.getState().module;
    const patternLengths = [...module.patternLengths];
    patternLengths.push(length);
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
};
