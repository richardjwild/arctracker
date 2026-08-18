import { CurrentPattern } from "../transport/transport.ts";
import { EditorState } from "../editing/editor.ts";
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
): PatternContentDimensions {
  const layout = patternLayout.getPatternLayout(viewportSize);
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
  viewportSize: ViewportSize;
  numTracks: number;
  linesPerBeat: number;
  trackMuteState: boolean[];
  trackPanning: number[];
  editorState: EditorState;
  effectsDisplayed: number[];
  patternSelection: PatternSelection | null;
};

type RenderPatternContext = {
  view: RenderPatternView;
  layout: PatternLayout;
  gridViewportFit: GridViewportFit;
}

export class PatternRenderer {
  private ctx: CanvasRenderingContext2D;
  private readonly coloursAtPlayhead: Colours;
  private readonly coloursOffPlayhead: Colours;
  private readonly trackHeaderFont: string;
  private readonly patternDataFont: string;

  public constructor(
    ctx: CanvasRenderingContext2D,
    coloursAtPlayhead: Colours,
    coloursOffPlayhead: Colours,
    trackHeaderFont: string,
    patternDataFont: string,
  ) {
    this.ctx = ctx;
    this.coloursAtPlayhead = coloursAtPlayhead;
    this.coloursOffPlayhead = coloursOffPlayhead;
    this.trackHeaderFont = trackHeaderFont;
    this.patternDataFont = patternDataFont;
  }

  public renderPattern(view: RenderPatternView) {
    const context: RenderPatternContext = {
      view,
      layout: patternLayout.getPatternLayout(view.viewportSize),
      gridViewportFit: patternLayout.calculateGridViewportFit(view.viewportSize, view.numTracks),
    };
    try {
      this.ctx.textBaseline = "hanging";
      this.ctx.clearRect(0, 0, view.viewportSize.width, view.viewportSize.height);
      if (view.linesPerBeat > 0) this.renderBeatLines(context);
      this.renderTrackLanes(context);
      this.renderTrackHeaders(context);
      this.renderTrackFooters(context);
      this.renderPlayhead(context);
      if (view.patternSelection) this.renderSelection(context);
      this.renderCursor(context, patternEvents.editing());
      this.renderPatternLines(context);
    } catch (err) {
      this.renderError(context, err);
    }
  }

  private renderBeatLines(context: RenderPatternContext) {
    let y = context.layout.trackHeaderHeight;
    let width = context.layout.leftPadding + context.layout.rowNumberWidth - context.layout.glyphWidth;
    for (let track = context.gridViewportFit.firstVisibleTrack; track <= context.gridViewportFit.lastVisibleTrack; track++) {
      width += context.layout.getEventWidth(track);
    }
    for (let screenLine = 0; screenLine < context.gridViewportFit.linesToShow; screenLine++) {
      const patternIndex = context.view.playheadIndex - context.gridViewportFit.playheadLocationOnScreen + screenLine;
      const atPlayhead = (patternIndex === context.view.playheadIndex);
      if (patternIndex >= 0 && patternIndex < context.view.pattern.lines.length && patternIndex % context.view.linesPerBeat === 0) {
        const lineY = atPlayhead ? y + context.layout.playheadPadding : y;
        this.withFillStyle(this.colours().beatLine)
          .fillRect(0, lineY, width, context.layout.rowHeight);
      }
      y += atPlayhead
        ? context.layout.rowHeight + 2 * context.layout.playheadPadding
        : context.layout.rowHeight;
    }
  }

  private renderTrackLanes(context: RenderPatternContext) {
    let x = this.renderRowNumberLane(context);
    for (let track = 0; track <= context.view.numTracks; track++) {
      x += this.renderTrackLane(context, x, track);
    }
  }

  private renderTrackHeaders(context: RenderPatternContext) {
    this.ctx.font = this.trackHeaderFont;
    let x = context.layout.leftPadding + context.layout.rowNumberWidth - context.layout.glyphWidth;
    for (let track = 0; track <= context.view.numTracks; track++) {
      x += this.renderTrackHeader(context, x, track, context.view.trackMuteState[track]);
    }
  }

  private renderTrackFooters(context: RenderPatternContext) {
    this.ctx.font = this.trackHeaderFont;
    let x = context.layout.leftPadding + context.layout.rowNumberWidth - context.layout.glyphWidth;
    for (let track = 0; track <= context.view.numTracks; track++) {
      x += this.renderTrackFooter(context, x, track, context.view.trackMuteState[track]);
    }
  }

  private renderPlayhead(context: RenderPatternContext) {
    const y = context.layout.trackHeaderHeight
      + context.gridViewportFit.playheadLocationOnScreen * context.layout.rowHeight;
    this.withFillStyle(this.colours().playheadBackground)
      .fillRect(0, y + context.layout.playheadPadding, context.view.viewportSize.width, context.layout.rowHeight);
  }

  private renderSelection(context: RenderPatternContext) {
    const bounds = selection.patternSelectionBounds();
    if (!bounds) return;
    let boxLeft = context.layout.rowNumberWidth;
    for (let track = context.gridViewportFit.firstVisibleTrack; track < bounds.left; track++)
      boxLeft += context.layout.getEventWidth(track);
    let boxWidth = 0;
    for (let track = Math.max(bounds.left, context.gridViewportFit.firstVisibleTrack); track <= bounds.right; track++)
      boxWidth += context.layout.getEventWidth(track);
    const rowOffsetFromPlayhead = bounds.top - context.view.playheadIndex;
    const top = (context.gridViewportFit.playheadLocationOnScreen + rowOffsetFromPlayhead)
      * context.layout.rowHeight
      + context.layout.trackHeaderHeight
      + context.layout.playheadPadding;
    const boxHeight = (bounds.bottom - bounds.top + 1) * context.layout.rowHeight;
    this.withFillStyle(this.colours().selectionBox).fillRect(boxLeft, top, boxWidth, boxHeight);
    this.withStrokeStyle(this.colours().selectionBoxOutline).strokeRect(boxLeft, top, boxWidth, boxHeight);
  }

  private renderCursor(context: RenderPatternContext, editMode: boolean) {
    const cursorTrack = context.view.editorState.cursorPosition.track;
    let x = context.layout.leftPadding + context.layout.rowNumberWidth;
    for (let track = context.gridViewportFit.firstVisibleTrack; track < cursorTrack; track++) {
      x += context.layout.getEventWidth(track);
    }
    const y = context.layout.trackHeaderHeight
      + (context.gridViewportFit.playheadLocationOnScreen * context.layout.rowHeight)
      + context.layout.playheadPadding;
    if (editMode) this.renderEditCursor(context, x, y, cursorTrack);
    else this.renderNonEditCursor(context, x, y, cursorTrack);
  }

  private renderNonEditCursor(context: RenderPatternContext, x: number, y: number, cursorTrack: number) {
    this.withStrokeStyle(this.colours().playheadBackground).withLineWidth(2).strokeRect(
      x - 1,
      y - 1,
      2 + context.layout.getEventWidth(cursorTrack) - (context.layout.glyphWidth * 2),
      2 + context.layout.rowHeight
    );
  }

  private renderEditCursor(context: RenderPatternContext, x: number, y: number, cursorTrack: number) {
    this.withStrokeStyle(this.colours().cursor).withLineWidth(2).strokeRect(
      x - 1,
      y - 1,
      2 + context.layout.getEventWidth(cursorTrack) - (context.layout.glyphWidth * 2),
      2 + context.layout.rowHeight
    );
    let cursorX = context.layout.leftPadding + x - context.layout.glyphWidth;
    let cursorWidth = context.layout.glyphWidth;
    const cursor = new Cursor();
    const cursorField = cursor.currentField();
    if (cursorField.field === "note") {
      cursorWidth = context.layout.glyphWidth * 3;
    } else if (cursorField.field === "sampleHigh") {
      cursorX += context.layout.glyphWidth * 4;
    } else if (cursorField.field === "sampleLow") {
      cursorX += context.layout.glyphWidth * 5;
    } else if (cursorField.field === "effectCode") {
      cursorX += context.layout.glyphWidth * (7 + (cursorField.effectIndex * 4));
    } else if (cursorField.field === "effectData1") {
      cursorX += context.layout.glyphWidth * (8 + (cursorField.effectIndex * 4));
    } else if (cursorField.field === "effectData2") {
      cursorX += context.layout.glyphWidth * (9 + (cursorField.effectIndex * 4));
    }
    this.withFillStyle(this.colours().cursor)
      .fillRect(cursorX, y, cursorWidth, context.layout.rowHeight);
  }

  private renderPatternLines(context: RenderPatternContext) {
    this.ctx.font = this.patternDataFont;
    let y = context.layout.trackHeaderHeight;
    for (let screenLine = 0; screenLine < context.gridViewportFit.linesToShow; screenLine++) {
      const patternIndex = context.view.playheadIndex - context.gridViewportFit.playheadLocationOnScreen + screenLine;
      const patternLine =
        patternIndex >= 0 && patternIndex < context.view.pattern.lines.length
          ? context.view.pattern.lines[patternIndex]
          : null;
      const atPlayhead = (patternIndex === context.view.playheadIndex);
      y += this.renderPatternLine(context, patternLine, y, atPlayhead);
    }
  }

  private renderRowNumberLane(context: RenderPatternContext): number {
    const laneWidth = context.layout.leftPadding + context.layout.rowNumberWidth - context.layout.glyphWidth;
    this.withStrokeStyle(this.colours().trackLaneSeparator).withLineWidth(1)
      .renderLine(laneWidth, 0, laneWidth, context.view.viewportSize.height);
    return laneWidth;
  }

  private renderTrackLane(context: RenderPatternContext, trackX: number, track: number): number {
    const trackWidth = context.layout.getEventWidth(track);
    if (track < context.gridViewportFit.firstVisibleTrack || track > context.gridViewportFit.lastVisibleTrack)
      return 0;
    this.withStrokeStyle(this.colours().trackLaneSeparator).withLineWidth(1)
      .renderLine(trackX + trackWidth, 0, trackX + trackWidth, context.view.viewportSize.height);
    return trackWidth;
  }

  private renderTrackHeader(context: RenderPatternContext, trackX: number, track: number, muted: boolean): number {
    const fgColour = muted ? this.colours().trackHeaderMutedFg : this.colours().trackHeaderNotMutedFg;
    const bgColour = muted ? this.colours().trackHeaderMutedBg : this.colours().trackHeaderNotMutedBg;
    const trackWidth = context.layout.getEventWidth(track);
    if (track < context.gridViewportFit.firstVisibleTrack || track > context.gridViewportFit.lastVisibleTrack)
      return 0;
    this.withFillStyle(bgColour)
      .fillRect(trackX + 1, 0, trackWidth - 2, context.layout.trackHeaderHeight);
    this.withFillStyle(fgColour).renderGlyph(
      (track + 1).toString(),
      trackX + context.layout.glyphWidth / 2,
      2,
    );
    if (muted) this.renderMutedIcon(trackX + trackWidth - 14, 3);
    else this.renderNotMutedIcon(trackX + trackWidth - 14, 3);
    return trackWidth;
  }

  private renderTrackFooter(context: RenderPatternContext, trackX: number, track: number, muted: boolean): number {
    const fgColour = muted ? this.colours().trackFooterMutedFg : this.colours().trackFooterNotMutedFg;
    const bgColour = muted ? this.colours().trackFooterMutedBg : this.colours().trackFooterNotMutedBg;
    const trackWidth = context.layout.getEventWidth(track);
    if (track < context.gridViewportFit.firstVisibleTrack || track > context.gridViewportFit.lastVisibleTrack)
      return 0;
    this.withFillStyle(bgColour)
      .fillRect(trackX + 1, context.layout.viewportSize.height - context.layout.trackFooterHeight, trackWidth - 2, context.layout.trackFooterHeight);
    this.withFillStyle(fgColour).renderGlyph(
      "L",
      trackX + context.layout.glyphWidth,
      context.layout.viewportSize.height - context.layout.trackFooterHeight + 2,
    );
    this.withFillStyle(fgColour).renderGlyph(
      "R",
      trackX + trackWidth - context.layout.glyphWidth * 2 + 1,
      context.layout.viewportSize.height - context.layout.trackFooterHeight + 2,
    );
    const centreX = trackX - 1 + trackWidth / 2;
    const sliderThrow = (trackWidth - context.layout.glyphWidth * 5) / 2;
    this.ctx.lineWidth = 1;
    this.withStrokeStyle(fgColour);
    this.ctx.beginPath();
    this.ctx.moveTo(centreX - sliderThrow, context.layout.viewportSize.height - context.layout.trackFooterHeight / 2);
    this.ctx.lineTo(centreX + sliderThrow, context.layout.viewportSize.height - context.layout.trackFooterHeight / 2);
    this.ctx.closePath();
    this.ctx.stroke();
    this.ctx.beginPath();
    this.ctx.moveTo(centreX, context.layout.viewportSize.height - context.layout.trackFooterHeight + 4);
    this.ctx.lineTo(centreX, context.layout.viewportSize.height - 4);
    this.ctx.closePath();
    this.ctx.stroke();
    const trackPanning =
      track >= 0 && track < context.view.trackPanning.length
        ? context.view.trackPanning[track] - 128
        : 0;
    const deflection = sliderThrow * trackPanning / 127;
    this.ctx.beginPath();
    this.ctx.arc(centreX + deflection, context.layout.viewportSize.height - context.layout.trackFooterHeight / 2, 5, 0, FULL_CIRCLE);
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

  private renderPatternLine(context: RenderPatternContext, line: PatternLine | null, y: number, atPlayhead: boolean): number {
    if (line) {
      const rowNumber = Number(line.row).toString().padStart(3, " ");
      const eventY = atPlayhead ? y + context.layout.playheadPadding : y;
      let x = context.layout.leftPadding;
      x += this.renderRowNumber(context, rowNumber, x, eventY, atPlayhead);
      let track = 0;
      for (const event of line.events) {
        x += this.renderEvent(context, track, event, x, eventY, atPlayhead);
        track++;
      }
    }
    return atPlayhead
      ? context.layout.rowHeight + 2 * context.layout.playheadPadding
      : context.layout.rowHeight;
  }

  private renderRowNumber(context: RenderPatternContext, rowNumber: string, x: number, y: number, atPlayhead: boolean): number {
    this.withFillStyle(this.colours(atPlayhead).text)
      .renderGlyph(rowNumber.charAt(0), x, y)
      .renderGlyph(rowNumber.charAt(1), x + context.layout.glyphWidth, y)
      .renderGlyph(rowNumber.charAt(2), x + context.layout.glyphWidth * 2, y);
    return context.layout.rowNumberWidth;
  }

  private renderEvent(context: RenderPatternContext, track: number, event: PatternEvent, x: number, y: number, atPlayhead: boolean): number {
    if (track < context.gridViewportFit.firstVisibleTrack || track > context.gridViewportFit.lastVisibleTrack)
      return 0;
    const cursorOnEvent = (atPlayhead && patternEvents.editing() && track === context.view.editorState.cursorPosition.track);
    const muted = context.view.trackMuteState[track];
    x += this.renderNote(context, x, y, event.note, atPlayhead, cursorOnEvent, muted);
    x += this.renderSample(context, x, y, event.sampleNo, atPlayhead, cursorOnEvent, muted);
    for (let effectIndex = 0; effectIndex < context.view.effectsDisplayed[track]; effectIndex++) {
      x += this.renderEffect(context, x, y, effectIndex, event.effects[effectIndex], atPlayhead, cursorOnEvent, muted);
    }
    return context.layout.getEventWidth(track);
  }

  private renderNote(context: RenderPatternContext, x: number, y: number, note: number, atPlayhead: boolean, cursorOnEvent: boolean, muted: boolean): number {
    const noteStr = notes.toString(note);
    let colour;
    if (muted)
      colour = this.colours().channelMuted;
    else if (cursorOnEvent && context.view.editorState.cursorPosition.field === NOTE_FIELD)
      colour = this.colours().cursorText;
    else if (note === 0)
      colour = this.colours(atPlayhead).text;
    else
      colour = this.colours(atPlayhead).note;
    if (noteStr === "–––") {
      this.withFillStyle(colour).renderGlyph(noteStr, x, y)
      return context.layout.glyphWidth * 4;
    }
    if (noteStr.length === 2)
      this.withFillStyle(colour)
        .renderGlyph(noteStr.charAt(0), x, y)
        .renderGlyph(noteStr.charAt(1), x + context.layout.glyphWidth, y);
    else if (noteStr.length === 3)
      this.withFillStyle(colour)
        .renderGlyph(noteStr.charAt(0), x, y)
        .renderGlyph(noteStr.charAt(1), x - 2 + context.layout.glyphWidth, y)
        .renderGlyph(noteStr.charAt(2), x - 4 + context.layout.glyphWidth * 2, y);
    return context.layout.glyphWidth * 4;
  }

  private renderSample(context: RenderPatternContext, x: number, y: number, sampleNo: number, atPlayhead: boolean, cursorOnEvent: boolean, muted: boolean) {
    if (sampleNo === 0) {
      x += this.renderSampleDigit(context, x, y, DASH, SAMPLE_HIGH_FIELD, atPlayhead, cursorOnEvent, muted);
      this.renderSampleDigit(context, x, y, DASH, SAMPLE_LOW_FIELD, atPlayhead, cursorOnEvent, muted);
    } else {
      const sampleNumber = hexadecimal.toHex(sampleNo, 2);
      x += this.renderSampleDigit(context, x, y, sampleNumber.charAt(0), SAMPLE_HIGH_FIELD, atPlayhead, cursorOnEvent, muted);
      this.renderSampleDigit(context, x, y, sampleNumber.charAt(1), SAMPLE_LOW_FIELD, atPlayhead, cursorOnEvent, muted);
    }
    return context.layout.glyphWidth * 3;
  }

  private renderSampleDigit(context: RenderPatternContext, x: number, y: number, digit: string, field: number, atPlayhead: boolean, cursorOnEvent: boolean, muted: boolean): number {
    let colour;
    if (muted)
      colour = this.colours().channelMuted;
    else if (cursorOnEvent && context.view.editorState.cursorPosition.field === field) {
      colour = this.colours().cursorText;
    } else {
      colour = (digit === DASH) ? this.colours(atPlayhead).text : this.colours(atPlayhead).sample;
    }
    this.withFillStyle(colour).renderGlyph(digit, x, y);
    return context.layout.glyphWidth;
  }

  private renderEffect(context: RenderPatternContext, x: number, y: number, effectIndex: number, effect: Effect, atPlayhead: boolean, cursorOnEvent: boolean, muted: boolean): number {
    if (effect.effectCode === "" && effect.effectData[0] === 0 && effect.effectData[1] === 0) {
      x += this.renderEffectField(context, x, y, effectIndex, DASH, atPlayhead, cursorOnEvent, 0, muted);
      x += this.renderEffectField(context, x, y, effectIndex, DASH, atPlayhead, cursorOnEvent, 1, muted);
      this.renderEffectField(context, x, y, effectIndex, DASH, atPlayhead, cursorOnEvent, 2, muted);
    } else {
      x += this.renderEffectField(context, x, y, effectIndex, effect.effectCode, atPlayhead, cursorOnEvent, 0, muted);
      x += this.renderEffectField(context, x, y, effectIndex, hexadecimal.toHex(effect.effectData[0]), atPlayhead, cursorOnEvent, 1, muted);
      this.renderEffectField(context, x, y, effectIndex, hexadecimal.toHex(effect.effectData[1]), atPlayhead, cursorOnEvent, 2, muted);
    }
    return context.layout.glyphWidth * (FIELDS_PER_EFFECT + 1);
  }

  private renderEffectField(context: RenderPatternContext, x: number, y: number, effectIndex: number, effectParam: string, atPlayhead: boolean, cursorOnEvent: boolean, effectField: number, muted: boolean): number {
    const cursorField = FIRST_EFFECT_FIELD + (effectIndex * 3) + effectField;
    let colour;
    if (muted)
      colour = this.colours().channelMuted;
    else if (cursorOnEvent && context.view.editorState.cursorPosition.field === cursorField)
      colour = this.colours().cursorText;
    else
      colour = (effectParam === DASH) ? this.colours(atPlayhead).text : this.colours(atPlayhead).effect;
    this.withFillStyle(colour).renderGlyph(effectParam, x, y)
    return context.layout.glyphWidth;
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

  private renderError(context: RenderPatternContext, err: any) {
    const cols = context.view.viewportSize.width / context.layout.glyphWidth;
    const rows = context.view.viewportSize.height / context.layout.rowHeight;
    if (err instanceof Error) {
      const stack = (err.stack || "").split("\n");
      const longestLine = [...stack, err.message].reduce((a, b) => a.length > b.length ? a : b);
      const textWidth = Math.min(longestLine.length, cols - 2);
      const textHeight = Math.min(stack.length + 3, rows - 2);
      const boxWidth = (textWidth) * context.layout.glyphWidth;
      const boxHeight = (textHeight + 2) * context.layout.rowHeight;
      const boxX = (context.view.viewportSize.width - boxWidth) / 2;
      const boxY = (context.view.viewportSize.height - boxHeight) / 2;
      this.withFillStyle("rgba(0,0,0,0.5)").fillRect(boxX, boxY, boxWidth, boxHeight);
      this.withStrokeStyle("red").withLineWidth(4).strokeRect(boxX, boxY, boxWidth, boxHeight);
      this.withFillStyle("red").renderGlyph("Guru Meditation", boxX + ((textWidth - 15) / 2) * context.layout.glyphWidth, boxY + context.layout.rowHeight);
      this.renderGlyph(err.message.substring(0, textWidth), boxX + context.layout.glyphWidth, boxY + context.layout.rowHeight * 3);
      stack.forEach((line, i) =>
        this.renderGlyph(("  at " + line).substring(0, textWidth), boxX, boxY + (i + 4) * context.layout.rowHeight));
    } else {
      this.withFillStyle("red").renderGlyph(err as string, 0, context.layout.rowHeight);
    }
  }
}
