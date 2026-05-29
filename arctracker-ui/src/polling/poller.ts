export type AppPoller = () => void | Promise<void>;

const pollers = new Map<AppPoller, boolean>();
let running = false;

function poll() {
  for (const poller of pollers.keys()) {
    void runPoller(poller);
  }
  if (running) setTimeout(poll, 20);
}

async function runPoller(poller: AppPoller) {
  if (pollers.get(poller)) return;
  pollers.set(poller, true);
  try {
    await poller();
  } finally {
    if (pollers.has(poller)) {
      pollers.set(poller, false);
    }
  }
}

export const poller = {
  registerPoller: (poller: AppPoller) => {
    pollers.set(poller, false);
    return () => {
      pollers.delete(poller);
    };
  },

  start: () => {
    if (running) return;
    running = true;
    poll();
  },

  stop: () => {
    running = false;
  },
};
