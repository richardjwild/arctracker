import { editor } from "../editing/editor.ts";
import { alerting } from "../alerting/alert.ts";
import { message, messageFn } from "../language/messages.ts";
import { appConfig } from "../config/appConfig.ts";
import { engine } from "../engine/engine.ts";

let exiting = false;

async function saveConfigAndExit() {
  try {
    await appConfig.save();
  } catch (err) {
    await alerting.showError(messageFn("errorSavingConfig")(err as string));
  } finally {
    await engine.exitSuccessfully();
  }
}

export default async function exitArctracker() {
  if (exiting) {
    // Prevent duplicate dialogs if the user presses Cmd+Q repeatedly.
    return;
  }
  exiting = true;
  try {
    if (!editor.hasUnsavedChanges()) {
      await saveConfigAndExit();
      return;
    }
    const proceed = await alerting.askConfirmation(message("unsavedChanges"));
    if (proceed) {
      await saveConfigAndExit();
      return;
    }
  } finally {
    exiting = false;
  }
}
