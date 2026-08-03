import { engine } from "../engine/engine.ts";
import { useStore } from "../store/useStore.ts";
import { AppPoller } from "../polling/poller.ts";
import { commands } from "../control/commands.ts";
import { alerting } from "../alerting/alert.ts";
import { PatternLine } from "../editing/patternEvents.ts";

export type Track = {
  muted: boolean;
  panning: number;
  effectsDisplayed: number;
};

export type PlayerSnapshot = {
  playing: boolean;
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

let snapshotPolling = false;

export const player = {
  snapshotPoller: (async () => {
    if (snapshotPolling) return;
    snapshotPolling = true;
    try {
      const numTracks = useStore.getState().module.numTracks;
      const displayedPatternNo = useStore.getState().currentPattern?.patternNo;
      const snapshot = await engine.getPlayerSnapshot(displayedPatternNo, numTracks);
      useStore.getState().setTransportState({
        playing: snapshot.playing,
        looping: snapshot.looping,
        sequencePos: snapshot.sequencePos,
        patternIndex: snapshot.patternIndex,
      });
      useStore.getState().setTrackMuteState(snapshot.tracks.map((track) => track.muted));
      useStore.getState().setEffectsDisplayed(snapshot.tracks.map((track) => track.effectsDisplayed));
      useStore.getState().setTrackPanning(snapshot.tracks.map((track) => track.panning));
      if (snapshot.playing && snapshot.newPattern !== null) {
        useStore.getState().setCurrentPattern({
          patternNo: snapshot.patternNo,
          lines: snapshot.newPattern,
        });
      }
      const currentBpm = useStore.getState().module.beatsPerMinute;
      if (currentBpm !== snapshot.currentBpm) {
        console.log('current, new', currentBpm, snapshot.currentBpm);
        const linesPerBeat = useStore.getState().module.linesPerBeat;
        useStore.getState().updateTempo(linesPerBeat, snapshot.currentBpm);
      }
    } finally {
      snapshotPolling = false;
    }
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