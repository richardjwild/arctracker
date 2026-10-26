import { useEffect, useRef } from "react";
import { KeyHandler } from "../keyboard/keyHandler.ts";
import { registerKeyHandler } from "../keyboard/keyboardEventListener.ts";

export function useKeyboardHandler(handler: KeyHandler, enabled = true) {
  const handlerRef = useRef(handler);

  useEffect(() => {
    handlerRef.current = handler;
  });

  useEffect(() => {
    if (!enabled) return;
    return registerKeyHandler((e) => handlerRef.current(e));
  }, [enabled]);
}