# Bayeishuui — Martial Arts Combat

Bayeishuui is a Linux-terminal martial-arts duel focused on striking, footwork, range, stamina, and reading an opponent's behavior. The terminal renderer uses perspective floor lines and stylized fighter silhouettes to suggest an over-the-shoulder view: the player's fighter is in the foreground, viewed from behind, while the opponent faces them ahead.

## Run

Requirements: Python 3.10+ and an interactive Linux/POSIX terminal at least 60 columns by 25 rows. No third-party Python packages are required.

```sh
make check
make run
```

Optional difficulty settings:

```sh
python3 inventory/bayeshuui.py --difficulty easy
python3 inventory/bayeshuui.py --difficulty normal
python3 inventory/bayeshuui.py --difficulty hard
``

## Controls

| Key | Action |
| --- | --- |
| W | Advance toward the opponent |
| A | Circle left |
| S | Retreat |
| D | Circle right |
| Space | Strike |
| Q | Quit the bout |

## Combat

- Strike range and lateral alignment affect whether a punch connects.
- Strikes have wind-up, active impact, and recovery phases.
- Clean hits reduce health and briefly stun the target.
- Guarding reduces damage; stamina regenerates more quickly when resting.
- The opponent approaches, attacks, guards, and recovers based on difficulty.

## Repository layout

- `inventory/bayeshuui.py` — playable Python terminal fighting game.
- `inventory/README.md` — inventory game notes.
- `MARTIAL_ARTS_DESIGN.md` — design direction and future phases.
- `CONTROLS_AND_COMBAT.md` — input and combat contract.
- `Makefile` — `make run` and `make check`.

The terminal game is intentionally a stylized approximation, not a true 3D renderer or a biomechanically validated fighting simulator. It is the playable first step toward the over-the-shoulder perspective.

## License

Eclipse Public License 2.0; see [LICENSE](LICENSE).
