# Bayeishuui — Martial Arts Combat

Bayeishuui is evolving into a martial-arts striking game built around deliberate movement, spacing, timing, and readable opponent behavior. The intended presentation is an over-the-shoulder, third-person view: the player's superior fighter is seen from behind while the opponent faces them across the arena.

## Repository layout

- `inventory/bayeshuui.py` — preserved, dependency-free Python terminal adventure from the project's initial prototype.
- `MARTIAL_ARTS_DESIGN.md` — core vision, perspective, combat loop, and development phases.
- `CONTROLS_AND_COMBAT.md` — the five-button input contract and combat rules.
- `Makefile` — convenience commands for the legacy terminal game and basic Python syntax checks.

The original terminal game is retained as a separate inventory item; it is not yet the third-person fighting implementation.

## Current prototype: terminal adventure

Requirements: Python 3.10+ and a Linux/POSIX-compatible interactive terminal. No third-party packages are required.

```sh
make check
make run
```

Run the inventory game directly with `python3 inventory/bayeshuui.py`.

## Planned fighting-game controls

| Key | Action |
| --- | --- |
| W | Advance toward the opponent |
| A | Move/circle left |
| S | Retreat |
| D | Move/circle right |
| Space | Strike |

The target design keeps these five inputs as the primary controls. The first playable combat prototype should implement movement, facing, strike range, wind-up/recovery, health, and a basic opponent AI before expanding the move set.

## Development status

The martial-arts direction and control contract are documented; a real-time graphical renderer, third-person camera, animated fighters, and fighting-game loop still need implementation. See the design documents before choosing a graphics engine.

## License

This project is distributed under the Eclipse Public License 2.0; see [LICENSE](LICENSE).
