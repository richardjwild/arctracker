import { invoke } from "@tauri-apps/api/core";
import { PlayerEvent, PlayerSnapshot, Track } from "../player/player.ts";
import { Sample } from "../editing/editInstrument.ts";
import { Module, NewModuleParams } from "../module/module.ts";
import { Effect, PatternEvent, PatternLine } from "../editing/patternEvents.ts";
import { ExportState } from "../audioExport/audioExport.ts";
import { AudioDeviceInfo } from "../audioDevice/audioDevice.ts";
import { MidiDeviceInfo } from "../midi/midi.ts";
import { alerting } from "../alerting/alert.ts";
import { messageFn } from "../language/messages.ts";

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
  playbackAvailable: boolean;
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

export const engine = {
  getCurrentModule: async () => {
    return await invoke<Module>("current_module");
  },

  getAvailableOutputs: async (): Promise<AudioDeviceInfo[]> => {
    return await invoke<AudioDeviceInfo[]>("get_available_outputs");
  },

  useOutput: async (deviceIndex: number, name: string, hostApiName: string) => {
    return await invoke("use_output", {
      deviceIndex,
      name,
      hostApiName,
    });
  },

  useDefaultOutput: async () => {
    return await invoke("use_default_output");
  },

  getAvailableMidiDevices: async (): Promise<MidiDeviceInfo[]> => {
    return await invoke<MidiDeviceInfo[]>("get_available_midi_devices");
  },

  useMidiDevice: async (deviceName: string) => {
    await invoke("use_midi_device", { deviceName });
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

  createModule: async () => {
    return await invoke<Module>("create_module");
  },

  createModuleUsingDefaults: async (params: NewModuleParams) => {
    return await invoke<Module>("create_module_using_defaults", {
      params,
    });
  },

  startPlayer: () => {
    invoke("restart_player").catch((error) => {
      void alerting.showError(messageFn("audioSubsystemFailure")(error));
    });
  },

  setSelectedInstrument: (instrument: number) => {
    void invoke("set_midi_playback_instrument", {
      instrument,
    });
  },

  setSelectedChannel: (channel: number) => {
    void invoke("set_midi_playback_channel", {
      channel,
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
      playbackAvailable: packedSnapshot.playbackAvailable,
      playing: packedSnapshot.playing,
      looping: packedSnapshot.looping,
      currentBpm: packedSnapshot.currentBpm,
      sequencePos: packedSnapshot.sequencePos,
      patternIndex: packedSnapshot.patternIndex,
      patternNo: packedSnapshot.patternNo,
      patternLength: packedSnapshot.patternLength,
      tracks: packedSnapshot.tracks,
      newPattern: packedSnapshot.newPattern
        ? packedSnapshot.newPattern.map(toPatternLine)
        : null,
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

  exportSample: async (instrumentNo: number, exportPath: string) => {
    return await invoke("export_sample", {
      instrumentNo,
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

  setModuleTitle: async (
    name: String,
    author: String,
    defaultPatternLength: number,
  ) => {
    return await invoke("edit_set_module_title", {
      name,
      author,
      defaultPatternLength,
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
    });
  },

  exitSuccessfully: async () => {
    return await invoke("exit_successfully");
  },
};
