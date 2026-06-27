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
import EditNameAndAuthor from "./view/EditNameAndAuthor.tsx";

export default function AppLayout() {
  return (
    <main className="container">
      <ModuleHeader />
      <TransportBar />
      <Sequence />
      <PatternView />
      <InstrumentList />
      <SampleEditDialog />
      <ExportDialog />
      <EditPatternLength />
      <EditNameAndAuthor />
    </main>
  );
}
