import { useStore } from "../store/useStore.ts";
import "./TransportBar.css";
import { commands } from "../control/commands.ts";
import { message } from "../language/messages.ts";

export default function TransportBar() {
  const PlayIcon = () => (
    <>
      <svg
        xmlns="http://www.w3.org/2000/svg"
        width="18px"
        height="18px"
        viewBox="0 -960 960 960"
        fill="currentColor"
      >
        <path d="M320-200v-560l440 280-440 280Zm80-280Zm0 134 210-134-210-134v268Z" />
      </svg>
      <span className="visually-hidden">{message("startPlaybackHintText")}</span>
    </>
  );

  const PauseIcon = () => (
    <>
      <svg
        xmlns="http://www.w3.org/2000/svg"
        width="19px"
        height="19px"
        viewBox="0 -960 960 960"
        fill="currentColor"
      >
        <path d="M520-200v-560h240v560H520Zm-320 0v-560h240v560H200Zm400-80h80v-400h-80v400Zm-320 0h80v-400h-80v400Zm0-400v400-400Zm320 0v400-400Z" />
      </svg>
      <span className="visually-hidden">{message("pausePlaybackHintText")}</span>
    </>
  );

  const TurnRepeatOnIcon = () => (
    <>
      <svg
        xmlns="http://www.w3.org/2000/svg"
        width="19px"
        height="19px"
        viewBox="0 -960 960 960"
        fill="currentColor"
      >
        <path d="M280-80 120-240l160-160 56 58-62 62h406v-160h80v240H274l62 62-56 58Zm-80-440v-240h486l-62-62 56-58 160 160-160 160-56-58 62-62H280v160h-80Z" />
      </svg>
      <span className="visually-hidden">{message("enableLoopModeHintText")}</span>
    </>
  );

  const TurnRepeatOffIcon = () => (
    <>
      <svg
        xmlns="http://www.w3.org/2000/svg"
        width="19px"
        height="19px"
        viewBox="0 -960 960 960"
        fill="currentColor"
      >
        <path d="M120-40q-33 0-56.5-23.5T40-120v-720q0-33 23.5-56.5T120-920h720q33 0 56.5 23.5T920-840v720q0 33-23.5 56.5T840-40H120Zm160-40 56-58-62-62h486v-240h-80v160H274l62-62-56-58-160 160L280-80Zm-80-440h80v-160h406l-62 62 56 58 160-160-160-160-56 58 62 62H200v240Z" />
      </svg>
      <span className="visually-hidden">{message("disableLoopModeHintText")}</span>
    </>
  );

  const FastForwardIcon = () => (
    <>
      <svg
        xmlns="http://www.w3.org/2000/svg"
        width="19px"
        height="19px"
        viewBox="0 -960 960 960"
        fill="currentColor"
      >
        <path d="M100-240v-480l360 240-360 240Zm400 0v-480l360 240-360 240ZM180-480Zm400 0Zm-400 90 136-90-136-90v180Zm400 0 136-90-136-90v180Z" />
      </svg>
      <span className="visually-hidden">{message("seekSequenceForwardsHintText")}</span>
    </>
  );

  const RewindIcon = () => (
    <>
      <svg
        xmlns="http://www.w3.org/2000/svg"
        width="19px"
        height="19px"
        viewBox="0 -960 960 960"
        fill="currentColor"
      >
        <path d="M860-240 500-480l360-240v480Zm-400 0L100-480l360-240v480Zm-80-240Zm400 0Zm-400 90v-180l-136 90 136 90Zm400 0v-180l-136 90 136 90Z" />
      </svg>
      <span className="visually-hidden">{message("seekSequenceBackwardsHintText")}</span>
    </>
  );

  const SpeedIcon = () => (
    <>
      <svg
        xmlns="http://www.w3.org/2000/svg"
        height="19px"
        viewBox="0 -960 960 960"
        width="19px"
        fill="currentColor"
      >
        <path d="M480-316.5q38-.5 56-27.5l224-336-336 224q-27 18-28.5 55t22.5 61q24 24 62 23.5Zm0-483.5q59 0 113.5 16.5T696-734l-76 48q-33-17-68.5-25.5T480-720q-133 0-226.5 93.5T160-400q0 42 11.5 83t32.5 77h552q23-38 33.5-79t10.5-85q0-36-8.5-70T766-540l48-76q30 47 47.5 100T880-406q1 57-13 109t-41 99q-11 18-30 28t-40 10H204q-21 0-40-10t-30-28q-26-45-40-95.5T80-400q0-83 31.5-155.5t86-127Q252-737 325-768.5T480-800Zm7 313Z" />
      </svg>
    </>
  );

  const playing = useStore((state) => state.transportState.playing);
  const looping = useStore((state) => state.transportState.looping);
  const beatsPerMinute = useStore((state) => state.module.beatsPerMinute);

  return (
    <div className="transportBar uiArea padded">
      <div className="buttons">
        <button
          title={message("togglePlayHintText")}
          onClick={commands.togglePlay}
          aria-label="Play/Pause"
          className="playPause left"
        >
          {playing ? <PauseIcon /> : <PlayIcon />}
        </button>
        <button
          title={message("toggleLoopModeHintText")}
          onClick={commands.toggleLoop}
          aria-label="Repeat on/off"
          className="repeatOnOff"
        >
          {looping ? <TurnRepeatOffIcon /> : <TurnRepeatOnIcon />}
        </button>
        <button
          title={message("seekSequenceForwardsHintText")}
          onClick={commands.sequenceSeekForwards}
          aria-label="Fast forward"
          className="fastForward"
        >
          <FastForwardIcon />
        </button>
        <button
          title={message("seekSequenceBackwardsHintText")}
          onClick={commands.sequenceSeekBackwards}
          aria-label="Rewind"
          className="rewind right"
        >
          <RewindIcon />
        </button>
      </div>
      <button
        title={message("editTempoHintText")}
        className="tempo left right"
        onClick={commands.editTempo}
      >
        {beatsPerMinute === 0 ? <SpeedIcon /> : `${beatsPerMinute}bpm`}
      </button>
    </div>
  );
}
