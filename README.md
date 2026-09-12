# How to build Arctracker
## Macos
### Prerequisites
1. XCode command line development tools (or full XCode)
2. Meson: `brew install meson`
3. CMake: `brew install cmake`
4. Rust: `brew install rust`
5. Node: `brew install node`
### Instructions
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
