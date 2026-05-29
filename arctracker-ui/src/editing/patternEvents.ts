import { useStore } from "../store/useStore.ts";
import { engine, Effect, PatternEvent } from "../engine/engine.ts";
import { editor, EditCommand, EditType } from "./editor.ts";
import { Cursor, CursorField } from "./cursor.ts";
import { hexadecimal } from "../rendering/hexadecimal.ts";
import { patternGrid } from "./patternGrid.ts";

async function applyPatternEventEdit({
  buildEvent,
  postAction,
}: {
  buildEvent: (before: PatternEvent) => PatternEvent;
  postAction: (() => void) | null;
}) {
  const { editorState, transportState } = useStore.getState();
  if (!editorState.editing) return;
  try {
    const currentEvent: PatternEvent = await engine.getEvent(
      transportState.patternNo,
      transportState.patternIndex,
      editorState.cursorPosition.track,
    );
    const updatedEvent = buildEvent(currentEvent);
    const command: EditCommand = {
      type: EditType.EventEdit,
      patternNo: transportState.patternNo,
      patternIndex: transportState.patternIndex,
      track: editorState.cursorPosition.track,
      before: currentEvent,
      after: updatedEvent,
    };
    await editor.applyEdit(command);
    if (postAction) postAction();
  } catch (err) {
    throw err;
  }
}

function copyEffects(effects: Effect[]) {
  let copy: Effect[] = [];
  for (const effect of effects) {
    copy.push({
      effectCode: effect.effectCode,
      effectData: [...effect.effectData],
    });
  }
  return copy;
}

export const patternEvents = {
  setNote: async (note: number) => {
    const selectedSample = useStore.getState().selectedSample;
    if (selectedSample === null) return;
    await applyPatternEventEdit({
      buildEvent: () => {
        return {
          note,
          sampleNo: selectedSample + 1,
          effects: Array.from({ length: 4 }, () => ({
            effectCode: "",
            effectData: [0, 0],
          })),
        };
      },
      postAction: () => patternGrid.moveDown(true),
    });
  },

  setSampleField: async (field: CursorField, value: string) => {
    const numberValue = hexadecimal.fromHexDigit(value);
    if (numberValue === null) return;
    await applyPatternEventEdit({
      buildEvent: (currentEvent) => {
        const newSampleNo =
          field.field === "sampleHigh"
            ? (currentEvent.sampleNo & 0xf) + (numberValue << 4)
            : (currentEvent.sampleNo & 0xf0) + numberValue;
        return {
          ...currentEvent,
          sampleNo: newSampleNo,
        };
      },
      postAction: null,
    });
  },

  setEffectCode: async (field: CursorField, value: string) => {
    if (field.field !== "effectCode") return false;
    const effectIndex = field.effectIndex;
    await applyPatternEventEdit({
      buildEvent: (currentEvent) => {
        if (effectIndex < 0 || effectIndex >= currentEvent.effects.length) {
          return currentEvent;
        }
        let effects = copyEffects(currentEvent.effects);
        effects[effectIndex].effectCode = value.toUpperCase();
        return {
          ...currentEvent,
          effects,
        };
      },
      postAction: null,
    });
  },

  setEffectData: async (field: CursorField, value: string) => {
    if (field.field !== "effectData1" && field.field !== "effectData2") return false;
    const numberValue = hexadecimal.fromHexDigit(value);
    if (numberValue === null) return;
    await applyPatternEventEdit({
      buildEvent: (currentEvent) => {
        if (field.effectIndex < 0 || field.effectIndex >= currentEvent.effects.length) {
          return currentEvent;
        }
        let effects = copyEffects(currentEvent.effects);
        const dataIndex = field.field === "effectData1" ? 0 : 1;
        effects[field.effectIndex].effectData[dataIndex] = numberValue;
        return {
          ...currentEvent,
          effects,
        };
      },
      postAction: null,
    });
  },

  clearPatternEventField: async () => {
    const { editorState } = useStore.getState();
    if (!editorState.editing) return;
    await applyPatternEventEdit({
      buildEvent: (currentEvent) => {
        const cursorField = new Cursor().currentField();
        const field = cursorField.field;
        let effects = copyEffects(currentEvent.effects);
        if (field === "effectCode" || field === "effectData1" || field === "effectData2") {
          effects[cursorField.effectIndex].effectCode = "";
          effects[cursorField.effectIndex].effectData = [0, 0];
        }
        return {
          ...currentEvent,
          note: (field === "note" ? 0 : currentEvent.note),
          sampleNo: (field === "sampleHigh" || field === "sampleLow" ? 0 : currentEvent.sampleNo),
          effects,
        };
      },
      postAction: null,
    })
  },

  clearPatternEvent: async () => {
    const { editorState } = useStore.getState();
    if (!editorState.editing) return;
    await applyPatternEventEdit({
      buildEvent: () => {
        return {
          note: 0,
          sampleNo: 0,
          effects: Array.from({ length: 4 }, () => ({
            effectCode: "",
            effectData: [0, 0],
          })),
        };
      },
      postAction: null,
    })
  },
}
