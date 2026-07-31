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
import EditModuleTitle from "./view/EditModuleTitle.tsx";
import EditTrackCount from "./view/EditTrackCount.tsx";
import MasterGain from "./view/MasterGain.tsx";
import EditTempo from "./view/EditTempo.tsx";

export default function AppLayout() {
  return (
    <main className="container">
      <ModuleHeader />
      <MasterGain />
      <Sequence />
      <TransportBar />
      <PatternView />
      <InstrumentList />
      <SampleEditDialog />
      <ExportDialog />
      <EditPatternLength />
      <EditModuleTitle />
      <EditTrackCount />
      <EditTempo />
    </main>
  );
}
