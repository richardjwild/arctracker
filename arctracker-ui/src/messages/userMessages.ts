import { useStore } from "../store/useStore.ts";

export type UserMessage = {
  type: "welcome" | "info" | "warning";
  message: string;
};

export const userMessages = {
  logMessage: (message: UserMessage) => {
    useStore.getState().logMessage(message);
  }
};
