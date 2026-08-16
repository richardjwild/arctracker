import { useEffect } from "react";
import { appConfig } from "../config/appConfig.ts";
import { message } from "../language/messages.ts";

let readConfig = false;

export default function useAppConfig() {
  useEffect(() => {
    if (readConfig) return;
    appConfig
      .load()
      .then((config) => appConfig.apply(config))
      .catch((e) => {
        console.error(message("failedToLoadConfig"), e as string);
        appConfig.setToDefault();
      });
    readConfig = true;
  }, []);
}
