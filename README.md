# How to build Arctracker for development
## Prerequisites
1. C development tools
    1. macOS: XCode command line development tools (or full XCode)
    1. Windows: MSVC
    1. Linux: GCC
1. Meson
1. CMake
1. Rust
1. Node
1. (Linux only) `libasound2-dev`
## Instructions
```
cd arctracker-ui
npm install # you only need to do this the first time
npm run dev
```
Then, in another shell:
```
cd arctracker-ui/src-tauri
cargo clean # you only need to do this the first time
cargo run
```
Alternatively, you can do this to run Arctracker with a single command:
```
cd arctracker-ui
npm run tauri dev
```
however, anything printed to stdout will be swallowed. For development purposes you may find it more convenient to run `npm run dev` and `cargo run` separately.

# How to do a release build

## Build the icons
```
cd arctracker-ui
pnpm run tauri icon src-tauri/icons/arctracker-icon-square.png  # for macOS
pnpm run tauri icon src-tauri/icons/arctracker-icon-rounded.png # for Windows and Linux
```

## Build the app bundle
```
npm run tauri build -- --bundles app                                         # for macOS
sudo cp src-tauri/target/release/bundle/macos/Arctracker.app /Applications/. # for macOS (alternatively put it in ~/Applications)
```
