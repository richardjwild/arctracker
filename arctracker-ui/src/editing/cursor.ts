import { useStore } from "../store/useStore.ts";
import { EditorState } from "./editor.ts";

export type CursorPosition = {
  track: number;
  patternIndex: number;
  field: number;
};

export type NoteField = "note";
export type SampleField = "sampleHigh" | "sampleLow";
export type EffectCodeField = "effectCode";
export type EffectDataField = "effectData1" | "effectData2";

export type CursorField =
  | { field: NoteField }
  | { field: SampleField }
  | { field: EffectCodeField; effectIndex: number }
  | { field: EffectDataField; effectIndex: number };

export const NOTE_FIELD = 0;
export const SAMPLE_HIGH_FIELD = 1;
export const SAMPLE_LOW_FIELD = 2;
export const FIRST_EFFECT_FIELD = 3;
export const FIELDS_PER_EFFECT = 3;

export class Cursor {
  editorState: EditorState;
  numTracks: number;

  constructor() {
    const {
      editorState,
      module: { numChannels },
    } = useStore.getState();
    this.editorState = {
      ...editorState,
      cursorPosition: { ...editorState.cursorPosition },
      effectsDisplayed: [...editorState.effectsDisplayed],
    };
    this.numTracks = numChannels;
  }

  public currentPosition(): CursorPosition {
    return this.editorState.cursorPosition;
  }

  public currentField(): CursorField {
    if (this.editorState.cursorPosition.field === NOTE_FIELD) {
      return { field: "note" };
    } else if (this.editorState.cursorPosition.field === SAMPLE_HIGH_FIELD) {
      return { field: "sampleHigh" };
    } else if (this.editorState.cursorPosition.field === SAMPLE_LOW_FIELD) {
      return { field: "sampleLow" };
    } else {
      const relativeField =
        this.editorState.cursorPosition.field - FIRST_EFFECT_FIELD;
      const effectIndex = Math.floor(relativeField / FIELDS_PER_EFFECT);
      if (relativeField % 3 === 0) {
        return { field: "effectCode", effectIndex };
      } else if (relativeField % 3 === 1) {
        return { field: "effectData1", effectIndex };
      } else {
        return { field: "effectData2", effectIndex };
      }
    }
  }

  rightmostFieldFor(track: number): number {
    return (
      FIRST_EFFECT_FIELD + this.editorState.effectsDisplayed[track] * 3 - 1
    );
  }

  public moveFieldLeft() {
    if (this.editorState.cursorPosition.field === 0) {
      if (this.editorState.cursorPosition.track === 0) return;
      this.editorState.cursorPosition.field = this.rightmostFieldFor(
        this.editorState.cursorPosition.track - 1,
      );
      this.editorState.cursorPosition.track -= 1;
    } else {
      this.editorState.cursorPosition.field -= 1;
    }
  }

  public moveFieldRight() {
    if (
      this.editorState.cursorPosition.field ===
      this.rightmostFieldFor(this.editorState.cursorPosition.track)
    ) {
      if (this.editorState.cursorPosition.track === this.numTracks - 1) return;
      this.editorState.cursorPosition.field = 0;
      this.editorState.cursorPosition.track += 1;
    } else {
      this.editorState.cursorPosition.field += 1;
    }
  }

  public moveEventLeft() {
    if (this.editorState.cursorPosition.track > 0) {
      this.editorState.cursorPosition.track -= 1;
    }
    this.ensureCursorStillVisible();
  }

  public moveEventRight() {
    if (this.editorState.cursorPosition.track < this.numTracks - 1) {
      this.editorState.cursorPosition.track += 1;
    }
    this.ensureCursorStillVisible();
  }

  ensureCursorStillVisible() {
    const effectsDisplayed =
      this.editorState.effectsDisplayed[this.editorState.cursorPosition.track];
    const totalFields =
      FIRST_EFFECT_FIELD + effectsDisplayed * FIELDS_PER_EFFECT;
    while (this.editorState.cursorPosition.field >= totalFields)
      this.editorState.cursorPosition.field -= FIELDS_PER_EFFECT;
  }

  public effectsDisplayedDecreased() {
    if (this.editorState.cursorPosition.field < FIRST_EFFECT_FIELD) return;
    const relativeField =
      this.editorState.cursorPosition.field - FIRST_EFFECT_FIELD;
    const effectsDisplayed =
      this.editorState.effectsDisplayed[this.editorState.cursorPosition.track] -
      1;
    if (Math.floor(relativeField / FIELDS_PER_EFFECT) >= effectsDisplayed) {
      this.editorState.cursorPosition.field -= FIELDS_PER_EFFECT;
    }
  }
}

function moveCursor(moveOperation: (cursor: Cursor) => void): CursorPosition {
  let cursor = new Cursor();
  const { editorState, setEditorState } = useStore.getState();
  if (!editorState.editing) return cursor.currentPosition();
  moveOperation(cursor);
  setEditorState({
    ...editorState,
    cursorPosition: cursor.currentPosition(),
  });
  return cursor.currentPosition();
}

export const cursor = {
  moveFieldLeft: (): CursorPosition => {
    return moveCursor((cursor) => cursor.moveFieldLeft());
  },

  moveFieldRight: (): CursorPosition => {
    return moveCursor((cursor) => cursor.moveFieldRight());
  },

  moveEventLeft: (): CursorPosition => {
    return moveCursor((cursor) => cursor.moveEventLeft());
  },

  moveEventRight: (): CursorPosition => {
    return moveCursor((cursor) => cursor.moveEventRight());
  },

  updatePatternIndex: (patternIndex: number) => {
    const { editorState, setEditorState } = useStore.getState();
    if (patternIndex !== editorState.cursorPosition.patternIndex) {
      setEditorState({
        ...editorState,
        cursorPosition: {
          ...editorState.cursorPosition,
          patternIndex,
        },
      });
    }
  },
};
