import { PatternEvent } from "./patternEvents.ts";

export type PatternEventBlock = {
  width: number;
  height: number;
  events: PatternEvent[][];
}

export type PasteBufferObjectType =
  | { type: "patternEvents"; block: PatternEventBlock };
