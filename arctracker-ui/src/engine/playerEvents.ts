import { engine } from "./engine.ts";
import { message } from "@tauri-apps/plugin-dialog";
import { AppPoller } from "../polling/poller.ts";
import { commands } from "../control/commands.ts";

export type PlayerEvent =
  { eventType: "playerError"; errorMessage: string } |
  { eventType: "userMidiNoteOn"; midiNote: number } |
  { eventType: "audioOverflowed" };

function handlePlayerError(errorMessage: string) {
  message(
    `Audio subsystem encountered error and will attempt restart. Error details: ${errorMessage}`,
    {
      title: "Arctracker",
      kind: "error",
    },
  ).then(() => {
    engine.startPlayer();
  });
}

function processPlayerEvents() {
  engine.pollPlaybackEvents().then((events) => {
    for (const event of events) {
      switch (event.eventType) {
        case "playerError":
          handlePlayerError(event.errorMessage);
          return; // No point trying to handle any other events in this case.
        case "userMidiNoteOn":
          commands.editNoteField(event.midiNote);
          break;
        case "audioOverflowed":
          console.log("Audio overflowed"); // TODO: Light an indicator or something.
          break;
      }
    }
  });
}

export const playerEvents = {
  poller: (() => processPlayerEvents()) as AppPoller,
}
