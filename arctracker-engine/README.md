## How to build the engine
With Address Sanitizer:
```shell
mkdir builddir-asan
meson setup builddir-asan -Db_sanitize=address
meson compile -C builddir-asan
```

Without Address Sanitizer:
```shell
mkdir builddir
meson setup builddir
meson compile -C builddir
```

## Console UI
The engine has a bundled console UI which serves both as a useful test bed and also as a reference implementation for how to use the engine library. To run it, replace `<build dir>` with whatever name you gave your build directory (see above) and run: 
```
<build dir>/arctracker-console <path to modfile>
```
The controls for the engine are:
* space - toggles play/pause
* x - seek forwards
* z - seek backwards
* l - toggle pattern loop mode
* q - exit

Note there is no bounds checking on the sequence. If you attempt to seek backwards from position 0 or seek forwards from the end of the song, the UI will crash.

To export a module as a WAV file, run:
```
<build dir>/arctracker-console --output-file=<path to WAV output> <path to modfile>
```
