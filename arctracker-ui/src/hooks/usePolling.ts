import { useEffect } from "react";
import { poller } from "../polling/poller.ts";
import { controller } from "../control/controller.ts";
import { player } from "../player/player.ts";

export default function usePolling() {
  useEffect(() => {
    const deregisterAll = [
      poller.registerPoller(controller.commandPoller),
      poller.registerPoller(player.snapshotPoller),
      poller.registerPoller(player.eventsPoller),
    ];
    poller.start();
    return () => {
      for (const deregister of deregisterAll) deregister();
      poller.stop();
    };
  }, []);
}
