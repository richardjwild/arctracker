import { invoke } from "@tauri-apps/api/core";
import { message } from "@tauri-apps/plugin-dialog";
import { PlayerEvent, PlayerSnapshot, Track } from "../player/player.ts";
import { Sample } from "../editing/editInstrument.ts";
import { Module } from "../module/module.ts";
import { Effect, PatternEvent, PatternLine } from "../editing/patternEvents.ts";
import { ExportState } from "../audioExport/audioExport.ts";

export type InstrumentUpdate = {
  assigned: boolean;
  name: string;
  defaultVolume: number;
  transpose: number;
  repeats: boolean;
  repeatOffset: number;
  repeatLength: number;
  sampleIndex: number;
};

export type PeakLevels = {
  left: number;
  right: number;
};

type PackedPatternLine = {
  row: number;
  events: number[][];
};

type PackedSnapshot = {
  playing: boolean;
  looping: boolean;
  currentBpm: number;
  sequencePos: number;
  patternIndex: number;
  patternNo: number;
  patternLength: number;
  tracks: Track[];
  newPattern: PackedPatternLine[] | null;
};

function toPatternLine(snapshotLine: PackedPatternLine): PatternLine {
  return {
    row: snapshotLine.row,
    events: snapshotLine.events.map((event) => unpackEvent(event)),
  };
}

function unpackEvent(packed: number[]): PatternEvent {
  const noteAndInstrument = packed[0];
  const effectCodes = packed[1];
  const effectData = packed[2];
  const effects: Effect[] = [];
  for (let effect = 0; effect < 4; effect++) {
    const shift = effect * 8;
    const effectCode = toEffectCode((effectCodes >>> shift) & 0xff);
    const packedData = (effectData >>> shift) & 0xff;
    effects.push({
      effectCode,
      effectData: [(packedData >>> 4) & 0xf, packedData & 0xf],
    });
  }
  return {
    note: (noteAndInstrument & 0xff0000) >>> 16,
    sampleNo: (noteAndInstrument & 0xff000000) >>> 24,
    effects,
  };
}

function toEffectCode(charCode: number): string {
  if (charCode === 0) return "";
  return String.fromCharCode(charCode);
}

function exitUnsuccessfully() {
  invoke("exit_unsuccessfully").then(() => {});
}

export const engine = {
  getCurrentModule: async () => {
    return await invoke<Module>("current_module");
  },

  loadModule: async (fileName: string) => {
    return await invoke<Module>("load_module", {
      path: fileName,
    });
  },

  saveModule: async (fileName: string, format: number) => {
    return await invoke("save_module", {
      path: fileName,
      format,
    });
  },

  createModule: async (numTracks: number) => {
    return await invoke<Module>("create_module", {
      numTracks,
    });
  },

  startPlayer: () => {
    invoke("restart_player").catch((error) => {
      message(
        `Could not restart audio subsystem. Arctracker must now shut down. Error details: ${error}`,
        {
          title: "Arctracker",
          kind: "error",
        },
      ).then(() => exitUnsuccessfully());
    });
  },

  setSelectedInstrument: (instrumentNo: number) => {
    void invoke("set_selected_instrument", {
      instrumentNo,
    });
  },

  setSelectedChannel: (channelNo: number) => {
    void invoke("set_selected_channel", {
      channelNo,
    });
  },

  togglePlay: () => {
    void invoke("toggle_play");
  },

  toggleLoop: () => {
    void invoke("toggle_loop");
  },

  seek: (newSequencePos: number, newPatternPos: number) => {
    void invoke("seek", {
      newSequencePos,
      newPatternPos,
    });
  },

  setMasterGain: (masterGain: number) => {
    void invoke("set_master_gain", {
      masterGain,
    });
  },

  toggleTrackMute: (track: number) => {
    void invoke("toggle_track_mute", {
      track,
    });
  },

  setEffectsDisplayed: (track: number, effectsDisplayed: number) => {
    void invoke("set_effects_displayed", {
      track,
      effectsDisplayed,
    });
  },

  getPlayerSnapshot: async (
    displayedPatternNo: number | null,
    numTracks: number,
  ): Promise<PlayerSnapshot> => {
    const packedSnapshot: PackedSnapshot = await invoke("get_player_snapshot", {
      displayedPatternNo,
      numTracks,
    });
    return {
      playing: packedSnapshot.playing,
      looping: packedSnapshot.looping,
      currentBpm: packedSnapshot.currentBpm,
      sequencePos: packedSnapshot.sequencePos,
      patternIndex: packedSnapshot.patternIndex,
      patternNo: packedSnapshot.patternNo,
      patternLength: packedSnapshot.patternLength,
      tracks: packedSnapshot.tracks,
      newPattern: packedSnapshot.newPattern ? packedSnapshot.newPattern.map(toPatternLine) : null,
    };
  },

  getAndResetPeakLevels: async (): Promise<PeakLevels> => {
    return await invoke("get_and_reset_peak_levels");
  },

  getPattern: async (
    patternNo: number,
    numLines: number,
    numTracks: number,
  ): Promise<PatternLine[]> => {
    const packedPattern: PackedPatternLine[] = await invoke("get_pattern", {
      patternNo,
      numLines,
      numTracks,
    });
    return packedPattern.map(toPatternLine);
  },

  pollPlaybackEvents: async (): Promise<PlayerEvent[]> => {
    return await invoke("poll_playback_events");
  },

  pollExportEvents: async (): Promise<PlayerEvent[]> => {
    return await invoke("poll_export_events");
  },

  defaultSavePath: async (modulePath: string): Promise<string | undefined> => {
    return await invoke("default_save_path", {
      modulePath,
    });
  },

  defaultExportPath: async (
    modulePath: string,
  ): Promise<string | undefined> => {
    return await invoke("default_export_path", {
      modulePath,
    });
  },

  exportAudio: async (exportPath: string) => {
    return await invoke("export_audio", {
      exportPath,
    });
  },

  getExportState: async (): Promise<ExportState> => {
    return await invoke("get_export_state");
  },

  exportCleanup: async () => {
    return await invoke("export_cleanup");
  },

  noteOn: (note: number) => {
    void invoke("keyboard_note_on", {
      note,
    });
  },

  getEvent: async (
    patternNo: number,
    patternIndex: number,
    track: number,
  ): Promise<PatternEvent> => {
    return await invoke("edit_get_event", {
      patternNo,
      patternIndex,
      track,
    });
  },

  setEvent: async (
    patternNo: number,
    patternIndex: number,
    track: number,
    newEvent: PatternEvent,
  ) => {
    return await invoke("edit_set_event", {
      patternNo,
      patternIndex,
      track,
      newEvent,
    });
  },

  getSequence: async (expectedSequenceLen: number): Promise<number[]> => {
    return await invoke("edit_get_sequence", {
      expectedSequenceLen,
    });
  },

  setSequence: async (newSequence: number[]) => {
    return await invoke("edit_set_sequence", {
      newSequence,
    });
  },

  createPattern: async (patternLength: number): Promise<number> => {
    return await invoke("edit_create_pattern", {
      patternLength,
    });
  },

  deletePattern: async (patternNo: number) => {
    await invoke("edit_delete_pattern", {
      patternNo,
    });
  },

  setPatternLength: async (patternNo: number, newLength: number) => {
    await invoke("edit_set_pattern_length", {
      patternNo,
      newLength,
    });
  },

  updateInstrument: async (
    instrumentIndex: number,
    instrumentUpdate: InstrumentUpdate,
  ) => {
    await invoke("edit_update_instrument", {
      instrumentIndex,
      instrumentUpdate,
    });
  },

  loadSample: async (path: String): Promise<Sample> => {
    return await invoke("edit_load_sample", {
      path,
    });
  },

  setModuleTitle: async (name: String, author: String) => {
    return await invoke("edit_set_module_title", {
      name,
      author,
    });
  },

  setNumTracks: async (numTracks: number) => {
    return await invoke("edit_set_num_tracks", {
      numTracks,
    });
  },

  setTempo: async (linesPerBeat: number, beatsPerMinute: number) => {
    await invoke("edit_set_tempo", {
      linesPerBeat,
      beatsPerMinute,
    })
  },

  exitSuccessfully: async () => {
    return await invoke("exit_successfully");
  },
};
