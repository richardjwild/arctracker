import { listen } from "@tauri-apps/api/event";
import { useEffect } from "react";
import { editor } from "../editing/editor.ts";
import { engine } from "../engine/engine.ts";
import { alerting } from "../alerting/alert.ts";
import { message } from "../language/messages.ts";

export function useExitGuard() {
  useEffect(() => {
    let exiting = false;
    const unlistenPromise = listen("exit-requested", async () => {
      if (exiting) {
        // Prevent duplicate dialogs if the user presses Cmd+Q repeatedly.
        return;
      }
      exiting = true;
      try {
        if (!editor.hasUnsavedChanges()) {
          await engine.exitSuccessfully();
          return;
        }
        const proceed = await alerting.askConfirmation(message("unsavedChanges"));
        if (proceed) {
          await engine.exitSuccessfully();
          return;
        }
      } finally {
        exiting = false;
      }
    });
    return () => {
      void unlistenPromise.then((unlisten) => unlisten());
    };
  }, []);
}