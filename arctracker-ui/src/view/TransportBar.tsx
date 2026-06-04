import { useStore } from "../store/useStore.ts";
import "./TransportBar.css";
import { commands } from "../control/commands.ts";

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
      <span className="visually-hidden">Start playback</span>
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
      <span className="visually-hidden">Pause playback</span>
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
      <span className="visually-hidden">Turn repeat on</span>
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
      <span className="visually-hidden">Turn repeat off</span>
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
      <span className="visually-hidden">Seek forwards 1 pattern</span>
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
      <span className="visually-hidden">Seek backwards 1 pattern</span>
    </>
  );

  // const RecordIcon = () => (
  //   <>
  //     <svg
  //       xmlns="http://www.w3.org/2000/svg"
  //       width="19px"
  //       height="19px"
  //       viewBox="0 -960 960 960"
  //       fill="currentColor"
  //     >
  //       <path d="M480-480ZM282-282q-82-82-82-198t82-198q82-82 198-82t198 82q82 82 82 198t-82 198q-82 82-198 82t-198-82Zm339.5-56.5Q680-397 680-480t-58.5-141.5Q563-680 480-680t-141.5 58.5Q280-563 280-480t58.5 141.5Q397-280 480-280t141.5-58.5Z" />
  //     </svg>
  //     <span className="visually-hidden">Start recording</span>
  //   </>
  // );

  const playing = useStore((state) => state.transportState.playing);
  const looping = useStore((state) => state.transportState.looping);
  const sequencePos = useStore((state) => state.transportState.sequencePos);
  const tuneLength = useStore((state) => state.sequence.length);

  return (
    <div className="transportBar uiArea padded">
      <button
        title="Play/pause"
        onClick={() => commands.togglePlay()}
        aria-label="Play/Pause"
        className="playPause"
      >
        {playing ? <PauseIcon /> : <PlayIcon />}
      </button>
      <button
        title="Repeat on/off"
        onClick={() => commands.toggleLoop()}
        aria-label="Repeat on/off"
        className="repeatOnOff"
      >
        {looping ? <TurnRepeatOffIcon /> : <TurnRepeatOnIcon />}
      </button>
      <button
        title="Seek forwards"
        onClick={() => commands.sequenceSeekForwards()}
        aria-label="Fast forward"
        className="fastForward"
      >
        <FastForwardIcon />
      </button>
      <button
        title="Seek backwards"
        onClick={() => commands.sequenceSeekBackwards()}
        aria-label="Rewind"
        className="rewind"
      >
        <RewindIcon />
      </button>
      {/*<button title="Record MIDI" onClick={() => {}} aria-label="Record">*/}
      {/*  <RecordIcon />*/}
      {/*</button>*/}
      <div className="transportState">
        {tuneLength && (
          <span>
            Pos {sequencePos + 1}/{tuneLength}
          </span>
        )}
      </div>
    </div>
  );
}
