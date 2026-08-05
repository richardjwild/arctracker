import { ask, message } from "@tauri-apps/plugin-dialog";

export const alerting = {
  showError: async (errorMessage: string) => {
    return await message(
      errorMessage,
      {
        title: "Arctracker",
        kind: "error",
      },
    )
  },

  showErrorWithContext: async (errorMessage: string, errorContext: string) => {
    return await message(
      `${errorMessage} (${errorContext})`,
      {
        title: "Arctracker",
        kind: "error",
      },
    )
  },

  showInfo: async (infoMessage: string) => {
    return await message(
      infoMessage,
      {
        title: "Arctracker",
        kind: "info",
      },
    )
  },

  askConfirmation: async (question: string) => {
    return await ask(
      question,
      {
        title: "Arctracker",
        kind: "warning"
      }
    );
  },
};
