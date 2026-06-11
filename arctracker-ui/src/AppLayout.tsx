import "./theme.css";
import "./App.css";
import ModuleHeader from "./view/ModuleHeader.tsx";
import TransportBar from "./view/TransportBar.tsx";
import PatternView from "./view/PatternView.tsx";
import InstrumentList from "./view/InstrumentList.tsx";
import ExportDialog from "./view/ExportDialog.tsx";
import Sequence from "./view/Sequence.tsx";

export default function AppLayout() {
  return (
    <main className="container">
      <ModuleHeader />
      <TransportBar />
      <Sequence />
      <PatternView />
      <InstrumentList />
      <ExportDialog />
    </main>
  );
}
