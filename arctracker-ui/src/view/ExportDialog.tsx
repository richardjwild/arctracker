import { useStore } from "../store/useStore.ts";
import Modal from "./Modal.tsx";
import "./ExportDialog.css";

export default function ExportDialog() {
  const exportMonitoring = useStore((s) => s.exportMonitoring);
  const exportState = useStore((s) => s.exportState);

  if (!exportMonitoring || !exportState) {
    return null;
  }

  return (
    <Modal className="audioExportState">
      {exportState.completed ? (
        <div>Export complete</div>
      ) : (
        <>
          <div>Exporting audio...</div>
          <progress
            value={exportState.percentComplete}
            max={100}
          />
        </>
      )}
    </Modal>
  );
}
