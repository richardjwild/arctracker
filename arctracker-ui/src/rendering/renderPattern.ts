import { CurrentPattern } from "../transport/transport.ts";
import { useStore } from "../store/useStore.ts";
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

function cssProperty(name: string): string {
  return getComputedStyle(document.documentElement)
    .getPropertyValue(name)
    .trim();
}

type Colours = {
  background: string;
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

export class PatternRenderer {
  private readonly pattern: CurrentPattern;
  private ctx: CanvasRenderingContext2D;
  private viewportSize: ViewportSize;
  private readonly editorState: EditorState;
  private readonly effectsDisplayed: number[];
  private readonly patternSelection: PatternSelection | null;
  private patternLayout: PatternLayout;
  private readonly numTracks: number;
  private readonly trackMuted: boolean[];
  private readonly trackPanning: number[];
  private readonly coloursAtPlayhead: Colours;
  private readonly coloursOffPlayhead: Colours;
  private readonly fontTrackHeader: string;
  private readonly fontPatternData: string;
  private readonly gridViewportFit: GridViewportFit;

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
    this.trackMuted = useStore.getState().trackMuteState;
    this.trackPanning = useStore.getState().trackPanning;
    this.editorState = useStore.getState().editorState;
    this.effectsDisplayed = useStore.getState().effectsDisplayed;
    this.patternSelection = useStore.getState().patternSelection;
    this.patternLayout = patternLayout.getPatternLayout(viewportSize);
    this.coloursAtPlayhead = {
      background: cssProperty("--colour-panel-bg"),
      trackLaneSeparator: cssProperty("--colour-track-lane-separator"),
      trackHeaderMutedFg: cssProperty("--colour-track-header-muted-fg"),
      trackHeaderMutedBg: cssProperty("--colour-track-header-muted-bg"),
      trackHeaderNotMutedFg: cssProperty("--colour-track-header-not-muted-fg"),
      trackHeaderNotMutedBg: cssProperty("--colour-track-header-not-muted-bg"),
      trackFooterMutedFg: cssProperty("--colour-track-footer-muted-fg"),
      trackFooterMutedBg: cssProperty("--colour-track-footer-muted-bg"),
      trackFooterNotMutedFg: cssProperty("--colour-track-footer-not-muted-fg"),
      trackFooterNotMutedBg: cssProperty("--colour-track-footer-not-muted-bg"),
      playheadBackground: cssProperty("--colour-playhead"),
      text: cssProperty("--colour-pattern-text-bright"),
      channelMuted: cssProperty("--colour-pattern-text-channel-muted"),
      cursor: cssProperty("--colour-cursor"),
      cursorText: cssProperty("--colour-cursor-text"),
      note: cssProperty("--colour-note-at-playhead"),
      sample: cssProperty("--colour-sample-at-playhead"),
      effect: cssProperty("--colour-effect-at-playhead"),
      selectionBox: cssProperty("--colour-selection-fill"),
      selectionBoxOutline: cssProperty("--colour-selection-outline"),
    };
    this.coloursOffPlayhead = {
      background: cssProperty("--colour-panel-bg"),
      trackLaneSeparator: cssProperty("--colour-track-lane-separator"),
      trackHeaderMutedFg: cssProperty("--colour-track-header-muted-fg"),
      trackHeaderMutedBg: cssProperty("--colour-track-header-muted-bg"),
      trackHeaderNotMutedFg: cssProperty("--colour-track-header-not-muted-fg"),
      trackHeaderNotMutedBg: cssProperty("--colour-track-header-not-muted-bg"),
      trackFooterMutedFg: cssProperty("--colour-track-footer-muted-fg"),
      trackFooterMutedBg: cssProperty("--colour-track-footer-muted-bg"),
      trackFooterNotMutedFg: cssProperty("--colour-track-footer-not-muted-fg"),
      trackFooterNotMutedBg: cssProperty("--colour-track-footer-not-muted-bg"),
      playheadBackground: cssProperty("--colour-playhead"),
      text: cssProperty("--colour-pattern-text-muted"),
      channelMuted: cssProperty("--colour-pattern-text-channel-muted"),
      cursor: cssProperty("--colour-cursor"),
      cursorText: cssProperty("--colour-cursor-text"),
      note: cssProperty("--colour-note"),
      sample: cssProperty("--colour-sample"),
      effect: cssProperty("--colour-effect"),
      selectionBox: cssProperty("--colour-selection-fill"),
      selectionBoxOutline: cssProperty("--colour-selection-outline"),
    };
    this.fontTrackHeader = cssProperty("--font-track-header");
    this.fontPatternData = cssProperty("--font-pattern-data");
    this.gridViewportFit = patternLayout.calculateGridViewportFit(viewportSize, numTracks);
  }

  public renderPattern(playheadIndex: number) {
    try {
      this.ctx.textBaseline = "hanging";
      this.ctx.clearRect(0, 0, this.viewportSize.width, this.viewportSize.height);
      this.renderTrackLanes();
      this.renderTrackHeaders();
      this.renderTrackFooters();
      this.renderPlayhead();
      if (this.patternSelection) this.renderSelection(playheadIndex);
      this.renderCursor(patternEvents.editing());
      this.renderPatternLines(playheadIndex);
    } catch (err) {
      console.error(err);
    }
  }

  private renderTrackLanes() {
    let x = this.renderRowNumberLane();
    for (let track = 0; track <= this.numTracks; track++) {
      x += this.renderTrackLane(x, track);
    }
  }

  private renderTrackHeaders() {
    this.ctx.font = this.fontTrackHeader;
    let x = this.patternLayout.leftPadding + this.patternLayout.rowNumberWidth - this.patternLayout.glyphWidth;
    for (let track = 0; track <= this.numTracks; track++) {
      x += this.renderTrackHeader(x, track, this.trackMuted[track]);
    }
  }

  private renderTrackFooters() {
    this.ctx.font = this.fontTrackHeader;
    let x = this.patternLayout.leftPadding + this.patternLayout.rowNumberWidth - this.patternLayout.glyphWidth;
    for (let track = 0; track <= this.numTracks; track++) {
      x += this.renderTrackFooter(x, track, this.trackMuted[track]);
    }
  }

  private renderPlayhead() {
    const y = this.patternLayout.trackHeaderHeight
      + this.gridViewportFit.playheadLocationOnScreen * this.patternLayout.rowHeight;
    this.withFillStyle(this.colours().playheadBackground)
      .fillRect(0, y + this.patternLayout.playheadPadding, this.viewportSize.width, this.patternLayout.rowHeight);
  }

  private renderSelection(playheadIndex: number) {
    const bounds = selection.patternSelectionBounds();
    if (!bounds) return;
    let boxLeft = this.patternLayout.rowNumberWidth;
    for (let track = this.gridViewportFit.firstVisibleTrack; track < bounds.left; track++)
      boxLeft += this.patternLayout.getEventWidth(track);
    let boxWidth = 0;
    for (let track = Math.max(bounds.left, this.gridViewportFit.firstVisibleTrack); track <= bounds.right; track++)
      boxWidth += this.patternLayout.getEventWidth(track);
    const rowOffsetFromPlayhead = bounds.top - playheadIndex;
    const top = (this.gridViewportFit.playheadLocationOnScreen + rowOffsetFromPlayhead)
      * this.patternLayout.rowHeight
      + this.patternLayout.trackHeaderHeight
      + this.patternLayout.playheadPadding;
    const boxHeight = (bounds.bottom - bounds.top + 1) * this.patternLayout.rowHeight;
    this.withFillStyle(this.colours().selectionBox).fillRect(boxLeft, top, boxWidth, boxHeight);
    this.withStrokeStyle(this.colours().selectionBoxOutline).strokeRect(boxLeft, top, boxWidth, boxHeight);
  }

  private renderCursor(editMode: boolean) {
    const cursorTrack = this.editorState.cursorPosition.track;
    let x = this.patternLayout.leftPadding + this.patternLayout.rowNumberWidth;
    for (let track = this.gridViewportFit.firstVisibleTrack; track < cursorTrack; track++) {
      x += this.patternLayout.getEventWidth(track);
    }
    const y = this.patternLayout.trackHeaderHeight
      + (this.gridViewportFit.playheadLocationOnScreen * this.patternLayout.rowHeight)
      + this.patternLayout.playheadPadding;
    if (editMode) this.renderEditCursor(x, y, cursorTrack);
    else this.renderNonEditCursor(x, y, cursorTrack);
  }

  private renderNonEditCursor(x: number, y: number, cursorTrack: number) {
    this.withStrokeStyle(this.colours().playheadBackground).withLineWidth(2).strokeRect(
      x - 1,
      y - 1,
      2 + this.patternLayout.getEventWidth(cursorTrack) - (this.patternLayout.glyphWidth * 2),
      2 + this.patternLayout.rowHeight
    );
  }

  private renderEditCursor(x: number, y: number, cursorTrack: number) {
    this.withStrokeStyle(this.colours().cursor).withLineWidth(2).strokeRect(
      x - 1,
      y - 1,
      2 + this.patternLayout.getEventWidth(cursorTrack) - (this.patternLayout.glyphWidth * 2),
      2 + this.patternLayout.rowHeight
    );
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

  private renderPatternLines(playheadIndex: number) {
    this.ctx.font = this.fontPatternData;
    let y = this.patternLayout.trackHeaderHeight;
    for (let screenLine = 0; screenLine < this.gridViewportFit.linesToShow; screenLine++) {
      const patternIndex = playheadIndex - this.gridViewportFit.playheadLocationOnScreen + screenLine;
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

  private renderTrackLane(trackX: number, track: number): number {
    const trackWidth = this.patternLayout.getEventWidth(track);
    if (track < this.gridViewportFit.firstVisibleTrack || track > this.gridViewportFit.lastVisibleTrack)
      return 0;
    this.withStrokeStyle(this.colours().trackLaneSeparator)
      .renderLine(trackX + trackWidth, 0, trackX + trackWidth, this.viewportSize.height);
    return trackWidth;
  }

  private renderTrackHeader(trackX: number, track: number, muted: boolean): number {
    const fgColour = muted ? this.colours().trackHeaderMutedFg : this.colours().trackHeaderNotMutedFg;
    const bgColour = muted ? this.colours().trackHeaderMutedBg : this.colours().trackHeaderNotMutedBg;
    const trackWidth = this.patternLayout.getEventWidth(track);
    if (track < this.gridViewportFit.firstVisibleTrack || track > this.gridViewportFit.lastVisibleTrack)
      return 0;
    this.withFillStyle(bgColour)
      .fillRect(trackX + 1, 0, trackWidth - 2, this.patternLayout.trackHeaderHeight);
    this.withFillStyle(fgColour).renderGlyph(
      (track + 1).toString(),
      trackX + this.patternLayout.glyphWidth / 2,
      2,
    );
    if (muted) this.renderMutedIcon(trackX + trackWidth - 14, 3);
    else this.renderNotMutedIcon(trackX + trackWidth - 14, 3);
    return trackWidth;
  }

  private renderTrackFooter(trackX: number, track: number, muted: boolean): number {
    const fgColour = muted ? this.colours().trackFooterMutedFg : this.colours().trackFooterNotMutedFg;
    const bgColour = muted ? this.colours().trackFooterMutedBg : this.colours().trackFooterNotMutedBg;
    const trackWidth = this.patternLayout.getEventWidth(track);
    if (track < this.gridViewportFit.firstVisibleTrack || track > this.gridViewportFit.lastVisibleTrack)
      return 0;
    this.withFillStyle(bgColour)
      .fillRect(trackX + 1, this.patternLayout.viewportSize.height - this.patternLayout.trackFooterHeight, trackWidth - 2, this.patternLayout.trackFooterHeight);
    this.withFillStyle(fgColour).renderGlyph(
      "L",
      trackX + this.patternLayout.glyphWidth,
      this.patternLayout.viewportSize.height - this.patternLayout.trackFooterHeight + 2,
    );
    this.withFillStyle(fgColour).renderGlyph(
      "R",
      trackX + trackWidth - this.patternLayout.glyphWidth * 2 + 1,
      this.patternLayout.viewportSize.height - this.patternLayout.trackFooterHeight + 2,
    );
    const centreX = trackX - 1 + trackWidth / 2;
    const sliderThrow = (trackWidth - this.patternLayout.glyphWidth * 5) / 2;
    this.ctx.lineWidth = 1;
    this.withStrokeStyle(fgColour);
    this.ctx.beginPath();
    this.ctx.moveTo(centreX - sliderThrow, this.patternLayout.viewportSize.height - this.patternLayout.trackFooterHeight / 2);
    this.ctx.lineTo(centreX + sliderThrow, this.patternLayout.viewportSize.height - this.patternLayout.trackFooterHeight / 2);
    this.ctx.closePath();
    this.ctx.stroke();
    this.ctx.beginPath();
    this.ctx.moveTo(centreX, this.patternLayout.viewportSize.height - this.patternLayout.trackFooterHeight + 4);
    this.ctx.lineTo(centreX, this.patternLayout.viewportSize.height - 4);
    this.ctx.closePath();
    this.ctx.stroke();
    const trackPanning =
      track >= 0 && track < this.trackPanning.length
        ? this.trackPanning[track] - 128
        : 0;
    const deflection = sliderThrow * trackPanning / 127;
    this.ctx.beginPath();
    this.ctx.arc(centreX + deflection, this.patternLayout.viewportSize.height - this.patternLayout.trackFooterHeight / 2, 5, 0, FULL_CIRCLE);
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
    if (track < this.gridViewportFit.firstVisibleTrack || track > this.gridViewportFit.lastVisibleTrack)
      return 0;
    const cursorOnEvent = (atPlayhead && patternEvents.editing() && track === this.editorState.cursorPosition.track);
    const muted = this.trackMuted[track];
    x += this.renderNote(x, y, event.note, atPlayhead, cursorOnEvent, muted);
    x += this.renderSample(x, y, event.sampleNo, atPlayhead, cursorOnEvent, muted);
    for (let effectIndex = 0; effectIndex < this.effectsDisplayed[track]; effectIndex++) {
      x += this.renderEffect(x, y, effectIndex, event.effects[effectIndex], atPlayhead, cursorOnEvent, muted);
    }
    return this.patternLayout.getEventWidth(track);
  }

  private renderNote(x: number, y: number, note: number, atPlayhead: boolean, cursorOnEvent: boolean, muted: boolean): number {
    const noteStr = notes.toString(note);
    let colour;
    if (muted)
      colour = this.colours().channelMuted;
    else if (cursorOnEvent && this.editorState.cursorPosition.field === NOTE_FIELD)
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

  private renderSample(x: number, y: number, sampleNo: number, atPlayhead: boolean, cursorOnEvent: boolean, muted: boolean) {
    if (sampleNo === 0) {
      x += this.renderSampleDigit(x, y, DASH, SAMPLE_HIGH_FIELD, atPlayhead, cursorOnEvent, muted);
      this.renderSampleDigit(x, y, DASH, SAMPLE_LOW_FIELD, atPlayhead, cursorOnEvent, muted);
    } else {
      const sampleNumber = hexadecimal.toHex(sampleNo, 2);
      x += this.renderSampleDigit(x, y, sampleNumber.charAt(0), SAMPLE_HIGH_FIELD, atPlayhead, cursorOnEvent, muted);
      this.renderSampleDigit(x, y, sampleNumber.charAt(1), SAMPLE_LOW_FIELD, atPlayhead, cursorOnEvent, muted);
    }
    return this.patternLayout.glyphWidth * 3;
  }

  private renderSampleDigit(x: number, y: number, digit: string, field: number, atPlayhead: boolean, cursorOnEvent: boolean, muted: boolean): number {
    let colour;
    if (muted)
      colour = this.colours().channelMuted;
    else if (cursorOnEvent && this.editorState.cursorPosition.field === field) {
      colour = this.colours().cursorText;
    } else {
      colour = (digit === DASH) ? this.colours(atPlayhead).text : this.colours(atPlayhead).sample;
    }
    this.withFillStyle(colour).renderGlyph(digit, x, y);
    return this.patternLayout.glyphWidth;
  }

  private renderEffect(x: number, y: number, effectIndex: number, effect: Effect, atPlayhead: boolean, cursorOnEvent: boolean, muted: boolean): number {
    if (effect.effectCode === "" && effect.effectData[0] === 0 && effect.effectData[1] === 0) {
      x += this.renderEffectField(x, y, effectIndex, DASH, atPlayhead, cursorOnEvent, 0, muted);
      x += this.renderEffectField(x, y, effectIndex, DASH, atPlayhead, cursorOnEvent, 1, muted);
      this.renderEffectField(x, y, effectIndex, DASH, atPlayhead, cursorOnEvent, 2, muted);
    } else {
      x += this.renderEffectField(x, y, effectIndex, effect.effectCode, atPlayhead, cursorOnEvent, 0, muted);
      x += this.renderEffectField(x, y, effectIndex, hexadecimal.toHex(effect.effectData[0]), atPlayhead, cursorOnEvent, 1, muted);
      this.renderEffectField(x, y, effectIndex, hexadecimal.toHex(effect.effectData[1]), atPlayhead, cursorOnEvent, 2, muted);
    }
    return this.patternLayout.glyphWidth * (FIELDS_PER_EFFECT + 1);
  }

  private renderEffectField(x: number, y: number, effectIndex: number, effectParam: string, atPlayhead: boolean, cursorOnEvent: boolean, effectField: number, muted: boolean): number {
    const cursorField = FIRST_EFFECT_FIELD + (effectIndex * 3) + effectField;
    let colour;
    if (muted)
      colour = this.colours().channelMuted;
    else if (cursorOnEvent && this.editorState.cursorPosition.field === cursorField)
      colour = this.colours().cursorText;
    else
      colour = (effectParam === DASH) ? this.colours(atPlayhead).text : this.colours(atPlayhead).effect;
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
}
