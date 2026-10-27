# T1

This project is an unfinished abstraction of the OS and graphics API. Only Mac OS X is available now, but ports will follow because that's the whole point.

Goals:
- Use C without dependencies, no objective-c or libc
- Basic OS functions: open windows, draw in windows, create mutexes, file I/O, read input devices (mouse, keyboard, gamepad), play sound streams 
- Basic GPU functions: make shader pipelines, send buffers, run compute shaders

<img width="991" alt="Screen Shot 2023-01-02 at 16 52 28" src="https://user-images.githubusercontent.com/40927974/210205771-546859c6-8602-4ab9-9cfd-fa41ccb1a3cf.png">

## How to build/concatenate

```
gcc concat.c -o concat
./concat
```

Your files (T1.h & T1.c) are now in the build folder, copy them into your project and you're done. There are also examples in the build folder, take a look.

## Examples

`examples/noise.c` is a tiny T1 app: it draws its own pixels and T1 shows them in a window, no GPU setup or shaders needed. `./concat` builds every example along with the library, run them from the tok_one folder with e.g. `./build/examples/noise`.

Reading the example code and running the example programs is a faster way to get started than to read all of the documentation
