import { useStore } from "../store/useStore.ts";
import { engine, Effect, PatternEvent } from "../engine/engine.ts";
import { editor, EditCommand, EditType, EventEditCommand } from "./editor.ts";
import { Cursor, CursorField } from "./cursor.ts";
import { hexadecimal } from "../rendering/hexadecimal.ts";
import { patternGrid } from "./patternGrid.ts";

export type EventLocation = {
  patternNo: number;
  patternIndex: number;
  track: number;
};

async function buildEventEditCommand({
  eventLocation,
  buildEvent,
}: {
  eventLocation: EventLocation | null;
  buildEvent: (before: PatternEvent) => PatternEvent;
}): Promise<EventEditCommand> {
  const { transportState } = useStore.getState();
  try {
    const currentPosition = patternGrid.currentPosition();
    const location = eventLocation || {
      patternNo: transportState.patternNo,
      patternIndex: currentPosition.patternIndex,
      track: currentPosition.track,
    };
    const currentEvent: PatternEvent = await engine.getEvent(
      location.patternNo,
      location.patternIndex,
      location.track,
    );
    const updatedEvent = buildEvent(currentEvent);
    return {
      type: EditType.EventEdit,
      patternNo: location.patternNo,
      patternIndex: location.patternIndex,
      track: location.track,
      before: currentEvent,
      after: updatedEvent,
    };
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

function emptyEvent(): PatternEvent {
  return {
    note: 0,
    sampleNo: 0,
    effects: Array.from({ length: 4 }, () => ({
      effectCode: "",
      effectData: [0, 0],
    })),
  };
}

export const patternEvents = {
  setEventNote: async (note: number) => {
    if (!editor.editing()) return;
    const selectedSample = useStore.getState().selectedSample;
    if (selectedSample === null) return;
    const command = await buildEventEditCommand({
      eventLocation: null,
      buildEvent: () => {
        return {
          ...emptyEvent(),
          note,
          sampleNo: selectedSample + 1,
        };
      },
    });
    await editor.applyEdit(command);
    patternGrid.moveDown(true);
  },

  setEventSample: async (field: CursorField, value: string) => {
    if (!editor.editing()) return;
    const numberValue = hexadecimal.fromHexDigit(value);
    if (numberValue === null) return;
    const command = await buildEventEditCommand({
      eventLocation: null,
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
    });
    await editor.applyEdit(command);
  },

  setEventEffectCode: async (field: CursorField, value: string) => {
    if (!editor.editing()) return;
    if (field.field !== "effectCode") return false;
    const effectIndex = field.effectIndex;
    const command = await buildEventEditCommand({
      eventLocation: null,
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
    });
    await editor.applyEdit(command);
  },

  setEventEffectData: async (field: CursorField, value: string) => {
    if (!editor.editing()) return;
    if (field.field !== "effectData1" && field.field !== "effectData2")
      return false;
    const numberValue = hexadecimal.fromHexDigit(value);
    if (numberValue === null) return;
    const command = await buildEventEditCommand({
      eventLocation: null,
      buildEvent: (currentEvent) => {
        if (
          field.effectIndex < 0 ||
          field.effectIndex >= currentEvent.effects.length
        ) {
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
    });
    await editor.applyEdit(command);
  },

  clearEventField: async () => {
    if (!editor.editing()) return;
    const command = await buildEventEditCommand({
      eventLocation: null,
      buildEvent: (currentEvent) => {
        const cursorField = new Cursor().currentField();
        const field = cursorField.field;
        let effects = copyEffects(currentEvent.effects);
        if (
          field === "effectCode" ||
          field === "effectData1" ||
          field === "effectData2"
        ) {
          effects[cursorField.effectIndex].effectCode = "";
          effects[cursorField.effectIndex].effectData = [0, 0];
        }
        return {
          ...currentEvent,
          note: field === "note" ? 0 : currentEvent.note,
          sampleNo:
            field === "sampleHigh" || field === "sampleLow"
              ? 0
              : currentEvent.sampleNo,
          effects,
        };
      },
    });
    await editor.applyEdit(command);
  },

  clearEvent: async () => {
    if (!editor.editing()) return;
    const command = await buildEventEditCommand({
      eventLocation: null,
      buildEvent: emptyEvent,
    });
    await editor.applyEdit(command);
  },

  clearEvents: async (locations: EventLocation[]) => {
    if (!editor.editing()) return;
    const compoundEventEdit: EditCommand = {
      type: EditType.CompoundEventEdit,
      eventEdits: [],
    };
    for (const location of locations) {
      const before = await engine.getEvent(
        location.patternNo,
        location.patternIndex,
        location.track,
      );
      const eventEdit: EventEditCommand = {
        type: EditType.EventEdit,
        patternNo: location.patternNo,
        patternIndex: location.patternIndex,
        track: location.track,
        before,
        after: emptyEvent(),
      };
      compoundEventEdit.eventEdits.push(eventEdit);
    }
    await editor.applyEdit(compoundEventEdit);
  },

  setEvents: async (
    events: { location: EventLocation; event: PatternEvent }[],
  ) => {
    if (!editor.editing()) return;
    const compoundEventEdit: EditCommand = {
      type: EditType.CompoundEventEdit,
      eventEdits: [],
    };
    for (const { location, event } of events) {
      const before = await engine.getEvent(
        location.patternNo,
        location.patternIndex,
        location.track,
      );
      const eventEdit: EventEditCommand = {
        type: EditType.EventEdit,
        patternNo: location.patternNo,
        patternIndex: location.patternIndex,
        track: location.track,
        before,
        after: event,
      };
      compoundEventEdit.eventEdits.push(eventEdit);
    }
    await editor.applyEdit(compoundEventEdit);
  },
};
