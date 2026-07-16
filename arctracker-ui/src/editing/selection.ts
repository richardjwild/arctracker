import { patternGrid, PatternGridPosition } from "./patternGrid.ts";
import { useStore } from "../store/useStore.ts";
import { patternEvents } from "./patternEvents.ts";

export type PatternSelection = {
  anchor: PatternGridPosition,
  focus: PatternGridPosition,
}

export type PatternSelectionBounds = {
  top: number,
  bottom: number,
  left: number,
  right: number,
}

export const selection = {
  navigateGrid: (move: () => PatternGridPosition, extendSelection: boolean) => {
    const before = patternGrid.currentPosition();
    const after = move();
    if (extendSelection && patternEvents.editing()) {
      const existing = useStore.getState().patternSelection;
      useStore.getState().setPatternSelection({
        anchor: existing?.anchor ?? before,
        focus: after,
      });
    } else {
      selection.clearPatternSelection();
    }
  },

  patternSelectionBounds: (): PatternSelectionBounds | null => {
    const selection = useStore.getState().patternSelection;
    if (!selection) return null;
    return {
      top: Math.min(
        selection.anchor.patternIndex,
        selection.focus.patternIndex,
      ),
      bottom: Math.max(
        selection.anchor.patternIndex,
        selection.focus.patternIndex,
      ),
      left: Math.min(selection.anchor.track, selection.focus.track),
      right: Math.max(selection.anchor.track, selection.focus.track),
    };
  },

  clearPatternSelection: () => {
    useStore.getState().setPatternSelection(null);
  },
};
