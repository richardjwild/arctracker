import { engine, PatternLine } from "../engine/engine.ts";
import { useStore } from "../store/useStore.ts";
import { AppPoller } from "../polling/poller.ts";
import { commands } from "../control/commands.ts";
import { alerting } from "../alerting/alert.ts";

export type PlayerSnapshot = {
  playing: boolean;
  looping: boolean;
  sequencePos: number;
  patternIndex: number;
  patternNo: number;
  patternLength: number;
  trackMuted: boolean[];
  newPattern: PatternLine[] | null;
}

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

export const player = {
  snapshotPoller: (() => {
    const numTracks = useStore.getState().module.numTracks;
    const displayedPatternNo = useStore.getState().currentPattern?.patternNo;
    engine.getPlayerSnapshot(displayedPatternNo, numTracks).then((snapshot) => {
      useStore.getState().setTransportState({
        playing: snapshot.playing,
        looping: snapshot.looping,
        sequencePos: snapshot.sequencePos,
        patternIndex: snapshot.patternIndex,
      });
      useStore.getState().setTrackMuteState(snapshot.trackMuted);
      if (snapshot.newPattern !== null)
        useStore.getState().setCurrentPattern({
          patternNo: snapshot.patternNo,
          lines: snapshot.newPattern,
        });
    });
  }) as AppPoller,

  eventsPoller: (() => {
    engine.pollPlaybackEvents().then((events) => {
      for (const event of events) {
        switch (event.eventType) {
          case "playerError":
            handlePlayerError(event.errorMessage);
            return; // No point trying to handle any other events in this case.
          case "userMidiNoteOn":
            commands.editNoteField(event.midiNote);
            break;
        }
      }
    });
  }) as AppPoller,
}