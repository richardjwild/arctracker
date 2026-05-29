import { useEffect } from "react";
import { useStore } from "../store/useStore.ts";
import { midi } from "../midi/midi.ts";

export default function useMidiPlayback() {
  const selectedSample = useStore((state) => state.selectedSample);
  const track = useStore((state) => state.editorState.cursorPosition.track);
  useEffect(() => {
    if (selectedSample !== null)
      midi.useSample(selectedSample);
  }, [selectedSample])
  useEffect(() => {
    midi.useChannel(track);
  }, [track])
}
