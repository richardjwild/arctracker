import "./theme.css";
import "./App.css";
import ModuleHeader from "./view/ModuleHeader.tsx";
import TransportBar from "./view/TransportBar.tsx";
import PatternView from "./view/PatternView.tsx";
import InstrumentList from "./view/InstrumentList.tsx";
import ExportDialog from "./view/ExportDialog.tsx";
import Sequence from "./view/Sequence.tsx";
import SampleEditDialog from "./view/SampleEditDialog.tsx";
import EditPatternLength from "./view/EditPatternLength.tsx";
import EditModuleMetaData from "./view/EditModuleMetaData.tsx";
import EditTrackCount from "./view/EditTrackCount.tsx";
import MasterGain from "./view/MasterGain.tsx";
import EditTempo from "./view/EditTempo.tsx";
import EditAppConfig from "./view/EditAppConfig.tsx";
import KeyboardOctave from "./view/KeyboardOctave.tsx";
import UserMessages from "./view/UserMessages.tsx";
import HexCalculator from "./view/HexCalculator.tsx";

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
    </main>
  );
}
