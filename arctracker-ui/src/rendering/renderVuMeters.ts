import { engine } from "../engine/engine.ts";

export type VuMeterColours = {
  clipIndicatorOff: string;
  clipIndicatorOn: string;
  guide: string;
  vuMeterBottom: string;
  vuMeterMiddle: string;
  vuMeterTop: string;
};

type Levels = {
  left: number;
  right: number;
};

type Rectangle = {
  x: number;
  y: number;
  width: number;
  height: number;
};

type Circle = {
  x: number;
  y: number;
  radius: number;
};

type Objects = {
  meterL: Rectangle;
  meterR: Rectangle;
  clipIndicatorL: Circle;
  clipIndicatorR: Circle;
};

const objects: Objects = {
  meterL: { x: 1, y: 24, width: 210, height: 22 },
  meterR: { x: 1, y: 56, width: 210, height: 22 },
  clipIndicatorL: { x: 230, y: 35, radius: 10 },
  clipIndicatorR: { x: 230, y: 67, radius: 10 },
};

const FALL_PER_SECOND = 1.5;
const CANVAS_WIDTH = 240;
const CANVAS_HEIGHT = 100;
const FULL_CIRCLE = 2 * Math.PI;

export class VuMeterRenderer {
  private readonly ctx: CanvasRenderingContext2D;
  private readonly colours: VuMeterColours;
  private readonly vuGradient: CanvasGradient;
  private displayed: Levels;
  private clip: { left: boolean; right: boolean };
  private lastTimestamp: number | null;

  public constructor(ctx: CanvasRenderingContext2D, colours: VuMeterColours) {
    this.ctx = ctx;
    this.colours = colours;
    this.vuGradient = ctx.createLinearGradient(
      objects.meterL.x,
      0,
      objects.meterL.width + 1,
      0,
    );
    this.vuGradient.addColorStop(0, colours.vuMeterBottom);
    this.vuGradient.addColorStop(0.8, colours.vuMeterMiddle);
    this.vuGradient.addColorStop(1, colours.vuMeterTop);
    this.displayed = { left: 0, right: 0 };
    this.clip = { left: false, right: false };
    this.lastTimestamp = null;
  }

  public async render(timestamp: number) {
    const peakLevels = await engine.getAndResetPeakLevels();
    this.checkForClipping(peakLevels);
    const dt = this.timeSinceLastAnimationFrame(timestamp);
    this.calculateNewMeterLevels(peakLevels, dt);
    this.ctx.clearRect(0, 0, CANVAS_WIDTH, CANVAS_HEIGHT);
    this.renderVuMeters();
    this.renderClipIndicators();
  }

  private timeSinceLastAnimationFrame(timestamp: number) {
    const last = this.lastTimestamp ?? timestamp;
    this.lastTimestamp = timestamp;
    return (timestamp - last) / 1000;
  }

  private checkForClipping(peakLevels: Levels) {
    this.clip.left = peakLevels.left > 1.0;
    this.clip.right = peakLevels.right > 1.0;
  }

  private calculateNewMeterLevels(peakLevels: Levels, dt: number) {
    this.displayed.left = Math.max(
      this.clip.left ? 1.0 : peakLevels.left,
      this.displayed.left - FALL_PER_SECOND * dt,
    );
    this.displayed.right = Math.max(
      this.clip.right ? 1.0 : peakLevels.right,
      this.displayed.right - FALL_PER_SECOND * dt,
    );
  }

  private renderVuMeters() {
    this.ctx.strokeStyle = this.colours.guide;
    this.ctx.lineWidth = 1;
    this.strokeRect(objects.meterL);
    this.strokeRect(objects.meterR);
    this.ctx.fillStyle = this.vuGradient;
    if (this.displayed.left > 0)
      this.fillRect({
        ...objects.meterL,
        width: this.displayed.left * objects.meterL.width,
      });
    if (this.displayed.right > 0)
      this.fillRect({
        ...objects.meterR,
        width: this.displayed.right * objects.meterR.width,
      });
  }

  private renderClipIndicators() {
    this.ctx.fillStyle = this.clip.left
      ? this.colours.clipIndicatorOn
      : this.colours.clipIndicatorOff;
    this.fillCircle(objects.clipIndicatorL);
    this.ctx.fillStyle = this.clip.right
      ? this.colours.clipIndicatorOn
      : this.colours.clipIndicatorOff;
    this.fillCircle(objects.clipIndicatorR);
  }

  private strokeRect(rectangle: Rectangle) {
    this.ctx.strokeRect(
      rectangle.x,
      rectangle.y,
      rectangle.width,
      rectangle.height,
    );
  }

  private fillRect(rectangle: Rectangle) {
    this.ctx.fillRect(
      rectangle.x,
      rectangle.y,
      rectangle.width,
      rectangle.height,
    );
  }

  private fillCircle(circle: Circle) {
    this.ctx.beginPath();
    this.ctx.arc(circle.x, circle.y, circle.radius, 0, FULL_CIRCLE);
    this.ctx.fill();
  }
}
