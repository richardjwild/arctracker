Yes—but there’s an important catch: the constructor does **not** depend only on those four parameters.

It also depends on several pieces of ambient state:

```ts
useStore.getState().trackMuteState
useStore.getState().trackPanning
useStore.getState().editorState
useStore.getState().effectsDisplayed
useStore.getState().patternSelection
```

and on the current CSS environment:

```ts
cssProperty(...)
```

plus whatever state or configuration `patternLayout` itself uses.

So retaining the renderer until one of the four arguments changes would alter its behaviour. Cursor movement, selection changes, mute changes, panning changes, or a theme change could leave it rendering stale values.

That said, I think your instinct has exposed a genuine design smell. The constructor currently does two rather different jobs:

1. It captures the state needed for **this frame**.
2. It constructs or retrieves resources that are mostly **stable between frames**.

The first category includes:

```ts
pattern
trackMuted
trackPanning
editorState
effectsDisplayed
patternSelection
```

The second includes:

```ts
ctx
coloursAtPlayhead
coloursOffPlayhead
fontTrackHeader
fontPatternData
```

And some values sit in between:

```ts
patternLayout
gridViewportFit
```

Those depend on viewport dimensions and perhaps track count, so they need recalculating after resize or layout changes—but certainly not necessarily every frame.

The CSS reads concern me more than the single `PatternRenderer` allocation. Every frame you currently create:

* one renderer;
* two colour objects;
* roughly 34 CSS property lookups;
* the layout objects returned by the layout calculations.

The renderer object itself is likely trivial. Repeatedly querying computed CSS values may be considerably less trivial, depending on what `cssProperty()` does.

I would probably move towards this conceptual shape:

```ts
renderer.render({
  pattern,
  viewportSize,
  numTracks,
  trackMuted,
  trackPanning,
  editorState,
  effectsDisplayed,
  patternSelection,
});
```

The renderer instance would retain only genuinely durable resources, perhaps:

```ts
class PatternRenderer {
  constructor(
    private readonly ctx: CanvasRenderingContext2D,
    private readonly colours: PatternColours,
    private readonly fonts: PatternFonts,
  ) {}
}
```

Then the current state would be supplied to `render()` each frame rather than captured permanently in the constructor.

That avoids a potentially awkward mutable object with methods such as:

```ts
renderer.setPattern(...)
renderer.setViewportSize(...)
renderer.setSelection(...)
renderer.setMutedTracks(...)
```

which could easily become another little state store that must be kept synchronised. The renderer remains essentially stateless with respect to the application: durable drawing resources live on the instance, while the complete truth for a frame arrives together.

For the layout calculations, you could cache them according to their actual inputs:

```text
pattern layout:
    recalculate when viewportSize changes

grid viewport fit:
    recalculate when viewportSize or numTracks changes
```

The colours and fonts could be read once, then refreshed when the theme changes. If Arctracker has no runtime theme switching yet, reading them when the canvas or renderer is initialised would probably be enough for now.

There is also a naming insight here. The current constructor suggests `PatternRenderer` represents a particular rendering of:

```text
pattern + viewport + track count + application state + CSS theme
```

But its actual identity may be simpler:

> the thing capable of drawing Arctracker patterns into this canvas context.

In that model, the context belongs in its lifetime; the pattern does not.

So I would not use “reconstruct whenever the four constructor arguments change” as the rule, because those are not all its dependencies. I would instead use this discovery to separate:

```text
long-lived renderer resources
```

from:

```text
per-frame rendering inputs
```

That is cleaner independently of performance—and should incidentally remove most of the per-frame allocation and repeated CSS work.
