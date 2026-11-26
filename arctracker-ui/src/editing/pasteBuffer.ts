import { PatternEvent } from "./patternEvents.ts";

export type SelectionType = "patternEventSelection" | "track" | "pattern";

export type PatternEventBlock = {
  width: number;
  height: number;
  events: PatternEvent[][];
}

export type PasteBufferObjectType =
  | { type: SelectionType; block: PatternEventBlock };
