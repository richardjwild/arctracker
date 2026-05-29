import { useEffect } from "react";
import { module } from "../module/module.ts";

export default function useInitialModule() {
  useEffect(() => {
    void module.getCurrent();
  }, []);
}
