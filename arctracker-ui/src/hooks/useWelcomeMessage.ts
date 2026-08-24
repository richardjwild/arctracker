import { useEffect } from "react";
import { userMessages } from "../messages/userMessages.ts";
import { message } from "../language/messages.ts";

let welcomeSent = false;

export default function useWelcomeMessage() {
  useEffect(() => {
    if (welcomeSent) return;
    userMessages.logMessage({
      type: "welcome",
      message: message("welcomeMessage"),
    });
    welcomeSent = true;
  }, [welcomeSent]);
}
