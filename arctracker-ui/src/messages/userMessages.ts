import { info, warn } from '@tauri-apps/plugin-log';
import { useStore } from "../store/useStore.ts";

export type UserMessage = {
  type: "welcome" | "info" | "warning";
  message: string;
};

export const userMessages = {
  logMessage: (message: UserMessage) => {
    if ("info" === message.type) {
      void info(message.message);
    }
    if ("warning" === message.type) {
      void warn(message.message);
    }
    useStore.getState().logMessage(message);
  }
};
