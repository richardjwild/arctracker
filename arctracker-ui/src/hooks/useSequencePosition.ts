import { useStore } from "../store/useStore.ts";

export default function useSequencePosition() {
  const sequencePos = useStore((state) => state.transportState.playing)
    ? useStore((state) => state.transportState.sequencePos)
    : useStore((state) => state.editorState.sequencePosition);
  const sequence = useStore((state) => state.sequence);
  const patternNo = sequence[sequencePos] ?? 0;
  const patternLengths = useStore((state) => state.module.patternLengths);
  const patternLength = patternLengths[patternNo] ?? 0;
  return { sequencePos, patternNo, patternLength };
}
