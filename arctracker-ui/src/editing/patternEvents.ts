import { useStore } from "../store/useStore.ts";
import { engine } from "../engine/engine.ts";
import { editor, EditCommand } from "./editor.ts";
import { Cursor, CursorField } from "./cursor.ts";
import { hexadecimal } from "../rendering/hexadecimal.ts";
import { patternGrid } from "./patternGrid.ts";

export type Effect = {
  effectCode: string;
  effectData: number[];
}

export type PatternEvent = {
  note: number;
  sampleNo: number;
  effects: Effect[];
}

export type PatternLine = {
  row: number;
  events: PatternEvent[];
}

export type EventEdit = {
  patternNo: number;
  patternIndex: number;
  track: number;
  before: PatternEvent;
  after: PatternEvent;
};

export type EventLocation = {
  patternNo: number;
  patternIndex: number;
  track: number;
};

async function buildEventEditCommand({
  eventLocation,
  buildEvent,
  postApply,
}: {
  eventLocation: EventLocation | null;
  buildEvent: (before: PatternEvent) => PatternEvent;
  postApply?: () => void;
}): Promise<EditCommand> {
  const { sequence } = useStore.getState();
  const { sequencePosition } = useStore.getState().editorState;
  try {
    const currentPosition = patternGrid.currentPosition();
    const location = eventLocation || {
      patternNo: sequence[sequencePosition],
      patternIndex: currentPosition.patternIndex,
      track: currentPosition.track,
    };
    const currentEvent: PatternEvent = await patternEvents.getEvent(
      location.patternNo,
      location.patternIndex,
      location.track,
    );
    const updatedEvent = buildEvent(currentEvent);
    return {
      apply: async (redoing: boolean) => {
        if (!eventsEqual(currentEvent, updatedEvent)) {
          await engine.setEvent(
            location.patternNo,
            location.patternIndex,
            location.track,
            updatedEvent,
          );
          useStore.getState().patternRevised();
          if (postApply && !redoing) postApply();
          return true;
        }
        return false;
      },
      undo: async () => {
        await engine.setEvent(
          location.patternNo,
          location.patternIndex,
          location.track,
          currentEvent,
        );
        useStore.getState().patternRevised();
      },
    };
  } catch (err) {
    throw err;
  }
}

function buildMultipleEventEditCommand(eventEdits: EventEdit[]): EditCommand {
  return {
    apply: async () => {
      let revised = false;
      for (const edit of eventEdits) {
        if (!eventsEqual(edit.before, edit.after)) {
          await engine.setEvent(
            edit.patternNo,
            edit.patternIndex,
            edit.track,
            edit.after,
          );
          revised = true;
        }
      }
      if (revised) useStore.getState().patternRevised();
      return revised;
    },
    undo: async () => {
      for (const edit of eventEdits) {
        await engine.setEvent(
          edit.patternNo,
          edit.patternIndex,
          edit.track,
          edit.before,
        );
      }
      useStore.getState().patternRevised();
    },
  };
}

function eventsEqual(a: PatternEvent, b: PatternEvent): boolean {
  return (
    a.note === b.note &&
    a.sampleNo === b.sampleNo &&
    a.effects.length === b.effects.length &&
    a.effects.every((effect, i) => effectsEqual(effect, b.effects[i]))
  );
}

function effectsEqual(a: Effect, b: Effect): boolean {
  return (
    a.effectCode === b.effectCode &&
    a.effectData[0] === b.effectData[0] &&
    a.effectData[1] === b.effectData[1]
  );
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
  editing: () => {
    return useStore.getState().editorState.editMode === "patternEvents";
  },

  getEvent: async (patternNo: number, patternIndex: number, track: number) =>
    await engine.getEvent(patternNo, patternIndex, track),

  setEventNote: async (note: number) => {
    if (!patternEvents.editing()) return;
    const selectedSample = useStore.getState().selectedInstrument;
    if (selectedSample === null) return;
    const command = await buildEventEditCommand({
      eventLocation: null,
      buildEvent: () => {
        return {
          ...emptyEvent(),
          note: note + 1,
          sampleNo: selectedSample + 1,
        };
      },
      postApply: () => patternGrid.moveDown(true),
    });
    await editor.applyEdit(command);
  },

  setEventSample: async (field: CursorField, value: string) => {
    if (!patternEvents.editing()) return;
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
    if (!patternEvents.editing()) return;
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
    if (!patternEvents.editing()) return;
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
    if (!patternEvents.editing()) return;
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
    if (!patternEvents.editing()) return;
    const command = await buildEventEditCommand({
      eventLocation: null,
      buildEvent: emptyEvent,
    });
    await editor.applyEdit(command);
    patternGrid.moveDown(true);
  },

  clearEvents: async (locations: EventLocation[]) => {
    if (!patternEvents.editing()) return;
    const eventEdits: EventEdit[] = [];
    for (const location of locations) {
      const before = await patternEvents.getEvent(
        location.patternNo,
        location.patternIndex,
        location.track,
      );
      const eventEdit: EventEdit = {
        patternNo: location.patternNo,
        patternIndex: location.patternIndex,
        track: location.track,
        before,
        after: emptyEvent(),
      };
      eventEdits.push(eventEdit);
    }
    const eventEditCommand = buildMultipleEventEditCommand(eventEdits);
    await editor.applyEdit(eventEditCommand);
  },

  setEvents: async (
    events: { location: EventLocation; event: PatternEvent }[],
  ) => {
    if (!patternEvents.editing()) return;
    const eventEdits: EventEdit[] = [];
    for (const { location, event } of events) {
      const before = await patternEvents.getEvent(
        location.patternNo,
        location.patternIndex,
        location.track,
      );
      const eventEdit: EventEdit = {
        patternNo: location.patternNo,
        patternIndex: location.patternIndex,
        track: location.track,
        before,
        after: event,
      };
      eventEdits.push(eventEdit);
    }
    const eventEditCommand = buildMultipleEventEditCommand(eventEdits);
    await editor.applyEdit(eventEditCommand);
  },
};
