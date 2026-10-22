# Bayeishuui — Martial Arts Combat

Bayeishuui is a Linux-terminal martial-arts duel focused on striking, footwork, range, stamina, and reading an opponent's behavior. The native renderer uses ANSI true-colour and Unicode terminal drawing to create a perspective dojo floor, coloured fighter silhouettes, health/stamina meters, range feedback, and live combat messages.

## Native C and C++ builds

The native game is in `/source` and has two standalone implementations. They require no third-party graphics libraries.

```sh
make build-c
make run-c

make build-cpp
make run-cpp
```

To compile all implementations and run the Python syntax check:

```sh
make check
```

Requirements for the native games: GCC or Clang, a Linux/POSIX terminal supporting ANSI escape sequences and UTF-8, and a terminal at least 60 columns by 24 rows. The graphics are rendered directly in the terminal; this is not hardware-accelerated 3D.

## Native controls

| Key | Action |
| --- | --- |
| W | Advance toward the opponent |
| A / D | Circle left / right |
| S | Retreat |
| Space | Strike |
| G | Guard |
| Q | Quit the bout |

## Python version

The original Python terminal implementation remains in `inventory/bayeshuui.py`:

```sh
make run
python3 inventory/bayeshuui.py --difficulty hard
```

## Repository layout

- `source/bayeishuui.c` — native C11 terminal game.
- `source/bayeishuui.cpp` — native C++17 terminal game.
- `source/README.md` — native build and renderer notes.
- `inventory/bayeshuui.py` — Python terminal game.
- `MARTIAL_ARTS_DESIGN.md` — design direction and future phases.
- `CONTROLS_AND_COMBAT.md` — input and combat contract.
- `Makefile` — build, run, check, and clean targets.

The game is a stylized terminal-rendered approximation rather than a true 3D renderer or a biomechanically validated fighting simulator.

## License

Eclipse Public License 2.0; see [LICENSE](LICENSE).
