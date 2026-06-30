import { CurrentPattern } from "../transport/transport.ts";
import { Effect, PatternEvent, PatternLine } from "../engine/engine.ts";
import { useStore } from "../store/useStore.ts";
import { EditorState } from "../editing/editor.ts";
import {
  Cursor,
  FIELDS_PER_EFFECT,
  FIRST_EFFECT_FIELD,
  NOTE_FIELD,
  SAMPLE_HIGH_FIELD,
  SAMPLE_LOW_FIELD
} from "../editing/cursor.ts";
import { hexadecimal } from "./hexadecimal.ts";
import { PatternSelection, selection } from "../editing/selection.ts";
import { patternEvents } from "../editing/patternEvents.ts";
import { notes } from "./notes.ts";

type ViewportSize = { width: number; height: number };

type PatternLayout = {
  leftPadding: number;
  glyphWidth: number;
  rowHeight: number;
  playheadPadding: number;
  rowNumberWidth: number;
  getEventWidth: (track: number) => number;
  maxLines: number;
};

type GridViewportFit = {
  playheadRowHeight: number;
  linesToShow: number;
  playheadLocationOnScreen: number;
}

export type PatternContentDimensions = {
  contentWidth: number;
  contentHeight: number;
};

export function getPatternContentDimensions(
  numTracks: number,
): PatternContentDimensions {
  const patternLayout = getPatternLayout();
  let eventsWidth = 0;
  for (let track = 0; track < numTracks; track++)
    eventsWidth += patternLayout.getEventWidth(track);
  return {
    contentWidth:
      patternLayout.leftPadding +
      patternLayout.rowNumberWidth +
      eventsWidth,
    contentHeight: patternLayout.maxLines * patternLayout.rowHeight,
  };
}

function getPatternLayout(): PatternLayout {
  const leftPadding = 10;
  const glyphHeight = 20;
  const glyphWidth = 10;
  const effectsDisplayed = useStore.getState().editorState.effectsDisplayed;
  return {
    leftPadding,
    glyphWidth,
    rowHeight: glyphHeight,
    playheadPadding: 2,
    rowNumberWidth: glyphWidth * 5,
    getEventWidth: (track: number) => ((glyphWidth * 8) + (effectsDisplayed[track] * glyphWidth * 4)),
    maxLines: 1000,
  };
}

function cssColour(name: string): string {
  return getComputedStyle(document.documentElement)
    .getPropertyValue(name)
    .trim();
}

type Colours = {
  trackLaneSeparator: string;
  playheadBackground: string;
  text: string,
  cursor: string,
  cursorText: string,
  note: string,
  sample: string,
  effect: string,
  selectionBox: string,
  selectionBoxOutline: string,
}

export class PatternRenderer {
  private readonly pattern: CurrentPattern;
  private ctx: CanvasRenderingContext2D;
  private viewportSize: ViewportSize;
  private readonly editorState: EditorState;
  private readonly patternSelection: PatternSelection | null;
  private patternLayout: PatternLayout;
  private readonly numTracks: number;
  private readonly coloursAtPlayhead: Colours;
  private readonly coloursOffPlayhead: Colours;

  public constructor(
    pattern: CurrentPattern,
    ctx: CanvasRenderingContext2D,
    viewportSize: ViewportSize,
    numTracks: number,
  ) {
    this.pattern = pattern;
    this.ctx = ctx;
    this.viewportSize = viewportSize;
    this.numTracks = numTracks;
    this.editorState = useStore.getState().editorState;
    this.patternSelection = useStore.getState().patternSelection;
    this.patternLayout = getPatternLayout();
    this.coloursAtPlayhead = {
      trackLaneSeparator: cssColour("--colour-track-lane-separator"),
      playheadBackground: cssColour("--colour-playhead"),
      text: cssColour("--colour-pattern-text-bright"),
      cursor: cssColour("--colour-cursor"),
      cursorText: cssColour("--colour-cursor-text"),
      note: cssColour("--colour-note-at-playhead"),
      sample: cssColour("--colour-sample-at-playhead"),
      effect: cssColour("--colour-effect-at-playhead"),
      selectionBox: cssColour("--colour-selection-fill"),
      selectionBoxOutline: cssColour("--colour-selection-outline"),
    };
    this.coloursOffPlayhead = {
      trackLaneSeparator: cssColour("--colour-track-lane-separator"),
      playheadBackground: cssColour("--colour-playhead"),
      text: cssColour("--colour-pattern-text-muted"),
      cursor: cssColour("--colour-cursor"),
      cursorText: cssColour("--colour-cursor-text"),
      note: cssColour("--colour-note"),
      sample: cssColour("--colour-sample"),
      effect: cssColour("--colour-effect"),
      selectionBox: cssColour("--colour-selection-fill"),
      selectionBoxOutline: cssColour("--colour-selection-outline"),
    };
  }

  public renderPattern(playheadIndex: number) {
    const gridViewportFit = this.calculateGridViewportFit();
    this.ctx.font = "16px FiraCode";
    this.ctx.textBaseline = "hanging";
    this.ctx.clearRect(0, 0, this.viewportSize.width, this.viewportSize.height);
    this.renderTrackLanes();
    this.renderPlayhead(gridViewportFit);
    if (this.patternSelection) this.renderSelection(gridViewportFit, playheadIndex);
    if (patternEvents.editing()) this.renderCursor(gridViewportFit);
    this.renderPatternLines(gridViewportFit, playheadIndex);
  }

  private calculateGridViewportFit(): GridViewportFit {
    const playheadRowHeight = this.patternLayout.rowHeight + (2 * this.patternLayout.playheadPadding);
    const linesToShow = 1 + Math.floor((this.viewportSize.height - playheadRowHeight) / this.patternLayout.rowHeight);
    return {
      playheadRowHeight,
      linesToShow,
      playheadLocationOnScreen: Math.floor(linesToShow / 2),
    };
  }

  private renderTrackLanes() {
    let x = this.renderRowNumberLane();
    for (let track = 0; track <= this.numTracks; track++) {
      x += this.renderTrackLane(x, track);
    }
  }

  private renderPlayhead(gridViewportFit: GridViewportFit) {
    const y = gridViewportFit.playheadLocationOnScreen * this.patternLayout.rowHeight;
    this.withFillStyle(this.colours().playheadBackground)
      .fillRect(0, y + this.patternLayout.playheadPadding, this.viewportSize.width, this.patternLayout.rowHeight);
  }

  private renderSelection(gridViewportFit: GridViewportFit, playheadIndex: number) {
    const bounds = selection.patternSelectionBounds();
    if (!bounds) return;
    let boxLeft = this.patternLayout.rowNumberWidth;
    for (let track = 0; track < bounds.left; track++)
      boxLeft += this.patternLayout.getEventWidth(track);
    let boxWidth = 0;
    for (let track = bounds.left; track <= bounds.right; track++)
      boxWidth += this.patternLayout.getEventWidth(track);
    const rowOffsetFromPlayhead = bounds.top - playheadIndex;
    const top = (gridViewportFit.playheadLocationOnScreen + rowOffsetFromPlayhead) *
      this.patternLayout.rowHeight +
      this.patternLayout.playheadPadding;
    const boxHeight = (bounds.bottom - bounds.top + 1) * this.patternLayout.rowHeight;
    this.withFillStyle(this.colours().selectionBox).fillRect(boxLeft, top, boxWidth, boxHeight);
    this.withStrokeStyle(this.colours().selectionBoxOutline).strokeRect(boxLeft, top, boxWidth, boxHeight);
  }

  private renderCursor(gridViewportFit: GridViewportFit) {
    const cursorTrack = this.editorState.cursorPosition.track;
    let x = this.patternLayout.leftPadding + this.patternLayout.rowNumberWidth;
    for (let track = 0; track < cursorTrack; track++) {
      x += this.patternLayout.getEventWidth(track);
    }
    const y = (gridViewportFit.playheadLocationOnScreen * this.patternLayout.rowHeight) + this.patternLayout.playheadPadding;
    let cursorX = this.patternLayout.leftPadding + x - this.patternLayout.glyphWidth;
    let cursorWidth = this.patternLayout.glyphWidth;
    const cursor = new Cursor();
    const cursorField = cursor.currentField();
    if (cursorField.field === "note") {
      cursorWidth = this.patternLayout.glyphWidth * 3;
    } else if (cursorField.field === "sampleHigh") {
      cursorX += this.patternLayout.glyphWidth * 4;
    } else if (cursorField.field === "sampleLow") {
      cursorX += this.patternLayout.glyphWidth * 5;
    } else if (cursorField.field === "effectCode") {
      cursorX += this.patternLayout.glyphWidth * (7 + (cursorField.effectIndex * 4));
    } else if (cursorField.field === "effectData1") {
      cursorX += this.patternLayout.glyphWidth * (8 + (cursorField.effectIndex * 4));
    } else if (cursorField.field === "effectData2") {
      cursorX += this.patternLayout.glyphWidth * (9 + (cursorField.effectIndex * 4));
    }
    this.withFillStyle(this.colours().cursor)
      .fillRect(cursorX, y, cursorWidth, this.patternLayout.rowHeight);
  }

  private renderPatternLines(gridViewportFit: GridViewportFit, playheadIndex: number) {
    let y = 0;
    for (let screenLine = 0; screenLine < gridViewportFit.linesToShow; screenLine++) {
      const patternIndex = playheadIndex - gridViewportFit.playheadLocationOnScreen + screenLine;
      const patternLine =
        patternIndex >= 0 && patternIndex < this.pattern.lines.length
          ? this.pattern.lines[patternIndex]
          : null;
      const atPlayhead = (patternIndex === playheadIndex);
      y += this.renderPatternLine(patternLine, y, atPlayhead);
    }
  }

  private renderRowNumberLane(): number {
    const laneWidth = this.patternLayout.leftPadding + this.patternLayout.rowNumberWidth - this.patternLayout.glyphWidth;
    this.withStrokeStyle(this.colours().trackLaneSeparator)
      .renderLine(laneWidth, 0, laneWidth, this.viewportSize.height);
    return laneWidth;
  }

  private renderTrackLane(x: number, track: number): number {
    this.withStrokeStyle(this.colours().trackLaneSeparator)
      .renderLine(x, 0, x, this.viewportSize.height);
    return this.patternLayout.getEventWidth(track);
  }

  private renderPatternLine(line: PatternLine | null, y: number, atPlayhead: boolean): number {
    if (line) {
      const rowNumber = Number(line.row).toString().padStart(3, " ");
      const eventY = atPlayhead ? y + this.patternLayout.playheadPadding : y;
      let x = this.patternLayout.leftPadding;
      x += this.renderRowNumber(rowNumber, x, eventY, atPlayhead);
      let track = 0;
      for (const event of line.events) {
        x += this.renderEvent(track, event, x, eventY, atPlayhead);
        track++;
      }
    }
    return atPlayhead
      ? this.patternLayout.rowHeight + 2 * this.patternLayout.playheadPadding
      : this.patternLayout.rowHeight;
  }

  private renderRowNumber(rowNumber: string, x: number, y: number, atPlayhead: boolean): number {
    this.withFillStyle(this.colours(atPlayhead).text)
      .renderGlyph(rowNumber.charAt(0), x, y)
      .renderGlyph(rowNumber.charAt(1), x + this.patternLayout.glyphWidth, y)
      .renderGlyph(rowNumber.charAt(2), x + this.patternLayout.glyphWidth * 2, y);
    return this.patternLayout.rowNumberWidth;
  }

  private renderEvent(track: number, event: PatternEvent, x: number, y: number, atPlayhead: boolean): number {
    const cursorOnEvent = (atPlayhead && patternEvents.editing() && track === this.editorState.cursorPosition.track);
    x += this.renderNote(x, y, event.note, atPlayhead, cursorOnEvent);
    x += this.renderSample(x, y, event.sampleNo, atPlayhead, cursorOnEvent);
    for (let effectIndex = 0; effectIndex < this.editorState.effectsDisplayed[track]; effectIndex++) {
      x += this.renderEffect(x, y, effectIndex, event.effects[effectIndex], atPlayhead, cursorOnEvent);
    }
    return this.patternLayout.getEventWidth(track);
  }

  private renderNote(x: number, y: number, note: number, atPlayhead: boolean, cursorOnEvent: boolean): number {
    const noteStr = notes.toString(note);
    let colour;
    if (cursorOnEvent && this.editorState.cursorPosition.field === NOTE_FIELD)
      colour = this.colours().cursorText;
    else if (note === 0)
      colour = this.colours(atPlayhead).text;
    else
      colour = this.colours(atPlayhead).note;
    this.withFillStyle(colour)
      .renderGlyph(noteStr.charAt(0), x, y)
      .renderGlyph(noteStr.charAt(1), x + this.patternLayout.glyphWidth, y);
    if (noteStr.length > 2)
      this.renderGlyph(noteStr.charAt(2), x + this.patternLayout.glyphWidth * 2, y);
    return this.patternLayout.glyphWidth * 4;
  }

  private renderSample(x: number, y: number, sampleNo: number, atPlayhead: boolean, cursorOnEvent: boolean) {
    if (sampleNo === 0) {
      x += this.renderSampleDigit(x, y, "-", SAMPLE_HIGH_FIELD, atPlayhead, cursorOnEvent);
      this.renderSampleDigit(x, y, "-", SAMPLE_LOW_FIELD, atPlayhead, cursorOnEvent);
    } else {
      const sampleNumber = hexadecimal.toHex(sampleNo, 2);
      x += this.renderSampleDigit(x, y, sampleNumber.charAt(0), SAMPLE_HIGH_FIELD, atPlayhead, cursorOnEvent);
      this.renderSampleDigit(x, y, sampleNumber.charAt(1), SAMPLE_LOW_FIELD, atPlayhead, cursorOnEvent);
    }
    return this.patternLayout.glyphWidth * 3;
  }

  private renderSampleDigit(x: number, y: number, digit: string, field: number, atPlayhead: boolean, cursorOnEvent: boolean): number {
    let colour;
    if (cursorOnEvent && this.editorState.cursorPosition.field === field) {
      colour = this.colours().cursorText;
    } else {
      colour = (digit === "-") ? this.colours(atPlayhead).text : this.colours(atPlayhead).sample;
    }
    this.withFillStyle(colour).renderGlyph(digit, x, y);
    return this.patternLayout.glyphWidth;
  }

  private renderEffect(x: number, y: number, effectIndex: number, effect: Effect, atPlayhead: boolean, cursorOnEvent: boolean): number {
    if (effect.effectCode === "" && effect.effectData[0] === 0 && effect.effectData[1] === 0) {
      this.withFillStyle(this.colours(atPlayhead).text);
      x += this.renderEffectField(x, y, effectIndex, "-", atPlayhead, cursorOnEvent, 0);
      x += this.renderEffectField(x, y, effectIndex, "-", atPlayhead, cursorOnEvent, 1);
      this.renderEffectField(x, y, effectIndex, "-", atPlayhead, cursorOnEvent, 2);
    } else {
      this.withFillStyle(this.colours(atPlayhead).effect);
      x += this.renderEffectField(x, y, effectIndex, effect.effectCode, atPlayhead, cursorOnEvent, 0);
      x += this.renderEffectField(x, y, effectIndex, hexadecimal.toHex(effect.effectData[0]), atPlayhead, cursorOnEvent, 1);
      this.renderEffectField(x, y, effectIndex, hexadecimal.toHex(effect.effectData[1]), atPlayhead, cursorOnEvent, 2);
    }
    return this.patternLayout.glyphWidth * (FIELDS_PER_EFFECT + 1);
  }

  private renderEffectField(x: number, y: number, effectIndex: number, effectParam: string, atPlayhead: boolean, cursorOnEvent: boolean, effectField: number): number {
    const cursorField = FIRST_EFFECT_FIELD + (effectIndex * 3) + effectField;
    let colour;
    if (cursorOnEvent && this.editorState.cursorPosition.field === cursorField)
      colour = this.colours().cursorText;
    else
      colour = (effectParam === "-") ? this.colours(atPlayhead).text : this.colours(atPlayhead).effect;
    this.withFillStyle(colour).renderGlyph(effectParam, x, y)
    return this.patternLayout.glyphWidth;
  }

  private colours(atPlayhead: boolean = false): Colours {
    return atPlayhead ? this.coloursAtPlayhead : this.coloursOffPlayhead;
  }

  private withFillStyle(fillStyle: string): PatternRenderer {
    this.ctx.fillStyle = fillStyle;
    return this;
  }

  private withStrokeStyle(strokeStyle: string): PatternRenderer {
    this.ctx.strokeStyle = strokeStyle;
    return this;
  }

  private fillRect(x: number, y: number, width: number, height: number): PatternRenderer {
    this.ctx.fillRect(x, y, width, height);
    return this;
  }

  private strokeRect(x: number, y: number, width: number, height: number): PatternRenderer {
    this.ctx.strokeRect(x, y, width, height);
    return this;
  }

  private renderLine(startX: number, startY: number, endX: number, endY: number): PatternRenderer {
    this.ctx.beginPath();
    this.ctx.moveTo(startX, startY);
    this.ctx.lineTo(endX, endY);
    this.ctx.stroke();
    return this;
  }

  private renderGlyph(glyph: string, x: number, y: number): PatternRenderer {
    this.ctx.fillText(glyph, x, y);
    return this;
  }
}
