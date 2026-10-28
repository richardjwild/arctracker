Yes, absolutely. Tauri 2 allows you to construct and install native application menus entirely from TypeScript. You don't have to build them in Rust.

And looking at your three reasons, I think you've made a pretty compelling case for moving Arctracker's menu definitions to the frontend.

In particular, your third point is the one that would tip the balance for me: duplicating menu command IDs between Rust and TypeScript is a maintenance nuisance, especially when the actual behaviour of those commands lives in TypeScript anyway.

Moving the menus would give you one place to define the command IDs, their labels, their keyboard accelerators, and their initial enabled states. It would also make synchronising menu availability with your Zustand state much more straightforward.

## What it would look like

The TypeScript API lets you construct an entire menu hierarchy declaratively:

```
import { Menu } from "@tauri-apps/api/menu";

const menu = await Menu.new({
  items: [
    {
      id: "instrument-menu",
      text: message("instrumentMenuLabel"),
      items: [
        {
          id: "edit-instrument",
          text: message("editInstrumentMenuLabel"),
          accelerator: "CmdOrCtrl+I",
          enabled: false,
          action: () => editor.editInstrument(),
        },
        { item: "Separator" },
        {
          id: "export-sample",
          text: message("exportSampleMenuLabel"),
          enabled: false,
          action: () => editor.exportSample(),
        },
      ],
    },
  ],
});

await menu.setAsAppMenu();
```

The command methods are illustrative; substitute your existing Arctracker actions. Tauri supports nested menu definitions, accelerators, enabled states and JavaScript action callbacks.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://v2.tauri.app\&sz=32)

Tauri+1

That's a considerably more compact representation than constructing every `MenuItem` and `Submenu` individually in Rust.

And because you created the menu in TypeScript, you can retain its handle and use it for subsequent updates. No Rust commands, no duplicated IDs across languages, and no confusion over `Menu.default()`.

## How your three concerns would be addressed

- Unwieldy Rust code: Replaced with declarative TypeScript definitions. You could still split the definitions into `fileMenu.ts`, `instrumentMenu.ts`, etc., as the menu grows.
- Internationalisation: Labels come directly from your existing `message()` mechanism. If you later support switching languages while the application is running, Tauri also exposes `setText()` to update them.
- Duplicated IDs: Gone. The frontend can define IDs once and use them for menu construction, state synchronisation and action dispatch.

There's also a fourth benefit: menu actions can call your existing TypeScript command implementations directly. You might eventually find that your menu items, keyboard bindings and toolbar controls can all invoke the same underlying actions.

## A couple of considerations

Because Arctracker runs on macOS, I'd pay particular attention to preserving the standard application menu behaviour: About, Services, Hide, Quit and so on. Tauri supports predefined menu items for these, so you don't have to implement them yourself.

Also, `setAsAppMenu()` installs the application-wide menu on macOS. On Windows and Linux, `setAsWindowMenu()` is available for window-specific menus.&#x20;

[image](https://www.google.com/s2/favicons?domain=https://v2.tauri.app\&sz=32)

Tauri+1



I'd initialise the menu once during frontend startup, keep its handle in your menu module, and let your Zustand subscriptions update the relevant items as editor state changes. No need to involve React component lifecycles.

One small distinction worth making: the menu is still a native menu. You're merely constructing and controlling it from TypeScript. You aren't replacing it with an HTML imitation.

## Would I stop and refactor now?

Yes, I think I would.

Ordinarily, I'd be inclined to say that a working Rust menu isn't worth rewriting merely to save some code. But your internationalisation concern and the duplicated IDs are architectural reasons, not cosmetic ones.

And you've discovered this while you're already working on menu-state synchronisation, before investing much more effort in the Rust-to-TypeScript bridge.

I'd migrate one submenu first — Instrument is the obvious candidate — and verify that its accelerators, actions and enabled states behave correctly. Then move the rest across.

I suspect you'll end up deleting rather more code than you add, which is always a pleasant outcome for a refactor.