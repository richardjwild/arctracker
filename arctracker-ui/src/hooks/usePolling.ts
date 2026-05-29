import { useEffect } from "react";
import { poller } from "../polling/poller.ts";
import { transport } from "../transport/transport.ts";
import { controller } from "../control/controller.ts";
import { playerEvents } from "../engine/playerEvents.ts";

export default function usePolling() {
  useEffect(() => {
    const deregisterAll = [
      poller.registerPoller(controller.commandPoller),
      poller.registerPoller(transport.transportStatePoller),
      poller.registerPoller(playerEvents.poller),
    ];
    poller.start();
    return () => {
      for (const deregister of deregisterAll) deregister();
      poller.stop();
    };
  }, []);
}
