import { useEffect } from "react";
import { appConfig } from "../config/appConfig.ts";
import { messageFn } from "../language/messages.ts";
import { userMessages } from "../messages/userMessages.ts";

let readConfig = false;

export default function useAppConfig() {
  useEffect(() => {
    if (readConfig) return;
    appConfig
      .load()
      .then((config) => appConfig.apply(config))
      .catch((e) => {
        userMessages.logMessage({
          type: "warning",
          message: messageFn("failedToLoadConfig")(e as string),
        });
        appConfig.setToDefault();
      });
    readConfig = true;
  }, []);
}
