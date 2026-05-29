import { animation } from "../rendering/animation.ts";
import { useEffect } from "react";

export default function useAnimation() {
  useEffect(() => {
    animation.start();
    return animation.stop;
  }, []);
}
