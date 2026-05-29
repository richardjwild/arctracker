import { useStore } from "../store/useStore.ts";
import { useEffect } from "react";
import { poller } from "../polling/poller.ts";
import { audioExport } from "../audioExport/audioExport.ts";

export default function useExportMonitoring() {
  const { exportMonitoring, exportState } = useStore((state) => state);

  useEffect(() => {
    if (exportState) audioExport.handleStateChange(exportState);
  }, [exportState]);

  useEffect(() => {
    if (!exportMonitoring) return;
    console.log('registering audio export poller');
    return poller.registerPoller(audioExport.poller)
  }, [exportMonitoring]);
}