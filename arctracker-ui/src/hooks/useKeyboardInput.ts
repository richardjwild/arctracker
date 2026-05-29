import { useEffect } from "react";
import { keyboardEventListener } from "../keyboard/keyboardEventListener.ts";

export default function useKeyboardInput() {
  useEffect(() => {
    window.addEventListener("keydown", keyboardEventListener);
    return () => window.removeEventListener("keydown", keyboardEventListener);
  }, []);
}
