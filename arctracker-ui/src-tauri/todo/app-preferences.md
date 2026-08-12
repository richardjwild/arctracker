Yes. Tauri gives you exactly the abstraction you want.

For ordinary application preferences, the most direct path is:

```ts
import { appConfigDir } from "@tauri-apps/api/path";

const configDir = await appConfigDir();
```

That resolves to the platform-appropriate configuration directory plus your Tauri bundle identifier. Tauri defines it as:

```text
${configDir}/${bundleIdentifier}
```

where `bundleIdentifier` comes from `tauri.conf.json`. ([Tauri][1])

The underlying configuration locations are conventionally:

* Linux: `$XDG_CONFIG_HOME`, falling back to `$HOME/.config`
* macOS: `$HOME/Library/Application Support`
* Windows: the roaming application-data folder, normally `%APPDATA%`

Tauri handles those differences for you. ([Tauri][1])

So Arctracker might end up with paths resembling:

```text
Linux:
~/.config/com.yourdomain.arctracker/config.json

macOS:
~/Library/Application Support/com.yourdomain.arctracker/config.json

Windows:
%APPDATA%\com.yourdomain.arctracker\config.json
```

The exact directory name depends on your configured bundle identifier.

You then have two good approaches.

### A plain configuration file

Using the filesystem plugin, you can write relative to `BaseDirectory.AppConfig`:

```ts
import {
  BaseDirectory,
  readTextFile,
  writeTextFile,
} from "@tauri-apps/plugin-fs";

export type AppConfig = {
  pianoKeyboardTranspose: number;
  patternGridStrideLength: number;
};

export async function saveConfig(config: AppConfig): Promise<void> {
  await writeTextFile(
    "config.ts.json",
    JSON.stringify(config, null, 2),
    { baseDir: BaseDirectory.AppConfig },
  );
}
```

Tauri’s filesystem documentation specifically shows writing a JSON configuration file using `BaseDirectory.AppConfig`. ([Tauri][2])

You would also need to create the directory on first use and grant the appropriate filesystem capability permissions.

This option is attractive if you want:

* one typed configuration object;
* an inspectable, user-editable file;
* explicit versioning and migration;
* complete control over loading and validation.

For Arctracker, I suspect this would suit your instincts rather well.

### Tauri’s Store plugin

Tauri also has an official persistent key-value store:

```ts
import { LazyStore } from "@tauri-apps/plugin-store";

const settings = new LazyStore("settings.json");

await settings.set("pianoKeyboardTranspose", 2);
await settings.save();

const transpose =
  await settings.get<number>("pianoKeyboardTranspose");
```

The Store plugin persists its file in the application data directory and works across Windows, Linux and macOS. It can be accessed from either JavaScript or Rust. ([Tauri][3])

This is convenient for loosely coupled preferences such as:

* last-used directory;
* theme;
* MIDI device;
* editor stride;
* keyboard octave;
* window preferences.

My slight reservation is that a key-value store can allow configuration to become an unstructured bag of settings. A single `AppConfig` structure gives you a clearer schema and one place to apply defaults and migrations.

I would probably use:

```text
AppConfig file:
application behaviour and user preferences

Window-state plugin:
window size, position and maximised state
```

Tauri has a dedicated window-state plugin which automatically saves and restores window state, so there is little reason to reinvent that part yourself. ([Tauri][4])

One other distinction is worth preserving:

* **AppConfig**: user preferences and configurable behaviour.
* **AppData**: persistent application-created data that is not really configuration.
* **AppCache**: disposable derived data.

Tauri exposes platform-neutral paths for all three. ([Tauri][1])

For Arctracker, my default recommendation would be a versioned JSON file under `BaseDirectory.AppConfig`, loaded into a typed TypeScript structure with defaults. That would give you a clean, explicit configuration model while Tauri takes care of the Windows/macOS/Linux path conventions.

[1]: https://v2.tauri.app/reference/javascript/api/namespacepath/?utm_source=chatgpt.com "path"
[2]: https://v2.tauri.app/plugin/file-system/ "File System | Tauri"
[3]: https://v2.tauri.app/plugin/store/ "Store | Tauri"
[4]: https://v2.tauri.app/plugin/window-state/?utm_source=chatgpt.com "Window State"
