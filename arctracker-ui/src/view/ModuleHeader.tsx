import { useStore } from "../store/useStore.ts";
import "./ModuleHeader.css";
import { commands } from "../control/commands.ts";
import { Module } from "../module/module.ts";

export default function ModuleHeader() {
  const module = useStore((state) => state.module);
  const isLoadingModule = useStore((state) => state.isLoadingModule);

  const LoadIcon = () => (
    <>
      <svg
        xmlns="http://www.w3.org/2000/svg"
        width="19px"
        height="19px"
        viewBox="0 -960 960 960"
        fill="currentColor"
      >
        <path d="M240-80q-33 0-56.5-23.5T160-160v-640q0-33 23.5-56.5T240-880h320l240 240v240h-80v-200H520v-200H240v640h360v80H240Zm638 15L760-183v89h-80v-226h226v80h-90l118 118-56 57Zm-638-95v-640 640Z" />
      </svg>
      <span className="visually-hidden">Load file</span>
    </>
  );

  const SaveIcon = () => (
    <>
      <svg
        xmlns="http://www.w3.org/2000/svg"
        width="19px"
        height="19px"
        viewBox="0 -960 960 960"
        fill="currentColor"
      >
        <path d="m720-120 160-160-56-56-64 64v-167h-80v167l-64-64-56 56 160 160ZM560 0v-80h320V0H560ZM240-160q-33 0-56.5-23.5T160-240v-560q0-33 23.5-56.5T240-880h280l240 240v121h-80v-81H480v-200H240v560h240v80H240Zm0-80v-560 560Z" />
      </svg>
      <span className="visually-hidden">Save file</span>
    </>
  );

  // const ExportIcon = () => (
  //   <>
  //     <svg
  //       xmlns="http://www.w3.org/2000/svg"
  //       width="19px"
  //       height="19px"
  //       viewBox="0 -960 960 960"
  //       fill="currentColor"
  //     >
  //       <path d="M480-480ZM202-65l-56-57 118-118h-90v-80h226v226h-80v-89L202-65Zm278-15v-80h240v-440H520v-200H240v400h-80v-400q0-33 23.5-56.5T240-880h320l240 240v480q0 33-23.5 56.5T720-80H480Z" />
  //     </svg>
  //     <span className="visually-hidden">Export file</span>
  //   </>
  // );

  const BounceIcon = () => (
    <>
      <svg
        xmlns="http://www.w3.org/2000/svg"
        height="19px"
        viewBox="0 -960 960 960"
        width="19px"
        fill="currentColor"
      >
        <path d="M430-200q38 0 64-26t26-64v-150h120v-80H480v155q-11-8-23.5-11.5T430-380q-38 0-64 26t-26 64q0 38 26 64t64 26ZM240-80q-33 0-56.5-23.5T160-160v-640q0-33 23.5-56.5T240-880h320l240 240v480q0 33-23.5 56.5T720-80H240Zm280-520v-200H240v640h480v-440H520ZM240-800v200-200 640-640Z" />
        <span className="visually-hidden">Bounce audio</span>
      </svg>
    </>
  );

  const EditNameIcon = () => (
    <>
      <svg
        xmlns="http://www.w3.org/2000/svg"
        height="19px"
        viewBox="0 -960 960 960"
        width="19px"
        fill="currentColor"
      >
        <path d="M200-200h57l391-391-57-57-391 391v57Zm-80 80v-170l528-527q12-11 26.5-17t30.5-6q16 0 31 6t26 18l55 56q12 11 17.5 26t5.5 30q0 16-5.5 30.5T817-647L290-120H120Zm640-584-56-56 56 56Zm-141 85-28-29 57 57-29-28Z" />
        <span className="visually-hidden">Bounce audio</span>
      </svg>
    </>
  );

  const formatModuleInfo = (module: Module) => {
    if (module.author) return `${module.author} — ${module.name}`;
    else return module.name;
  };

  return (
    <div className="moduleHeader uiArea padded">
      <button
        title="Load module"
        className="loadButton"
        disabled={isLoadingModule}
        onClick={() => commands.loadFile()}
      >
        <LoadIcon />
      </button>
      <button
        title="Save module"
        className="saveButton"
        onClick={commands.saveModule}
      >
        <SaveIcon />
      </button>
      {/*<button title="Export module" className="exportButton">*/}
      {/*  <ExportIcon />*/}
      {/*</button>*/}
      <button
        title="Bounce audio"
        className="bounceButton"
        onClick={() => commands.exportAudio()}
      >
        <BounceIcon />
      </button>
      <div className="moduleInfo">{formatModuleInfo(module)}</div>
      <button
        title="Edit module details"
        className="editNameAuthorButton"
        onClick={commands.editModuleTitle}
      >
        <EditNameIcon />
      </button>
    </div>
  );
}
