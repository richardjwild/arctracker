import { listen } from "@tauri-apps/api/event";
import { useEffect } from "react";
import exitArctracker from "../exit/exit.ts";

export function useExitGuard() {
  useEffect(() => {
    const unlistenPromise = listen("exit-requested", exitArctracker);
    return () => {
      void unlistenPromise.then((unlisten) => unlisten());
    };
  }, []);
}
