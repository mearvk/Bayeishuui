# Native terminal game source

This directory contains two standalone implementations of Bayeishuui: `bayeishuui.c` (C11) and `bayeishuui.cpp` (C++17). Both use POSIX terminal input and ANSI true-colour output, with no graphics framework or third-party runtime dependency.

## Build and run from the repository root

```sh
make build-c
make run-c

make build-cpp
make run-cpp
```

The renderer draws a perspective dojo floor, coloured fighter silhouettes, health and stamina meters, range feedback, and combat messages. Controls: **W** advance, **S** retreat, **A/D** circle, **Space** strike, **G** guard, **Q** quit. Run in a Linux terminal at least 60 columns by 24 rows. The presentation is terminal graphics (ANSI/Unicode), not hardware-accelerated 3D.

Compiler requirements: a C11 compiler such as GCC or Clang for the C version; a C++17 compiler for the C++ version.
