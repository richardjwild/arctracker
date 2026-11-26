import { useEffect } from "react";
import { getCurrentWindow } from "@tauri-apps/api/window";
import exitArctracker from "../exit/exit.ts";

export function useWindowCloseHandler() {
  useEffect(() => {
    const unlisten = getCurrentWindow().onCloseRequested((event) => {
      event.preventDefault();
      void exitArctracker();
    });

    return () => {
      void unlisten.then((fn) => fn());
    };
  }, []);
}
