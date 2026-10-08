import { useEffect } from "react";
import { listen } from "@tauri-apps/api/event";
import { engine } from "../engine/engine.ts";
import { commands } from "../control/commands.ts";

export function useOpenFileListener() {
  useEffect(() => {
    let unlisten: (() => void) | undefined;
    let cancelled = false;

    const handleOpenRequest = async () => {
      const path = await engine.takePendingOpenRequest();
      if (path) {
        commands.loadSpecifiedFile(path);
      }
    };

    void listen("open-requested-files", () => {
      void handleOpenRequest();
    }).then((fn) => {
      if (cancelled) {
        fn();
      } else {
        unlisten = fn;
        // Listener is now active. Check for a request that arrived before it was registered.
        void handleOpenRequest();
      }
    });

    return () => {
      cancelled = true;
      unlisten?.();
    };
  }, []);
}