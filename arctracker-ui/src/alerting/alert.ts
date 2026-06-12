import { message } from "@tauri-apps/plugin-dialog";

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

  showInfo: async (infoMessage: string) => {
    return await message(
      infoMessage,
      {
        title: "Arctracker",
        kind: "info",
      },
    )
  },
}
