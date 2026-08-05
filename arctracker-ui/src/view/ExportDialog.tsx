import { useStore } from "../store/useStore.ts";
import Modal from "./Modal.tsx";
import "./ExportDialog.css";
import { message } from "../language/messages.ts";

export default function ExportDialog() {
  const exportMonitoring = useStore((s) => s.exportMonitoring);
  const exportState = useStore((s) => s.exportState);

  if (!exportMonitoring || !exportState) {
    return null;
  }

  return (
    <Modal className="audioExportState">
      <div>{message("exportingAudioMessage")}</div>
      <progress value={exportState.percentComplete} max={100} />
    </Modal>
  );
}
