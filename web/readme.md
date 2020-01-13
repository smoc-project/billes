# web

You will need https://github.com/texane/stlink installed on your computer.
You will also need an ARM gcc toolchain `arm-none-eabi-gcc` / `gcc-arm-none-eabi` depending on your platform.

To build the project just run `make` it will create a `build/billes.elf` binary.

To load it:

- Start `st-util` in one terminal
- In the other run a `gdb` session and inside it:

```gdb
target extend-remote :4242
load build/billes.elf
```
