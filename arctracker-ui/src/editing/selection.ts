import { patternGrid, PatternGridPosition } from "./patternGrid.ts";
import { useStore } from "../store/useStore.ts";

export type PatternSelection = {
  anchor: PatternGridPosition,
  focus: PatternGridPosition,
}

export const selection = {
  navigateGrid: (move: () => PatternGridPosition, extendSelection: boolean) => {
    const before = patternGrid.currentPosition();
    const after = move();
    if (extendSelection) {
      const existing = useStore.getState().patternSelection;
      useStore.getState().setPatternSelection({
        anchor: existing?.anchor ?? before,
        focus: after,
      });
    } else {
      useStore.getState().setPatternSelection(null);
    }
  },
}
