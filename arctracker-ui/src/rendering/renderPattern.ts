import { CurrentPattern } from "../transport/transport.ts";
import {
  Cursor,
  FIELDS_PER_EFFECT,
  FIRST_EFFECT_FIELD,
  NOTE_FIELD,
  SAMPLE_HIGH_FIELD,
  SAMPLE_LOW_FIELD,
} from "../editing/cursor.ts";
import { hexadecimal } from "./hexadecimal.ts";
import { PatternSelection, selection } from "../editing/selection.ts";
import { Effect, PatternEvent, patternEvents, PatternLine } from "../editing/patternEvents.ts";
import { notes } from "./notes.ts";
import { GridViewportFit, PatternLayout, patternLayout } from "./patternLayout.ts";

type ViewportSize = { width: number; height: number };

export type PatternContentDimensions = {
  contentWidth: number;
  contentHeight: number;
};

export function getPatternContentDimensions(
  viewportSize: ViewportSize,
  numTracks: number,
  effectsDisplayed: number[],
): PatternContentDimensions {
  const layout = patternLayout.getPatternLayout(viewportSize, effectsDisplayed);
  let eventsWidth = 0;
  for (let track = 0; track < numTracks; track++)
    eventsWidth += layout.getEventWidth(track);
  return {
    contentWidth:
      layout.leftPadding + layout.rowNumberWidth + eventsWidth,
    contentHeight: layout.maxLines * layout.rowHeight,
  };
}

export type Colours = {
  background: string;
  beatLine: string;
  trackLaneSeparator: string;
  trackHeaderMutedFg: string;
  trackHeaderMutedBg: string;
  trackHeaderNotMutedFg: string;
  trackHeaderNotMutedBg: string;
  trackFooterMutedFg: string;
  trackFooterMutedBg: string;
  trackFooterNotMutedFg: string;
  trackFooterNotMutedBg: string;
  playheadBackground: string;
  text: string;
  channelMuted: string;
  cursor: string;
  cursorText: string;
  note: string;
  sample: string;
  effect: string;
  selectionBox: string;
  selectionBoxOutline: string;
};

const FULL_CIRCLE = 2 * Math.PI;
const DASH = "–";

export type RenderPatternView = {
  playheadIndex: number;
  pattern: CurrentPattern;
  linesPerBeat: number;
  trackMuteState: boolean[];
  trackPanning: number[];
  cursorTrack: number;
  cursorField: number;
  editing: boolean;
  patternSelection: PatternSelection | null;
};

export class PatternRenderer {
  private ctx: CanvasRenderingContext2D;
  private readonly coloursAtPlayhead: Colours;
  private readonly coloursOffPlayhead: Colours;
  private readonly trackHeaderFont: string;
  private readonly patternDataFont: string;
  private readonly numTracks: number;
  private readonly effectsDisplayed: number[];
  private readonly viewportSize: ViewportSize;
  private readonly layout: PatternLayout;
  private readonly gridViewportFit: GridViewportFit;

  public constructor(
    ctx: CanvasRenderingContext2D,
    coloursAtPlayhead: Colours,
    coloursOffPlayhead: Colours,
    trackHeaderFont: string,
    patternDataFont: string,
    numTracks: number,
    effectsDisplayed: number[],
    viewportSize: ViewportSize,
  ) {
    this.ctx = ctx;
    this.coloursAtPlayhead = coloursAtPlayhead;
    this.coloursOffPlayhead = coloursOffPlayhead;
    this.trackHeaderFont = trackHeaderFont;
    this.patternDataFont = patternDataFont;
    this.numTracks = numTracks;
    this.effectsDisplayed = effectsDisplayed;
    this.viewportSize = viewportSize;
    this.layout = patternLayout.getPatternLayout(viewportSize, effectsDisplayed);
    this.gridViewportFit = patternLayout.calculateGridViewportFit(viewportSize, numTracks, effectsDisplayed);
    this.ctx.textBaseline = "hanging";
  }

  public renderPattern(view: RenderPatternView) {
    try {
      this.clearCanvas();
      if (view.linesPerBeat > 0) this.renderBeatLines(view);
      this.renderTrackLanes();
      this.renderTrackHeaders(view);
      this.renderTrackFooters(view);
      this.renderPlayhead();
      if (view.patternSelection) this.renderSelection(view);
      this.renderCursor(view);
      this.renderPatternLines(view);
    } catch (err) {
      this.renderError(err);
    }
  }

  private clearCanvas() {
    this.ctx.clearRect(0, 0, this.viewportSize.width, this.viewportSize.height);
  }

  private renderBeatLines(view: RenderPatternView) {
    let y = this.layout.trackHeaderHeight;
    let width = this.layout.leftPadding + this.layout.rowNumberWidth - this.layout.glyphWidth;
    for (let track = this.gridViewportFit.firstVisibleTrack; track <= this.gridViewportFit.lastVisibleTrack; track++) {
      width += this.layout.getEventWidth(track);
    }
    for (let screenLine = 0; screenLine < this.gridViewportFit.linesToShow; screenLine++) {
      const patternIndex = view.playheadIndex - this.gridViewportFit.playheadLocationOnScreen + screenLine;
      const atPlayhead = (patternIndex === view.playheadIndex);
      if (patternIndex >= 0 && patternIndex < view.pattern.lines.length && patternIndex % view.linesPerBeat === 0) {
        const lineY = atPlayhead ? y + this.layout.playheadPadding : y;
        this.withFillStyle(this.colours().beatLine)
          .fillRect(0, lineY, width, this.layout.rowHeight);
      }
      y += atPlayhead
        ? this.layout.rowHeight + 2 * this.layout.playheadPadding
        : this.layout.rowHeight;
    }
  }

  private renderTrackLanes() {
    let x = this.renderRowNumberLane();
    for (let track = 0; track <= this.numTracks; track++) {
      x += this.renderTrackLane(x, track);
    }
  }

  private renderTrackHeaders(view: RenderPatternView) {
    this.ctx.font = this.trackHeaderFont;
    let x = this.layout.leftPadding + this.layout.rowNumberWidth - this.layout.glyphWidth;
    for (let track = 0; track <= this.numTracks; track++) {
      x += this.renderTrackHeader(x, track, view.trackMuteState[track]);
    }
  }

  private renderTrackFooters(view: RenderPatternView) {
    this.ctx.font = this.trackHeaderFont;
    let x = this.layout.leftPadding + this.layout.rowNumberWidth - this.layout.glyphWidth;
    for (let track = 0; track <= this.numTracks; track++) {
      x += this.renderTrackFooter(view, x, track, view.trackMuteState[track]);
    }
  }

  private renderPlayhead() {
    const y = this.layout.trackHeaderHeight
      + this.gridViewportFit.playheadLocationOnScreen * this.layout.rowHeight;
    this.withFillStyle(this.colours().playheadBackground)
      .fillRect(0, y + this.layout.playheadPadding, this.viewportSize.width, this.layout.rowHeight);
  }

  private renderSelection(view: RenderPatternView) {
    const bounds = selection.patternSelectionBounds();
    if (!bounds) return;
    let boxLeft = this.layout.rowNumberWidth;
    for (let track = this.gridViewportFit.firstVisibleTrack; track < bounds.left; track++)
      boxLeft += this.layout.getEventWidth(track);
    let boxWidth = 0;
    for (let track = Math.max(bounds.left, this.gridViewportFit.firstVisibleTrack); track <= bounds.right; track++)
      boxWidth += this.layout.getEventWidth(track);
    const rowOffsetFromPlayhead = bounds.top - view.playheadIndex;
    const top = (this.gridViewportFit.playheadLocationOnScreen + rowOffsetFromPlayhead)
      * this.layout.rowHeight
      + this.layout.trackHeaderHeight
      + this.layout.playheadPadding;
    const boxHeight = (bounds.bottom - bounds.top + 1) * this.layout.rowHeight;
    this.withFillStyle(this.colours().selectionBox).fillRect(boxLeft, top, boxWidth, boxHeight);
    this.withStrokeStyle(this.colours().selectionBoxOutline).strokeRect(boxLeft, top, boxWidth, boxHeight);
  }

  private renderCursor(view: RenderPatternView) {
    const cursorTrack = view.cursorTrack;
    let x = this.layout.leftPadding + this.layout.rowNumberWidth;
    for (let track = this.gridViewportFit.firstVisibleTrack; track < cursorTrack; track++) {
      x += this.layout.getEventWidth(track);
    }
    const y = this.layout.trackHeaderHeight
      + (this.gridViewportFit.playheadLocationOnScreen * this.layout.rowHeight)
      + this.layout.playheadPadding;
    if (view.editing) this.renderEditCursor(x, y, cursorTrack);
    else this.renderNonEditCursor(x, y, cursorTrack);
  }

  private renderNonEditCursor(x: number, y: number, cursorTrack: number) {
    this.withStrokeStyle(this.colours().playheadBackground).withLineWidth(2).strokeRect(
      x - 1,
      y - 1,
      2 + this.layout.getEventWidth(cursorTrack) - (this.layout.glyphWidth * 2),
      2 + this.layout.rowHeight
    );
  }

  private renderEditCursor(x: number, y: number, cursorTrack: number) {
    this.withStrokeStyle(this.colours().cursor).withLineWidth(2).strokeRect(
      x - 1,
      y - 1,
      2 + this.layout.getEventWidth(cursorTrack) - (this.layout.glyphWidth * 2),
      2 + this.layout.rowHeight
    );
    let cursorX = this.layout.leftPadding + x - this.layout.glyphWidth;
    let cursorWidth = this.layout.glyphWidth;
    const cursor = new Cursor();
    const cursorField = cursor.currentField();
    if (cursorField.field === "note") {
      cursorWidth = this.layout.glyphWidth * 3;
    } else if (cursorField.field === "sampleHigh") {
      cursorX += this.layout.glyphWidth * 4;
    } else if (cursorField.field === "sampleLow") {
      cursorX += this.layout.glyphWidth * 5;
    } else if (cursorField.field === "effectCode") {
      cursorX += this.layout.glyphWidth * (7 + (cursorField.effectIndex * 4));
    } else if (cursorField.field === "effectData1") {
      cursorX += this.layout.glyphWidth * (8 + (cursorField.effectIndex * 4));
    } else if (cursorField.field === "effectData2") {
      cursorX += this.layout.glyphWidth * (9 + (cursorField.effectIndex * 4));
    }
    this.withFillStyle(this.colours().cursor)
      .fillRect(cursorX, y, cursorWidth, this.layout.rowHeight);
  }

  private renderPatternLines(view: RenderPatternView) {
    this.ctx.font = this.patternDataFont;
    let y = this.layout.trackHeaderHeight;
    for (let screenLine = 0; screenLine < this.gridViewportFit.linesToShow; screenLine++) {
      const patternIndex = view.playheadIndex - this.gridViewportFit.playheadLocationOnScreen + screenLine;
      const patternLine =
        patternIndex >= 0 && patternIndex < view.pattern.lines.length
          ? view.pattern.lines[patternIndex]
          : null;
      const atPlayhead = (patternIndex === view.playheadIndex);
      y += this.renderPatternLine(view, patternLine, y, atPlayhead);
    }
  }

  private renderRowNumberLane(): number {
    const laneWidth = this.layout.leftPadding + this.layout.rowNumberWidth - this.layout.glyphWidth;
    this.withStrokeStyle(this.colours().trackLaneSeparator).withLineWidth(1)
      .renderLine(laneWidth, 0, laneWidth, this.viewportSize.height);
    return laneWidth;
  }

  private renderTrackLane(trackX: number, track: number): number {
    const trackWidth = this.layout.getEventWidth(track);
    if (track < this.gridViewportFit.firstVisibleTrack || track > this.gridViewportFit.lastVisibleTrack)
      return 0;
    this.withStrokeStyle(this.colours().trackLaneSeparator).withLineWidth(1)
      .renderLine(trackX + trackWidth, 0, trackX + trackWidth, this.viewportSize.height);
    return trackWidth;
  }

  private renderTrackHeader(trackX: number, track: number, muted: boolean): number {
    const fgColour = muted ? this.colours().trackHeaderMutedFg : this.colours().trackHeaderNotMutedFg;
    const bgColour = muted ? this.colours().trackHeaderMutedBg : this.colours().trackHeaderNotMutedBg;
    const trackWidth = this.layout.getEventWidth(track);
    if (track < this.gridViewportFit.firstVisibleTrack || track > this.gridViewportFit.lastVisibleTrack)
      return 0;
    this.withFillStyle(bgColour)
      .fillRect(trackX + 1, 0, trackWidth - 2, this.layout.trackHeaderHeight);
    this.withFillStyle(fgColour).renderGlyph(
      (track + 1).toString(),
      trackX + this.layout.glyphWidth / 2,
      2,
    );
    if (muted) this.renderMutedIcon(trackX + trackWidth - 14, 3);
    else this.renderNotMutedIcon(trackX + trackWidth - 14, 3);
    return trackWidth;
  }

  private renderTrackFooter(view: RenderPatternView, trackX: number, track: number, muted: boolean): number {
    const fgColour = muted ? this.colours().trackFooterMutedFg : this.colours().trackFooterNotMutedFg;
    const bgColour = muted ? this.colours().trackFooterMutedBg : this.colours().trackFooterNotMutedBg;
    const trackWidth = this.layout.getEventWidth(track);
    if (track < this.gridViewportFit.firstVisibleTrack || track > this.gridViewportFit.lastVisibleTrack)
      return 0;
    this.withFillStyle(bgColour)
      .fillRect(trackX + 1, this.layout.viewportSize.height - this.layout.trackFooterHeight, trackWidth - 2, this.layout.trackFooterHeight);
    this.withFillStyle(fgColour).renderGlyph(
      "L",
      trackX + this.layout.glyphWidth,
      this.layout.viewportSize.height - this.layout.trackFooterHeight + 2,
    );
    this.withFillStyle(fgColour).renderGlyph(
      "R",
      trackX + trackWidth - this.layout.glyphWidth * 2 + 1,
      this.layout.viewportSize.height - this.layout.trackFooterHeight + 2,
    );
    const centreX = trackX - 1 + trackWidth / 2;
    const sliderThrow = (trackWidth - this.layout.glyphWidth * 5) / 2;
    this.ctx.lineWidth = 1;
    this.withStrokeStyle(fgColour);
    this.ctx.beginPath();
    this.ctx.moveTo(centreX - sliderThrow, this.layout.viewportSize.height - this.layout.trackFooterHeight / 2);
    this.ctx.lineTo(centreX + sliderThrow, this.layout.viewportSize.height - this.layout.trackFooterHeight / 2);
    this.ctx.closePath();
    this.ctx.stroke();
    this.ctx.beginPath();
    this.ctx.moveTo(centreX, this.layout.viewportSize.height - this.layout.trackFooterHeight + 4);
    this.ctx.lineTo(centreX, this.layout.viewportSize.height - 4);
    this.ctx.closePath();
    this.ctx.stroke();
    const trackPanning =
      track >= 0 && track < view.trackPanning.length
        ? view.trackPanning[track] - 128
        : 0;
    const deflection = sliderThrow * trackPanning / 127;
    this.ctx.beginPath();
    this.ctx.arc(centreX + deflection, this.layout.viewportSize.height - this.layout.trackFooterHeight / 2, 5, 0, FULL_CIRCLE);
    this.ctx.fill();
    return trackWidth;
  }

  private renderMutedIcon(x: number, y: number) {
    this.ctx.lineWidth = 1;
    this.withStrokeStyle(this.colours().trackHeaderMutedFg);
    this.ctx.beginPath();
    this.ctx.moveTo(x, y + 2);
    this.ctx.lineTo(x + 10, y + 12);
    this.ctx.closePath();
    this.ctx.stroke();
    this.ctx.beginPath();
    this.ctx.moveTo(x + 10, y + 2);
    this.ctx.lineTo(x, y + 12);
    this.ctx.closePath();
    this.ctx.stroke();
  }

  private renderNotMutedIcon(x: number, y: number) {
    this.withStrokeStyle(this.colours().trackHeaderNotMutedFg);
    this.ctx.lineWidth = 1;
    this.ctx.beginPath();
    this.ctx.moveTo(x, y + 2);
    this.ctx.lineTo(x + 4, y + 5);
    this.ctx.lineTo(x + 8, y + 5);
    this.ctx.lineTo(x + 8, y + 10);
    this.ctx.lineTo(x + 4, y + 10);
    this.ctx.lineTo(x, y + 13);
    this.ctx.closePath();
    this.ctx.stroke();
  }

  private renderPatternLine(view: RenderPatternView, line: PatternLine | null, y: number, atPlayhead: boolean): number {
    if (line) {
      const rowNumber = Number(line.row).toString().padStart(3, " ");
      const eventY = atPlayhead ? y + this.layout.playheadPadding : y;
      let x = this.layout.leftPadding;
      x += this.renderRowNumber(rowNumber, x, eventY, atPlayhead);
      let track = 0;
      for (const event of line.events) {
        x += this.renderEvent(view, track, event, x, eventY, atPlayhead);
        track++;
      }
    }
    return atPlayhead
      ? this.layout.rowHeight + 2 * this.layout.playheadPadding
      : this.layout.rowHeight;
  }

  private renderRowNumber(rowNumber: string, x: number, y: number, atPlayhead: boolean): number {
    this.withFillStyle(this.colours(atPlayhead).text)
      .renderGlyph(rowNumber.charAt(0), x, y)
      .renderGlyph(rowNumber.charAt(1), x + this.layout.glyphWidth, y)
      .renderGlyph(rowNumber.charAt(2), x + this.layout.glyphWidth * 2, y);
    return this.layout.rowNumberWidth;
  }

  private renderEvent(view: RenderPatternView, track: number, event: PatternEvent, x: number, y: number, atPlayhead: boolean): number {
    if (track < this.gridViewportFit.firstVisibleTrack || track > this.gridViewportFit.lastVisibleTrack)
      return 0;
    const cursorOnEvent = (atPlayhead && patternEvents.editing() && track === view.cursorTrack);
    const muted = view.trackMuteState[track];
    x += this.renderNote(view, x, y, event.note, atPlayhead, cursorOnEvent, muted);
    x += this.renderSample(view, x, y, event.sampleNo, atPlayhead, cursorOnEvent, muted);
    for (let effectIndex = 0; effectIndex < this.effectsDisplayed[track]; effectIndex++) {
      x += this.renderEffect(view, x, y, effectIndex, event.effects[effectIndex], atPlayhead, cursorOnEvent, muted);
    }
    return this.layout.getEventWidth(track);
  }

  private renderNote(view: RenderPatternView, x: number, y: number, note: number, atPlayhead: boolean, cursorOnEvent: boolean, muted: boolean): number {
    const noteStr = notes.toString(note);
    let colour;
    if (muted)
      colour = this.colours().channelMuted;
    else if (cursorOnEvent && view.cursorField === NOTE_FIELD)
      colour = this.colours().cursorText;
    else if (note === 0)
      colour = this.colours(atPlayhead).text;
    else
      colour = this.colours(atPlayhead).note;
    if (noteStr === "–––") {
      this.withFillStyle(colour).renderGlyph(noteStr, x, y)
      return this.layout.glyphWidth * 4;
    }
    if (noteStr.length === 2)
      this.withFillStyle(colour)
        .renderGlyph(noteStr.charAt(0), x, y)
        .renderGlyph(noteStr.charAt(1), x + this.layout.glyphWidth, y);
    else if (noteStr.length === 3)
      this.withFillStyle(colour)
        .renderGlyph(noteStr.charAt(0), x, y)
        .renderGlyph(noteStr.charAt(1), x - 2 + this.layout.glyphWidth, y)
        .renderGlyph(noteStr.charAt(2), x - 4 + this.layout.glyphWidth * 2, y);
    return this.layout.glyphWidth * 4;
  }

  private renderSample(view: RenderPatternView, x: number, y: number, sampleNo: number, atPlayhead: boolean, cursorOnEvent: boolean, muted: boolean) {
    if (sampleNo === 0) {
      x += this.renderSampleDigit(view, x, y, DASH, SAMPLE_HIGH_FIELD, atPlayhead, cursorOnEvent, muted);
      this.renderSampleDigit(view, x, y, DASH, SAMPLE_LOW_FIELD, atPlayhead, cursorOnEvent, muted);
    } else {
      const sampleNumber = hexadecimal.toHex(sampleNo, 2);
      x += this.renderSampleDigit(view, x, y, sampleNumber.charAt(0), SAMPLE_HIGH_FIELD, atPlayhead, cursorOnEvent, muted);
      this.renderSampleDigit(view, x, y, sampleNumber.charAt(1), SAMPLE_LOW_FIELD, atPlayhead, cursorOnEvent, muted);
    }
    return this.layout.glyphWidth * 3;
  }

  private renderSampleDigit(view: RenderPatternView, x: number, y: number, digit: string, field: number, atPlayhead: boolean, cursorOnEvent: boolean, muted: boolean): number {
    let colour;
    if (muted)
      colour = this.colours().channelMuted;
    else if (cursorOnEvent && view.cursorField === field) {
      colour = this.colours().cursorText;
    } else {
      colour = (digit === DASH) ? this.colours(atPlayhead).text : this.colours(atPlayhead).sample;
    }
    this.withFillStyle(colour).renderGlyph(digit, x, y);
    return this.layout.glyphWidth;
  }

  private renderEffect(view: RenderPatternView, x: number, y: number, effectIndex: number, effect: Effect, atPlayhead: boolean, cursorOnEvent: boolean, muted: boolean): number {
    if (effect.effectCode === "" && effect.effectData[0] === 0 && effect.effectData[1] === 0) {
      x += this.renderEffectField(view, x, y, effectIndex, DASH, atPlayhead, cursorOnEvent, 0, muted);
      x += this.renderEffectField(view, x, y, effectIndex, DASH, atPlayhead, cursorOnEvent, 1, muted);
      this.renderEffectField(view, x, y, effectIndex, DASH, atPlayhead, cursorOnEvent, 2, muted);
    } else {
      x += this.renderEffectField(view, x, y, effectIndex, effect.effectCode, atPlayhead, cursorOnEvent, 0, muted);
      x += this.renderEffectField(view, x, y, effectIndex, hexadecimal.toHex(effect.effectData[0]), atPlayhead, cursorOnEvent, 1, muted);
      this.renderEffectField(view, x, y, effectIndex, hexadecimal.toHex(effect.effectData[1]), atPlayhead, cursorOnEvent, 2, muted);
    }
    return this.layout.glyphWidth * (FIELDS_PER_EFFECT + 1);
  }

  private renderEffectField(view: RenderPatternView, x: number, y: number, effectIndex: number, effectParam: string, atPlayhead: boolean, cursorOnEvent: boolean, effectField: number, muted: boolean): number {
    const cursorField = FIRST_EFFECT_FIELD + (effectIndex * 3) + effectField;
    let colour;
    if (muted)
      colour = this.colours().channelMuted;
    else if (cursorOnEvent && view.cursorField === cursorField)
      colour = this.colours().cursorText;
    else
      colour = (effectParam === DASH) ? this.colours(atPlayhead).text : this.colours(atPlayhead).effect;
    this.withFillStyle(colour).renderGlyph(effectParam, x, y)
    return this.layout.glyphWidth;
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

  private withLineWidth(lineWidth: number): PatternRenderer {
    this.ctx.lineWidth = lineWidth;
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
    this.ctx.fillText(glyph, x, y + 1);
    return this;
  }

  private renderError(err: any) {
    const cols = this.viewportSize.width / this.layout.glyphWidth;
    const rows = this.viewportSize.height / this.layout.rowHeight;
    if (err instanceof Error) {
      const stack = (err.stack || "").split("\n");
      const longestLine = [...stack, err.message].reduce((a, b) => a.length > b.length ? a : b);
      const textWidth = Math.min(longestLine.length, cols - 2);
      const textHeight = Math.min(stack.length + 3, rows - 2);
      const boxWidth = (textWidth) * this.layout.glyphWidth;
      const boxHeight = (textHeight + 2) * this.layout.rowHeight;
      const boxX = (this.viewportSize.width - boxWidth) / 2;
      const boxY = (this.viewportSize.height - boxHeight) / 2;
      this.withFillStyle("rgba(0,0,0,0.5)").fillRect(boxX, boxY, boxWidth, boxHeight);
      this.withStrokeStyle("red").withLineWidth(4).strokeRect(boxX, boxY, boxWidth, boxHeight);
      this.withFillStyle("red").renderGlyph("Guru Meditation", boxX + ((textWidth - 15) / 2) * this.layout.glyphWidth, boxY + this.layout.rowHeight);
      this.renderGlyph(err.message.substring(0, textWidth), boxX + this.layout.glyphWidth, boxY + this.layout.rowHeight * 3);
      stack.forEach((line, i) =>
        this.renderGlyph(("  at " + line).substring(0, textWidth), boxX, boxY + (i + 4) * this.layout.rowHeight));
    } else {
      this.withFillStyle("red").renderGlyph(err as string, 0, this.layout.rowHeight);
    }
  }
}
