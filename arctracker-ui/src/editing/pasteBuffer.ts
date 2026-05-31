import { PatternEvent } from "../engine/engine.ts";

export type PatternEventBlock = {
  width: number;
  height: number;
  events: PatternEvent[][];
}

export type PasteBufferObjectType =
  | { type: "patternEvents"; block: PatternEventBlock };
