Yes. Given those prerequisites, the goal should be:

```text
git clone …
npm install
npm run tauri dev
```

No manual Meson build, no editing paths, and no pre-existing `builddir-asan`.

The main change is that `build.rs` should **build the C engine itself**, into Cargo’s private output directory, and then tell Rust where the resulting library is. Meson should continue to discover PortAudio from the developer’s environment.

## 1. Make the repository self-contained

Ideally, the checkout should look roughly like this:

```text
arctracker/
├── package.json
├── src/
├── src-tauri/
│   ├── Cargo.toml
│   ├── build.rs
│   └── src/
└── arctracker-engine/
    ├── meson.build
    ├── include/
    └── src/
```

The important point is that the C engine source is inside the same clone—either directly, as a Git submodule, or as a Cargo-managed sibling that is reliably present.

Your current path:

```rust
manifest_dir.join("../../arctracker-engine/builddir-asan")
```

suggests Cargo is reaching outside the Arctracker repository into a separately cloned project. That immediately prevents “clone one repo and build” unless the repository setup automates fetching the engine.

If the engine is genuinely a separate repository, the usual choices are:

* add it as a Git submodule;
* make Arctracker a parent repository containing both projects;
* or move the engine source into the Arctracker repository.

For a project of this scale, I would probably put both under one repository unless you have a real need to release the engine independently.

## 2. Ensure Meson builds an actual library target

Your engine’s Meson project should expose a static library target, rather than only building the CLI executable.

Conceptually:

```meson
project(
  'arctracker-engine',
  'c',
  version: '0.4.2',
  default_options: [
    'c_std=c17',
    'warning_level=3',
  ],
)

portaudio = dependency('portaudio-2.0', required: true)

engine_sources = files(
  # ...
)

arctracker_library = static_library(
  'arctracker',
  engine_sources,
  dependencies: portaudio,
  install: false,
)
```

Meson’s `dependency()` function searches for installed dependencies using mechanisms including `pkg-config` and CMake metadata, so the Meson file should say what it needs rather than where it happens to be installed. ([mesonbuild.com][1])

Do not put `/opt/homebrew`, `/usr/local`, `/usr/lib`, or Windows installation directories into `meson.build`.

The engine’s standalone CLI can link the same target:

```meson
executable(
  'arctracker',
  cli_sources,
  link_with: arctracker_library,
  dependencies: portaudio,
)
```

## 3. Have `build.rs` invoke Meson

Add any required build-script dependencies to `src-tauri/Cargo.toml`:

```toml
[build-dependencies]
tauri-build = { version = "2", features = [] }
pkg-config = "0.3"
```

Then replace the hardcoded script with something along these lines:

```rust
use std::env;
use std::ffi::OsStr;
use std::path::{Path, PathBuf};
use std::process::{Command, ExitStatus};

fn run<I, S>(program: &str, arguments: I) -> ExitStatus
where
    I: IntoIterator<Item = S>,
    S: AsRef<OsStr>,
{
    let mut command = Command::new(program);
    command.args(arguments);

    eprintln!("Running: {command:?}");

    let status = command
        .status()
        .unwrap_or_else(|error| panic!("Could not run {program}: {error}"));

    if !status.success() {
        panic!("{program} failed with status {status}");
    }

    status
}

fn configure_meson(
    source_directory: &Path,
    build_directory: &Path,
    build_type: &str,
) {
    let coredata = build_directory.join("meson-private/coredata.dat");

    if coredata.exists() {
        run(
            "meson",
            [
                "setup".as_ref(),
                "--reconfigure".as_ref(),
                build_directory.as_os_str(),
                source_directory.as_os_str(),
                format!("-Dbuildtype={build_type}").as_ref(),
                "-Ddefault_library=static".as_ref(),
            ],
        );
    } else {
        run(
            "meson",
            [
                "setup".as_ref(),
                build_directory.as_os_str(),
                source_directory.as_os_str(),
                format!("-Dbuildtype={build_type}").as_ref(),
                "-Ddefault_library=static".as_ref(),
            ],
        );
    }
}

fn main() {
    tauri_build::build();

    let manifest_directory = PathBuf::from(
        env::var_os("CARGO_MANIFEST_DIR")
            .expect("CARGO_MANIFEST_DIR was not set"),
    );

    let out_directory = PathBuf::from(
        env::var_os("OUT_DIR")
            .expect("OUT_DIR was not set"),
    );

    let profile = env::var("PROFILE")
        .expect("PROFILE was not set");

    let engine_source_directory =
        manifest_directory.join("../arctracker-engine");

    let engine_build_directory =
        out_directory.join("arctracker-engine");

    let meson_build_type = match profile.as_str() {
        "release" => "release",
        _ => "debug",
    };

    configure_meson(
        &engine_source_directory,
        &engine_build_directory,
        meson_build_type,
    );

    run(
        "meson",
        [
            "compile".as_ref(),
            "-C".as_ref(),
            engine_build_directory.as_os_str(),
        ],
    );

    /*
     * Adjust this if Meson places libarctracker in a subdirectory
     * such as engine_build_directory.join("src").
     */
    println!(
        "cargo:rustc-link-search=native={}",
        engine_build_directory.display()
    );

    /*
     * A static arctracker library may still refer to PortAudio symbols,
     * so the final Tauri executable must link PortAudio too.
     */
    pkg_config::Config::new()
        .probe("portaudio-2.0")
        .expect(
            "PortAudio development files were not found. \
             Install PortAudio and ensure pkg-config can locate it.",
        );

    println!("cargo:rustc-link-lib=static=arctracker");

    println!(
        "cargo:rerun-if-changed={}",
        engine_source_directory.join("meson.build").display()
    );

    println!(
        "cargo:rerun-if-changed={}",
        engine_source_directory.join("src").display()
    );

    println!(
        "cargo:rerun-if-changed={}",
        engine_source_directory.join("include").display()
    );
}
```

Cargo officially provides `OUT_DIR` as the build script’s private output location, and `cargo::rustc-link-search`, `cargo::rustc-link-lib`, and `cargo::rerun-if-changed` are the intended mechanisms for exposing generated native libraries and controlling rebuilds. ([Rust Documentation][2])

The precise path to the resulting archive needs checking. Depending on your Meson layout, it might be:

```rust
let engine_library_directory =
    engine_build_directory.join("src");
```

and then:

```rust
println!(
    "cargo:rustc-link-search=native={}",
    engine_library_directory.display()
);
```

## 4. Let standard tools find PortAudio

This replaces:

```rust
println!("cargo:rustc-link-search=native=/opt/homebrew/lib");
println!("cargo:rustc-link-lib=portaudio");
```

with:

```rust
pkg_config::Config::new()
    .probe("portaudio-2.0")
    .expect("PortAudio development files were not found");
```

The `pkg-config` Rust crate invokes the system `pkg-config` tool and emits the appropriate Cargo linker settings for the discovered library. ([Docs.rs][3])

On macOS and Linux, that should give you the desired experience provided the PortAudio development installation supplies `portaudio-2.0.pc`.

For example, the developer might need:

```bash
# macOS
brew install portaudio pkg-config meson ninja

# Debian/Ubuntu
sudo apt install \
  build-essential \
  meson \
  ninja-build \
  pkg-config \
  portaudio19-dev
```

The repository should not care where those packages are installed.

Windows will probably need a separate PortAudio discovery path later—likely vcpkg, MSYS2, or a Meson wrap—but you need not solve that before establishing the Unix build. The important thing is that Windows-specific logic will be based on `CARGO_CFG_TARGET_OS`, not hardcoded paths from any particular Windows machine.

## 5. Remove ASan from the normal build

Delete all of this:

```rust
println!(
    "cargo:rustc-link-search=native=/Library/Developer/CommandLineTools/usr/lib/clang/21/lib/darwin"
);
println!("cargo:rustc-link-lib=dylib=clang_rt.asan_osx_dynamic");
println!(
    "cargo:rustc-link-arg=-Wl,-rpath,/Library/Developer/CommandLineTools/usr/lib/clang/21/lib/darwin"
);
```

That should not be part of a normal clone-and-build workflow.

Add an optional Meson configuration instead. For example, in `meson_options.txt`:

```meson
option(
  'sanitizers',
  type: 'boolean',
  value: false,
  description: 'Enable development sanitizers',
)
```

Then in `meson.build`:

```meson
if get_option('sanitizers')
  add_project_arguments(
    '-fsanitize=address',
    language: 'c',
  )

  add_project_link_arguments(
    '-fsanitize=address',
    language: 'c',
  )
endif
```

Or use Meson’s built-in sanitizer option when invoking setup:

```text
-Db_sanitize=address
```

Your ordinary build would leave it disabled. Your personal memory-debugging workflow could enable it explicitly through an environment variable or a separate script.

For instance:

```rust
let sanitizers_enabled =
    env::var_os("ARCTRACKER_SANITIZE").is_some();

println!("cargo:rerun-if-env-changed=ARCTRACKER_SANITIZE");
```

and while constructing the Meson setup command:

```rust
if sanitizers_enabled {
    command.arg("-Db_sanitize=address");
}
```

You may still need additional coordination to instrument or link the Rust executable correctly. I would therefore treat an end-to-end mixed Rust/C ASan build as a separate developer workflow, not part of the first portability refactor.

## 6. Do not use the build directory as the change input

This current line is backwards:

```rust
println!("cargo:rerun-if-changed={}", lib_path.display());
```

It watches generated output. You want to watch the **inputs**:

```rust
println!(
    "cargo:rerun-if-changed={}",
    engine_source_directory.join("meson.build").display()
);
println!(
    "cargo:rerun-if-changed={}",
    engine_source_directory.join("src").display()
);
println!(
    "cargo:rerun-if-changed={}",
    engine_source_directory.join("include").display()
);
```

Cargo then reruns `build.rs` when the C source or Meson definitions change, and `meson compile` decides which native objects actually need rebuilding. Cargo documents `rerun-if-changed` specifically as watching source paths that affect build-script output. ([Rust Documentation][2])

## 7. Add a single top-level developer command

Your `package.json` presumably already has a Tauri development command. You could make the intended workflow explicit:

```json
{
  "scripts": {
    "dev": "tauri dev",
    "build": "tauri build"
  }
}
```

Then the clean-clone experience becomes:

```bash
git clone <repository>
cd arctracker
npm install
npm run dev
```

During `npm run dev`:

```text
Tauri invokes Cargo
    ↓
Cargo runs build.rs
    ↓
build.rs configures and compiles the C engine with Meson
    ↓
Meson discovers PortAudio
    ↓
Cargo discovers the final PortAudio linker settings
    ↓
Rust links libarctracker and PortAudio
    ↓
Arctracker starts
```

That is the build contract you are after.

## One concern I would resolve before coding this

At present, Meson finds PortAudio to compile the engine, while Cargo also needs to find PortAudio to link the final executable. That duplication is not catastrophic, but it means two build systems know about the dependency.

A longer-term cleaner boundary would be:

```text
libarctracker-core.a
    no PortAudio dependency

libarctracker-portaudio.a
    PortAudio implementation

CLI
    links core + PortAudio backend

Tauri
    links core + whichever backend it uses
```

You have already introduced `audio_api_t`, so your architecture may be close to supporting this. But it is not necessary for the initial clean-clone build. For now, allowing both Meson and Cargo’s `pkg-config` helper to locate the same installed PortAudio is perfectly pragmatic.

The immediate checklist is therefore:

```text
[ ] Put the engine source inside the clone
[ ] Make Meson produce libarctracker.a
[ ] Build it beneath Cargo OUT_DIR
[ ] Have build.rs run meson setup and meson compile
[ ] Discover PortAudio through pkg-config
[ ] Remove all Homebrew and Apple Clang paths
[ ] Make ASan optional
[ ] Watch C source inputs, not generated build output
[ ] Document npm install + npm run dev
```

After those changes, your hypothetical developer should require knowledge of neither your directory layout nor the fact that you ordinarily use an ASan build. The build system will have become a reproducible description of how Arctracker is made, rather than a record of how it happens to be built on your Mac.

[1]: https://mesonbuild.com/Reference-manual_functions_dependency.html?utm_source=chatgpt.com "dependency()"
[2]: https://doc.rust-lang.org/cargo/reference/build-scripts.html?utm_source=chatgpt.com "Build Scripts - The Cargo Book"
[3]: https://docs.rs/pkg-config?utm_source=chatgpt.com "pkg_config - Rust"
