import ModuleHeader from "./view/ModuleHeader.tsx";
import TransportBar from "./view/TransportBar.tsx";
import PatternView from "./view/PatternView.tsx";
import SampleList from "./view/SampleList.tsx";
import ExportDialog from "./view/ExportDialog.tsx";
import "./App.css";
import Sequence from "./view/Sequence.tsx";

export default function AppLayout() {
  return (
    <main className="container">
      <ModuleHeader />
      <TransportBar />
      <Sequence />
      <PatternView />
      <SampleList />
      <ExportDialog />
    </main>
  );
}