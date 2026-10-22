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

## Fighter Guild campaign

The native C and C++ games feature **15 ranked opponents and 17 fights per opponent** (255 scheduled bouts). Both fighters have HP bars. Ground Game unlocks after 150 cumulative striking damage (150% of starting 100 HP), with control, ground-and-pound, and submission interactions. Opponents rate you 0–100 after each 17-fight series; 95+ is superior. Sudden Death after a knockout requires at least 11 wins and a lead in the overall record.

The progression runs from **Initiate** through **Copper, Bronze, Silver, Gold, Platinum, Jade, Ruby, Sapphire, Emerald, Obsidian, Master, Grandmaster, Champion**, and finally **Guild Legend**. The opponent roster is documented in [source/README.md](source/README.md).

## Terminal sprite animation

The player and each of the 15 Fighter Guild opponents have a consecutive **244-frame animation cycle** (indices 0–243), rendered directly as fixed-width ANSI/Unicode terminal sprites. The shared sprite engine adds fighter-specific stance silhouettes, color identity, and timing, with connected motion beats for Muay Thai strikes, footwork, recoil, ground control, ground-and-pound, and finishes. It is designed for the terminal rather than a raster or 3D renderer.

See [the sprite engine](source/bayeishuui_sprites.h) and [native source notes](source/README.md).

## Fighting styles

Muay Thai is the foundation of the native campaign: guard, teep, round kick, punches, knees and close-range elbows. Each of the 15 Fighter Guild opponents has an individual tactical style, displayed in the HUD and reflected in movement, pressure, timing or attack range. Use **Space** for straight/close-range strikes, **R** for a teep, and **E** for a round kick.

See the complete roster in [source/README.md](source/README.md).

## Native controls

| Key | Action |
| --- | --- |
| W | Advance toward the opponent |
| A / D | Circle left / right |
| S | Retreat |
| Space | Strike |
| G | Guard |
| J | Build ground control |
| K | Ground-and-pound |
| L | Attempt submission after building control |
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
