To build it with AddressSanitizer:
```shell
mkdir builddir-asan
meson setup builddir-asan -Db_sanitize=address
meson compile -C builddir-asan
```

To build it without:
```shell
mkdir builddir
meson setup builddir
meson compile -C builddir
```