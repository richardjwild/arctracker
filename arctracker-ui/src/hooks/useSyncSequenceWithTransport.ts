import { useEffect } from "react";
import { useStore } from "../store/useStore.ts";
import { sequence } from "../editing/sequence.ts";

export default function useSyncSequenceWithTransport() {
  const sequencePos = useStore((state) => state.transportState.sequencePos);
  useEffect(() => {
    sequence.updatePosition(sequencePos);
  }, [sequencePos]);
}