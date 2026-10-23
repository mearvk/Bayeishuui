# Bayeishuui - /beɪeɪʃʌʌɪ/ — Martial Arts Combat

Bayeishuui is a Linux-terminal martial-arts duel focused on striking, footwork, range, stamina, and reading an opponent's behavior. The native renderer uses ANSI true-colour and Unicode terminal drawing to create a perspective dojo floor, coloured fighter silhouettes, health/stamina meters, range feedback, and live combat messages.

## Native C and C++ builds

The native game is in `/source` and has two standalone implementations. They require no third-party graphics libraries. The C and C++ builds **drive** Bayeishuui — `make run` launches the game.

```sh
make            # build both native binaries (default target)

make build-c
make run-c

make build-cpp
make run-cpp
```

To compile all native implementations (the source of truth — no Python required):

```sh
make check
```

### Install and uninstall

`make install` performs a full, FHS-respecting system install of **both**
native implementations — binaries, man pages, and documentation:

| What | Installed to (under `PREFIX`, default `/usr/local`) |
| --- | --- |
| `bayeishuui-c`, `bayeishuui-cpp` | `bin/` |
| `bayeishuui-c.6`, `bayeishuui-cpp.6` | `share/man/man6/` |
| README, LICENSE, design & combat docs | `share/doc/bayeishuui/` |

```sh
sudo make install                 # install both to /usr/local
sudo make uninstall               # remove everything it installed

make install-c                    # install only the C implementation
make install-cpp                  # install only the C++ implementation

make install PREFIX=$HOME/.local  # install without root
make install DESTDIR=/tmp/pkg     # stage into a packaging root (DESTDIR)
```

After installing you can read the manuals with `man 6 bayeishuui-c` or
`man 6 bayeishuui-cpp`. `install-c`/`install-cpp` (and the matching
`uninstall-c`/`uninstall-cpp`) handle a single implementation plus its man
page. `PREFIX`, `BINDIR`, `MANDIR`, `DOCDIR`, and `DESTDIR` follow the usual
GNU conventions and can be overridden; `make clean` removes the local
`build/` directory and Python caches. Run `make help` for a target summary.

The Python files are optional companions kept on their own terms (see the
Python version section below). The native C and C++ builds are the source of
truth and require no Python.

Requirements for the native games: GCC or Clang, a Linux/POSIX terminal supporting ANSI escape sequences and UTF-8, and a terminal at least 60 columns by 24 rows. The graphics are rendered directly in the terminal; this is not hardware-accelerated 3D.

## Player 1 computer fighter

A tactical, engine-neutral Player 1 AI and input contract are provided in `source/bayeishuui_ai.py`. It chooses actions based on health, stamina, range, alignment, attack timing, and ground-game state.

```sh
make run-ai
# or make run-cpp for the same native game
```

The native C and C++ games offer a 21-second startup choice: double-tap **Space** to control Player 1 yourself; otherwise **Perfect Fighter** takes Player 1 automatically. The terminal displays live movement, attack, and defense messages. Press **Escape** to end an active Perfect Fighter session. At session end, a unique `.cfg` is written in the current working directory, named `RANDOMTOKEN_YYYYMMDD_DURATION_ACCOUNT.cfg` (four underscore-separated fields). `DURATION` uses `HHhMMmSSs`; the file records duration, result, mode, wins, losses, and account. Existing files are never overwritten.

The policy is a strong heuristic, not a mathematical guarantee of perfect play. The Python module is an optional engine-neutral API helper, not a live game launcher. See [Player 1 AI API](source/AI_PLAYER_API.md).

## Fighter Guild campaign

The native C and C++ games feature **15 ranked opponents and 17 fights per opponent** (255 scheduled bouts). Both fighters have HP bars. Ground Game unlocks after 150 cumulative striking damage (150% of starting 100 HP), with control, ground-and-pound, and submission interactions. Opponents rate you 0–100 after each 17-fight series; 95+ is superior. Sudden Death after a knockout requires at least 11 wins and a lead in the overall record.

The progression runs from **Initiate** through **Copper, Bronze, Silver, Gold, Platinum, Jade, Ruby, Sapphire, Emerald, Obsidian, Master, Grandmaster, Champion**, and finally **Guild Legend**. The opponent roster is documented in [source/README.md](source/README.md).

## SLeeLa character sheets

Fighters can be described in external **SLeeLa character sheets** — a `.sleela`
class file (`fighters/<name>.sleela`) whose body holds flat `type name = value;`
stat fields: identity (`name`, `rankTitle`, `rank`, `style`, `tactic`), vitals
(`health`, `stamina`), a striking profile (`reach`, `baseDamage`, `windupMs`,
`paceCentis`), behaviour tendencies (`aggression`, `lateral`, 0–100), and the
terminal sprite accent (`colorR/G/B`). Sample sheets live in `fighters/`.

The native C and C++ games read these through `source/fighter_sheet.h`, a shared
header providing a **reader → interpreter → disposer** lifecycle:

- **Reader** `bay_sheet_read(path)` loads the sheet file into a heap buffer.
- **Interpreter** `bay_fighter_from_source(text)` (or the convenience
  `bay_fighter_load(path, out)`) extracts the field initialisers into an
  in-game `BayFighter` struct. The sheet is treated as perfect input: unknown
  fields are ignored and missing fields keep engine defaults.
- **Disposer** `bay_sheet_free(buffer)` / `bay_fighter_dispose(fighter)` release
  the resources.

The reader does not execute SLeeLa; the stats are known constants, so it parses
the class body directly and stays dependency-free. Try the demos:

```sh
make run-sheet-c      # C reader/interpreter/disposer over the sample sheets
make run-sheet-cpp    # same, C++17
```

## Terminal sprite animation

The sprite engine authors **16 keyframe poses** per fighter and procedurally interpolates between them to produce a connected **244-step animation cycle** (indices 0–243), rendered directly as fixed-width ANSI/Unicode terminal sprites. The in-between frames are tweened at render time rather than stored as raster files. The shared engine adds fighter-specific stance silhouettes, color identity, and timing, with motion beats for Muay Thai strikes, footwork, recoil, ground control, ground-and-pound, and finishes. It is designed for the terminal rather than a raster or 3D renderer.

See [the sprite engine](source/bayeishuui_sprites.h) and [native source notes](source/README.md).

## Pixel-accurate rendering (next-generation terminals)

For terminals that support true pixel graphics, the native C and C++ games
render the fighters as an actual anti-aliased RGBA bitmap instead of glyph
cells — shaded head, round-capped limbs with real thickness, gloves, and a
ground shadow in the per-style colour. The backend is selected once at startup;
on a plain TTY or over SSH to a terminal without pixel support the games fall
back to the glyph sprite automatically, so existing behaviour is preserved. It rasterizes the **same 12-joint skeleton** the glyph
engine uses (via the shared `bay_sprite_skeleton()` in
[`bayeishuui_sprites.h`](source/bayeishuui_sprites.h)), so motion is identical
across backends. See [the pixel renderer](source/bayeishuui_pixels.h).

Transport is chosen automatically, in order of fidelity, and always degrades
cleanly:

1. **Kitty graphics protocol** — RGBA framebuffer, most accurate. Detected for
   kitty, WezTerm, and ghostty.
2. **Sixel** — bitmap escape sequence. Detected for foot, mlterm, contour,
   xterm (sixel), iTerm2, and similar.
3. **Glyph sprite** — the universal ANSI/Unicode fallback, for a plain TTY or
   SSH to a terminal without pixel support.

Override detection with the `BAYEISHUUI_RENDERER=kitty|sixel|glyph` environment
variable. No third-party libraries are used: the skeleton math is shared and
the base64/Sixel encoders are built in.

```sh
make demo                                   # build build/bayeishuui-pixeldemo
make run-demo                               # auto-detect and render a fighter

./build/bayeishuui-pixeldemo --style=4 --frame=42        # pick opponent + frame
./build/bayeishuui-pixeldemo --backend=kitty --frames=60 # animate in place
./build/bayeishuui-pixeldemo --style=2 --ppm=out.ppm     # dump exact pixels
```

The `--ppm=FILE` option writes the exact RGBA frame the terminal would receive
as a standard image, so the pixel output is verifiable even without a graphical
terminal.

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

## Python companions

The C and C++ builds drive the game; the Python files are preserved as optional
companions, reachable on their own and never required by the native build:

- `inventory/bayeshuui.py` — the original Python terminal prototype.
- `source/bayeishuui_ai.py` — the engine-neutral Player 1 AI reference module.

```sh
make run-py              # launch the original Python prototype
make run-ai-reference    # print the AI reference module's guidance

python3 inventory/bayeshuui.py --difficulty hard   # directly, on its own terms
```

`make check-py` is a friendly, always-successful nod to these files: for each
one it compares a recorded SHA-256 and, on a match, greets the file and lets the
module speak in its own voice. If a file changed, it says so gently (update its
`*_SHA256` in the `Makefile`); if `sha256sum` or `python3` is absent, it skips
quietly. It never fails the build.

## Repository layout

- `source/bayeishuui.c` — native C11 terminal game.
- `source/bayeishuui.cpp` — native C++17 terminal game.
- `source/bayeishuui_sprites.h` — shared terminal sprite engine.
- `source/bayeishuui_ai.py` — optional Python companion: engine-neutral Player 1 AI reference module.
- `source/AI_PLAYER_API.md` — AI state schema and engine integration contract.
- `source/fighter_sheet.h` — SLeeLa character sheet reader / interpreter / disposer (C11 + C++17).
- `source/fighter_sheet_demo.c` / `source/fighter_sheet_demo.cpp` — character sheet demo drivers.
- `fighters/*.sleela` — SLeeLa character sheets (sample fighter definitions).
- `source/README.md` — native build and renderer notes.
- `inventory/bayeshuui.py` — optional Python companion: original terminal prototype.
- `MARTIAL_ARTS_DESIGN.md` — design direction and future phases.
- `CONTROLS_AND_COMBAT.md` — input and combat contract.
- `Makefile` — build (`all`), run, AI, check, install, uninstall, and clean targets.

The game is a stylized terminal-rendered approximation rather than a true 3D renderer or a biomechanically validated fighting simulator.

## License

Eclipse Public License 2.0; see [LICENSE](LICENSE).
