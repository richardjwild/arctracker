import { useEffect, useState } from "react";
import { appConfig } from "../config/appConfig.ts";
import { messageFn } from "../language/messages.ts";
import { userMessages } from "../messages/userMessages.ts";

let configInitialisation: Promise<void> | undefined;

function initialiseAppConfig(): Promise<void> {
  if (!configInitialisation) {
    configInitialisation = appConfig
      .load()
      .then((config) => appConfig.apply(config))
      .catch((e) => {
        userMessages.logMessage({
          type: "warning",
          message: messageFn("failedToLoadConfig")(e as string),
        });
        return appConfig.setToDefault();
      });
  }
  return configInitialisation;
}

export default function useAppConfig() {
  const [ready, setReady] = useState(false);
  useEffect(() => {
    let cancelled = false;
    void initialiseAppConfig().then(() => {
      if (!cancelled) setReady(true);
    });
    return () => {
      cancelled = true;
    };
  }, []);
  return ready;
}
