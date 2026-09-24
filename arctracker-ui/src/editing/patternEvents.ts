import { useStore } from "../store/useStore.ts";
import { engine } from "../engine/engine.ts";
import { editor, EditCommand } from "./editor.ts";
import { cursor, Cursor, CursorField } from "./cursor.ts";
import { hexadecimal } from "../rendering/hexadecimal.ts";
import { patternGrid } from "./patternGrid.ts";
import { selection } from "./selection.ts";
import { sequence } from "./sequence.ts";

export type Effect = {
  effectCode: number[];
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
}: {
  eventLocation: EventLocation | null;
  buildEvent: (before: PatternEvent) => PatternEvent;
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
      apply: async () => {
        if (!eventsEqual(currentEvent, updatedEvent)) {
          await engine.setEvent(
            location.patternNo,
            location.patternIndex,
            location.track,
            updatedEvent,
          );
          useStore.getState().patternRevised();
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
    a.effectCode[0] === b.effectCode[0] &&
    a.effectCode[1] === b.effectCode[1] &&
    a.effectData[0] === b.effectData[0] &&
    a.effectData[1] === b.effectData[1]
  );
}

function copyEffects(effects: Effect[]) {
  let copy: Effect[] = [];
  for (const effect of effects) {
    copy.push({
      effectCode: [...effect.effectCode],
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
      effectCode: [0, 0],
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
    });
    await editor.applyEdit(command);
    patternGrid.moveDown(true);
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
    if (field.field !== "effectCode1" && field.field !== "effectCode2") return false;
    const numberValue = hexadecimal.fromHexDigit(value);
    if (numberValue === null) return;
    const effectIndex = field.effectIndex;
    const command = await buildEventEditCommand({
      eventLocation: null,
      buildEvent: (currentEvent) => {
        if (effectIndex < 0 || effectIndex >= currentEvent.effects.length) {
          return currentEvent;
        }
        let effects = copyEffects(currentEvent.effects);
        const codeIndex = field.field === "effectCode1" ? 0 : 1;
        effects[effectIndex].effectCode[codeIndex] = numberValue;
        const newEvent = {
          ...currentEvent,
          effects,
        };
        console.log('currentEvent', currentEvent);
        console.log('newEvent', newEvent);
        return newEvent;
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
          field === "effectCode1" ||
          field === "effectCode2" ||
          field === "effectData1" ||
          field === "effectData2"
        ) {
          effects[cursorField.effectIndex].effectCode = [0, 0];
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

  setMultipleEffects: async (effectLane: number, effect: Effect, noteOnsOnly: boolean) => {
    const moduleSequence = useStore.getState().sequence;
    const patternNo = moduleSequence[sequence.currentPosition()];
    const cursorPosition = cursor.currentPosition();
    const selectedBounds = selection.patternSelectionBounds() || {
      top: cursorPosition.patternIndex,
      bottom: cursorPosition.patternIndex,
      left: cursorPosition.track,
      right: cursorPosition.track,
    };
    console.log('setMultipleEffects', patternNo, selectedBounds, effectLane, effect, noteOnsOnly);
    const updatedEvents: { location: EventLocation; event: PatternEvent }[] = [];
    for (let track = selectedBounds.left; track <= selectedBounds.right; track++) {
      for (let patternIndex = selectedBounds.top; patternIndex <= selectedBounds.bottom; patternIndex++) {
        const event = await patternEvents.getEvent(patternNo, patternIndex, track);
        if (!noteOnsOnly || event.note > 0) {
          const location = {
            patternNo,
            patternIndex,
            track,
          }
          const effects = [ ...event.effects ];
          effects[effectLane] = effect;
          const updatedEvent = {
            ...event,
            effects,
          };
          updatedEvents.push({ location, event: updatedEvent });
        }
      }
    }
    console.log('updatedEvents', updatedEvents);
    void patternEvents.setEvents(updatedEvents);
  },
};
