import { engine } from "../engine/engine.ts";
import { useStore } from "../store/useStore.ts";
import { AppPoller } from "../polling/poller.ts";
import { commands } from "../control/commands.ts";
import { PatternLine } from "../editing/patternEvents.ts";
import { userMessages } from "../messages/userMessages.ts";
import { messageFn } from "../language/messages.ts";

export type Track = {
  muted: boolean;
  panning: number;
  effectsDisplayed: number;
};

export type PlayerSnapshot = {
  playing: boolean;
  playbackAvailable: boolean;
  looping: boolean;
  currentBpm: number;
  sequencePos: number;
  patternIndex: number;
  patternNo: number;
  patternLength: number;
  tracks: Track[];
  newPattern: PatternLine[] | null;
};

export type PlayerEvent =
  | {
      eventType: "PlayerError";
      data: { error_message: string };
    }
  | { eventType: "UserMidiNoteOn"; data: { midi_note: number } };

function handlePlayerError(errorMessage: string) {
  userMessages.logMessage({
    type: "warning",
    message: messageFn("audioSystemError")(errorMessage),
  });
  void engine.startPlayer();
}

let snapshotPolling = false;

export const player = {
  snapshotPoller: (async () => {
    if (snapshotPolling) return;
    snapshotPolling = true;
    try {
      const numTracks = useStore.getState().module.numTracks;
      const displayedPatternNo = useStore.getState().currentPattern?.patternNo;
      const snapshot = await engine.getPlayerSnapshot(
        displayedPatternNo,
        numTracks,
      );
      useStore.getState().updatePlaybackState(snapshot);
    } finally {
      snapshotPolling = false;
    }
  }) as AppPoller,

  eventsPoller: (() => {
    engine.pollPlaybackEvents().then((events) => {
      for (const event of events) {
        console.log('event', event);
        switch (event.eventType) {
          case "PlayerError":
            handlePlayerError(event.data.error_message);
            return; // No point trying to handle any other events in this case.
          case "UserMidiNoteOn":
            commands.editNoteField(event.data.midi_note);
            break;
        }
      }
    });
  }) as AppPoller,
};
