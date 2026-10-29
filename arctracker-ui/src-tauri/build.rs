use std::env;
use std::ffi::{OsStr, OsString};
use std::path::{Path, PathBuf};
use std::process::{Command, ExitStatus};
use tauri_build::build;

fn run<I, S>(program: &str, arguments: I) -> ExitStatus
where
    I: IntoIterator<Item = S>,
    S: AsRef<OsStr>,
{
    let mut command = Command::new(program);
    command.args(arguments);
    if cfg!(target_os = "macos") {
        command.env("MACOSX_DEPLOYMENT_TARGET", "11.0");
    }
    eprintln!("Running: {command:?}");
    let status = command
        .status()
        .unwrap_or_else(|e| panic!("Failed to execute {program:?}: {e}"));
    if !status.success() {
        panic!("Failed to run command: {program:?}");
    }
    status
}

fn configure_meson(source_directory: &Path, build_directory: &Path, build_type: &str) {
    let core_data = build_directory.join("meson-private/coredata.dat");
    let mut args: Vec<OsString> = vec!["setup".into()];
    if core_data.exists() {
        args.push("--reconfigure".into());
    }
    args.extend([
        build_directory.as_os_str().to_owned(),
        source_directory.as_os_str().to_owned(),
        format!("-Dbuildtype={build_type}").into(),
        "-Ddefault_library=static".into(),
        "-Dasan=false".into(),
        "-Db_sanitize=none".into(),
    ]);
    if cfg!(target_os = "windows") {
        args.push("-Db_vscrt=mt".into());
    }
    run("meson", args);
}

fn main() {
    tauri_build::build();
    let manifest_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap());
    let out_directory = PathBuf::from(env::var("OUT_DIR").unwrap());
    let profile = env::var("PROFILE").unwrap();
    let engine_source_directory = manifest_dir.join("../../arctracker-engine");
    // Keep the Meson build directory shallow to avoid CMake's
    // object-file path length limit on Windows.
    let engine_build_directory = out_directory.ancestors()
        .nth(3)
        .expect("Unexpected Cargo OUT_DIR layout")
        .join("arctracker-engine");
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
    println!(
        "cargo:rustc-link-search=native={}",
        engine_build_directory.display()
    );
    let portaudio_library_directory = engine_build_directory.join("subprojects/portaudio-19.7.0");
    let rtmidi_library_directory = engine_build_directory.join("subprojects/rtmidi-6.0.0");
    println!(
        "cargo:rustc-link-search=native={}",
        portaudio_library_directory.display()
    );
    println!(
        "cargo:rustc-link-search=native={}",
        rtmidi_library_directory.display()
    );
    println!("cargo:rustc-link-lib=static=arctracker");
    println!("cargo:rustc-link-lib=static=portaudio_static");
    println!("cargo:rustc-link-lib=static=rtmidi");
    #[cfg(target_os = "macos")]
    {
        println!("cargo:rustc-link-lib=framework=CoreAudio");
        println!("cargo:rustc-link-lib=framework=AudioToolbox");
        println!("cargo:rustc-link-lib=framework=AudioUnit");
        println!("cargo:rustc-link-lib=framework=CoreFoundation");
        println!("cargo:rustc-link-lib=framework=CoreServices");
        println!("cargo:rustc-link-lib=framework=CoreMIDI");
        println!("cargo:rustc-link-lib=m");
        println!("cargo:rustc-link-lib=pthread");
        // RtMidi is implemented in C++.
        println!("cargo:rustc-link-lib=c++");
    }
    #[cfg(target_os = "windows")]
    {
        println!("cargo:rustc-link-lib=winmm");
        println!("cargo:rustc-link-lib=ole32");
        println!("cargo:rustc-link-lib=uuid");
    }
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
