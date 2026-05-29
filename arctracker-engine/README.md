To build it with AddressSanitizer:
```shell
mkdir builddir-asan
meson setup builddir-asan -Dasan=true
meson compile -C builddir-asan
```

To build it without:
```shell
mkdir builddir
meson setup builddir -Dasan=false
meson compile -C builddir
```