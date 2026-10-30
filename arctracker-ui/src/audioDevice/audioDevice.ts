import { engine } from "../engine/engine.ts";
import { SelectedSampleRate } from "../config/appConfig.ts";
import { userMessages } from "../messages/userMessages.ts";
import { message, messageFn2 } from "../language/messages.ts";

export type AudioDeviceInfo = {
  deviceIndex: number;
  name: string;
  hostApiName: string;
}

export type PlaybackState = {
  running: boolean;
  sampleRate: number;
  reportedLatency: number;
}

function printPlaybackState(playbackState: PlaybackState) {
  if (!playbackState.running) {
    userMessages.logMessage({
      type: "warning",
      message: message("playerNotRunning"),
    });
    return;
  }
  const sampleRate = playbackState.sampleRate.toString();
  const reportedLatency = (1000 * playbackState.reportedLatency).toFixed(2).toString();
  userMessages.logMessage({
    type: "info",
    message: messageFn2("outputDeviceOpened")(sampleRate, reportedLatency),
  });
}

export const audioDevice = {
  getAvailableOutputs: async (): Promise<AudioDeviceInfo[]> => {
    return await engine.getAvailableOutputs();
  },

  useDefaultOutputDevice: async (selectedSampleRate: SelectedSampleRate) => {
    const sampleRate = selectedSampleRate === "auto" ? null : selectedSampleRate;
    await engine.useDefaultOutput(sampleRate);
    engine.getPlaybackState().then(printPlaybackState);
  },

  useOutputDevice: async (device: AudioDeviceInfo, selectedSampleRate: SelectedSampleRate) => {
    const sampleRate = selectedSampleRate === "auto" ? null : selectedSampleRate;
    await engine.useOutput(device.deviceIndex, device.name, device.hostApiName, sampleRate);
    engine.getPlaybackState().then(printPlaybackState);
  },
};