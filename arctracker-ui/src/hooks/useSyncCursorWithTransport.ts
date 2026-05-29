import { useEffect } from "react";
import { cursor } from "../editing/cursor.ts";
import { useStore } from "../store/useStore.ts";

export default function useSyncCursorWithTransport() {
  const transportPatternIndex = useStore((state) => state.transportState.patternIndex);
  const playing = useStore((state) => state.transportState.playing);
  useEffect(() => {
    if (playing) cursor.updatePatternIndex(transportPatternIndex);
  }, [playing, transportPatternIndex]);
}