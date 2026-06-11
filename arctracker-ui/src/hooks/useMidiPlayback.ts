import { useEffect } from "react";
import { useStore } from "../store/useStore.ts";
import { midi } from "../midi/midi.ts";

export default function useMidiPlayback() {
  const selectedInstrument = useStore((state) => state.selectedInstrument);
  const track = useStore((state) => state.editorState.cursorPosition.track);
  useEffect(() => {
    if (selectedInstrument !== null)
      midi.useInstrument(selectedInstrument);
  }, [selectedInstrument])
  useEffect(() => {
    midi.useChannel(track);
  }, [track])
}
