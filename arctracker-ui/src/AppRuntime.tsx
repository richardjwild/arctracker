import React from "react";
import useInitialModule from "./hooks/useInitialModule.ts";
import usePolling from "./hooks/usePolling.ts";
import useMidiPlayback from "./hooks/useMidiPlayback.ts";
import useExportMonitoring from "./hooks/useExportMonitoring.ts";
import useKeyboardInput from "./hooks/useKeyboardInput.ts";
import useAnimation from "./hooks/useAnimation.ts";
import { useExitGuard } from "./hooks/useExitGuard.ts";
import { useMenuActions } from "./hooks/useMenuActions.ts";

interface AppRuntimeProps {
  children: React.ReactNode;
}

export default function AppRuntime({ children }: AppRuntimeProps) {
  useMenuActions();
  useExitGuard();
  useInitialModule();
  usePolling();
  useMidiPlayback();
  useExportMonitoring();
  useKeyboardInput();
  useAnimation();
  return <>{children}</>;
}
