## How to build the engine
With Address Sanitizer:
```shell
mkdir builddir-asan                                             # You only need to do this the first time.
meson setup builddir-asan -Db_sanitize=address -Db_lundef=false # You only need to do this the first time.
meson compile -C builddir-asan
```

Without Address Sanitizer:
```shell
mkdir builddir        # You only need to do this the first time.
meson setup builddir  # You only need to do this the first time.
meson compile -C builddir
```

The output of the build is a static library called `libarctracker`.

To run the tests:
```shell
meson setup build-test -Dtests=true -Db_sanitize=address # You only need to do this the first time.
meson test -C build-test --print-errorlogs
```

To run the tests in CLion, add these setup options in _Settings_ > _Build, Execution, Deployment_ > _Meson_:

    -Dtests=true -Db_sanitize=address


(I assume something similar is required for Visual Studio and other IDEs but I don't know).

## Console UI

> Note: the console ui is currently disabled in the Windows build.

The engine has a bundled console UI which serves both as a useful test bed and also as a reference implementation for how to use the engine library. To run it, replace `<build dir>` with whatever name you gave your build directory (e.g. `builddir-asan` — see above) and run: 

    <build dir>/arctracker-console <modfile> <modfile> ...

The controls for the engine are:
* `space` — toggles play/pause (the player starts in paused mode)
* `x` — seeks forwards
* `z` — seeks backwards
* `l` — toggles pattern loop mode
* `q` — eject module or exit

> Note: there is no bounds checking on the sequence. If you attempt to seek backwards from position 0 or seek forwards from the end of the song, the UI will crash.

If more than one modfile is specified, the program will load each of them in turn; pressing q will eject the current module and load the next one. When the last module has been loaded, q will exit the program.

To export a module as a WAV file, run:

    <build dir>/arctracker-console --output=<path to WAV output> <path to modfile>

