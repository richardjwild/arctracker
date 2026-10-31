import "./theme.css";
import "./App.css";
import ModuleHeader from "./view/patterneditor/panels/ModuleHeader/ModuleHeader.tsx";
import TransportBar from "./view/patterneditor/panels/TransportBar/TransportBar.tsx";
import PatternView from "./view/patterneditor/panels/PatternView/PatternView.tsx";
import InstrumentList from "./view/patterneditor/panels/InstrumentList/InstrumentList.tsx";
import ExportDialog from "./view/global/dialogs/ExportDialog/ExportDialog.tsx";
import Sequence from "./view/patterneditor/panels/Sequence/Sequence.tsx";
import SampleEditDialog from "./view/patterneditor/dialogs/SampleEditDialog/SampleEditDialog.tsx";
import EditPatternLength from "./view/patterneditor/dialogs/EditPatternLength/EditPatternLength.tsx";
import EditModuleMetaData from "./view/patterneditor/dialogs/EditModuleMetaData/EditModuleMetaData.tsx";
import EditTrackCount from "./view/patterneditor/dialogs/EditTrackCount/EditTrackCount.tsx";
import MasterGain from "./view/patterneditor/panels/MasterGain/MasterGain.tsx";
import EditTempo from "./view/patterneditor/dialogs/EditTempo/EditTempo.tsx";
import EditAppConfig from "./view/global/dialogs/EditAppConfig/EditAppConfig.tsx";
import KeyboardOctave from "./view/patterneditor/panels/KeyboardOctave/KeyboardOctave.tsx";
import UserMessages from "./view/patterneditor/panels/UserMessages/UserMessages.tsx";
import HexCalculator from "./view/patterneditor/dialogs/HexCalculator/HexCalculator.tsx";
import SetMultipleEffects from "./view/patterneditor/dialogs/SetMultipleEffects/SetMultipleEffects.tsx";
import CommandReference from "./view/patterneditor/dialogs/CommandReference/CommandReference.tsx";
import GoToPosition from "./view/patterneditor/dialogs/GoToPosition/GoToPosition.tsx";

export default function AppLayout() {
  return (
    <main className="container">
      <ModuleHeader />
      <MasterGain />
      <Sequence />
      <TransportBar />
      <PatternView />
      <InstrumentList />
      <HexCalculator />
      <UserMessages />
      <KeyboardOctave />
      <SampleEditDialog />
      <ExportDialog />
      <EditPatternLength />
      <EditModuleMetaData />
      <EditTrackCount />
      <EditTempo />
      <EditAppConfig />
      <SetMultipleEffects />
      <GoToPosition />
      <CommandReference />
    </main>
  );
}
