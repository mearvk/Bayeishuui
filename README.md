# Bayeishuui — The Hollow Gate

A small, turn-based terminal adventure for Linux. Explore a procedurally carved cave, fight cave wardens, recover the golden relic, and escape through the gate across three levels.

## Requirements

- Linux or another POSIX-like system with an ANSI-compatible terminal
- Python 3.10+
- No third-party packages

## Run

```sh
chmod +x bayeshuui.py
./bayeshuui.py
```

Or use `python3 bayeshuui.py`. The game uses terminal raw/cbreak input, so run it in an interactive terminal rather than piping input into it.

## Controls

| Key | Action |
| --- | --- |
| W / A / S / D | Move north / west / south / east |
| Arrow keys | Move |
| F | Attack an adjacent cave warden |
| P | Drink a healing potion |
| Q | Quit |

## Goal

Collect the golden relic (`*`) on each level, then reach the gate (`>`). Survive all three levels to win. Wardens pursue you and attack when adjacent. Potions restore up to 5 health points.

## Map symbols

- `@` — player
- `#` — stone wall
- `.` — cave floor
- `g` — cave warden
- `!` — potion (reserved for future map placements)
- `*` — golden relic
- `>` — exit gate

## Development

Run the basic checks with:

```sh
python3 -m py_compile bayeshuui.py
make check
```

The game is dependency-free and intended as a simple, extensible starting point. Contributions remain subject to the repository's Eclipse Public License 2.0; see [LICENSE](LICENSE).
