export type Renderer = (timestamp: number) => void;

const renderers = new Set<Renderer>();
let running = false;

function frame(timestamp: number) {
  for (const renderer of renderers) {
    renderer(timestamp);
  }
  if (running) requestAnimationFrame(frame);
}

export const animation = {
  registerRenderer: (renderer: Renderer) => {
    renderers.add(renderer);
    return () => {
      renderers.delete(renderer);
    };
  },

  start: () => {
    if (running) return;
    running = true;
    requestAnimationFrame(frame);
  },

  stop: () => {
    running = false;
  },
};
