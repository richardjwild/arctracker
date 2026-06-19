import { invoke } from "@tauri-apps/api/core";
import { PlayerEvent } from "./playerEvents.ts";
import { message } from "@tauri-apps/plugin-dialog";

export type Module = {
  fileName: string | null;
  name: string;
  author: string;
  numChannels: number;
  numPatterns: number;
  patternLengths: number[];
  tuneLength: number;
  instruments: Instrument[];
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

function shutdownApp() {
  invoke("shutdown_app").then(() => {});
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

  createModule: async (numChannels: number) => {
    return await invoke<Module>("create_module", {
      numChannels,
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
      ).then(() => shutdownApp());
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

  getTransportState: async (): Promise<TransportState> => {
    return await invoke("get_transport_state");
  },

  getPattern: async (
    patternNo: number,
    numLines: number,
    numChannels: number,
  ): Promise<PatternLine[]> => {
    return await invoke("get_pattern", {
      patternNo,
      numLines,
      numChannels,
    });
  },

  pollPlaybackEvents: async (): Promise<PlayerEvent[]> => {
    return await invoke("poll_playback_events");
  },

  pollExportEvents: async (): Promise<PlayerEvent[]> => {
    return await invoke("poll_export_events");
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
    channelNo: number,
  ): Promise<PatternEvent> => {
    return await invoke("edit_get_event", {
      patternNo,
      patternIndex,
      channelNo,
    });
  },

  setEvent: async (
    patternNo: number,
    patternIndex: number,
    channelNo: number,
    newEvent: PatternEvent,
  ) => {
    return await invoke("edit_set_event", {
      patternNo,
      patternIndex,
      channelNo,
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
};
