import { engine } from "./engine.ts";
import { AppPoller } from "../polling/poller.ts";
import { commands } from "../control/commands.ts";
import { alerting } from "../alerting/alert.ts";

export type PlayerEvent =
  | { eventType: "playerError"; errorMessage: string }
  | { eventType: "userMidiNoteOn"; midiNote: number }
  | { eventType: "audioOverflowed" };

function handlePlayerError(errorMessage: string) {
  alerting
    .showError(
      `Audio subsystem encountered error and will attempt restart. Error details: ${errorMessage}`,
    )
    .then(() => {
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
};
