use std::env;
use std::path::PathBuf;

fn main() {
    tauri_build::build();
    let manifest_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap());
    let lib_path = manifest_dir.join("../../arctracker-engine/builddir-asan");
    println!("cargo:rustc-link-search=native={}", lib_path.display());
    println!("cargo:rustc-link-search=native=/opt/homebrew/lib");
    println!("cargo:rustc-link-lib=portaudio");
    println!("cargo:rustc-link-lib=rtmidi");
    println!("cargo:rustc-link-search=native=/Library/Developer/CommandLineTools/usr/lib/clang/21/lib/darwin");
    println!("cargo:rustc-link-lib=dylib=clang_rt.asan_osx_dynamic");
    println!("cargo:rustc-link-arg=-Wl,-rpath,/Library/Developer/CommandLineTools/usr/lib/clang/21/lib/darwin");
    println!("cargo:rustc-link-lib=static=arctracker");
    println!("cargo:rerun-if-changed={}", lib_path.display()); // Ensure rebuild if library changes.
}
