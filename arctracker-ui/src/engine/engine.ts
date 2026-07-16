import { invoke } from "@tauri-apps/api/core";
import { PlayerEvent } from "./playerEvents.ts";
import { message } from "@tauri-apps/plugin-dialog";

export type Module = {
  fileName: string | null;
  name: string;
  author: string;
  numTracks: number;
  numPatterns: number;
  patternLengths: number[];
  tuneLength: number;
  instruments: Instrument[];
  masterGain: number;
}

export type Instrument = {
  assigned: boolean;
  name: string;
  defaultVolume: number;
  transpose: number;
  repeats: boolean;
  repeatOffset: number;
  repeatLength: number;
  sample: Sample;
}

export type Sample = {
  sampleIndex: number;
  sampleLength: number;
}

export type InstrumentUpdate = {
  assigned: boolean;
  name: string;
  defaultVolume: number;
  transpose: number;
  repeats: boolean;
  repeatOffset: number;
  repeatLength: number;
  sampleIndex: number;
}

export type TransportState = {
  playing: boolean;
  looping: boolean;
  sequencePos: number;
  patternIndex: number;
  patternNo: number;
  patternLength: number;
}

export type PeakLevels = {
  left: number;
  right: number;
}

export type ExportState = {
  completed: boolean;
  percentComplete: number;
}

export type Effect = {
  effectCode: string;
  effectData: number[];
}

export type PatternEvent = {
  note: number;
  sampleNo: number;
  effects: Effect[];
}

export function effectsEqual(a: Effect, b: Effect): boolean {
  return (
    a.effectCode === b.effectCode &&
    a.effectData[0] === b.effectData[0] &&
    a.effectData[1] === b.effectData[1]
  );
}

export function eventsEqual(a: PatternEvent, b: PatternEvent): boolean {
  return (
    a.note === b.note &&
    a.sampleNo === b.sampleNo &&
    a.effects.length === b.effects.length &&
    a.effects.every((effect, i) => effectsEqual(effect, b.effects[i]))
  );
}

export function instrumentsEqual(a: Instrument, b: Instrument): boolean {
  return (
    a.assigned === b.assigned &&
    a.name === b.name &&
    a.defaultVolume === b.defaultVolume &&
    a.transpose === b.transpose &&
    a.repeats === b.repeats &&
    a.repeatOffset === b.repeatOffset &&
    a.repeatLength === b.repeatLength &&
    a.sample.sampleIndex === b.sample.sampleIndex
  );
}

export type PatternLine = {
  row: number;
  events: PatternEvent[];
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
    })
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
    })
  },

  getTransportState: async (): Promise<TransportState> => {
    return await invoke("get_transport_state");
  },

  getAndResetPeakLevels: async (): Promise<PeakLevels> => {
    return await invoke("get_and_reset_peak_levels");
  },

  getPattern: async (
    patternNo: number,
    numLines: number,
    numTracks: number,
  ): Promise<PatternLine[]> => {
    return await invoke("get_pattern", {
      patternNo,
      numLines,
      numTracks,
    });
  },

  pollPlaybackEvents: async (): Promise<PlayerEvent[]> => {
    return await invoke("poll_playback_events");
  },

  pollExportEvents: async (): Promise<PlayerEvent[]> => {
    return await invoke("poll_export_events");
  },

  defaultSavePath: async (
    modulePath: string,
  ): Promise<string | undefined> => {
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
      patternNo
    });
  },

  setPatternLength: async (patternNo: number, newLength: number) => {
    console.log('set_pattern_length', patternNo, newLength);
    await invoke("edit_set_pattern_length", {
      patternNo,
      newLength,
    });
  },

  updateInstrument: async (instrumentIndex: number, instrumentUpdate: InstrumentUpdate) => {
    await invoke("edit_update_instrument", {
      instrumentIndex,
      instrumentUpdate
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
    })
  },

  exitSuccessfully: async () => {
    return await invoke("exit_successfully");
  },
};
