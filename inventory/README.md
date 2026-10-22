# Inventory: Terminal Martial-Arts Duel

This is the current playable Python game. It replaces the original cave-adventure prototype with a real-time terminal martial-arts bout using five gameplay buttons.

## Run from the repository root

```sh
make run
```

Or launch directly:

```sh
python3 inventory/bayeshuui.py --difficulty normal
```

## Requirements

- Python 3.10+
- Linux/POSIX interactive terminal, at least 60 columns by 25 rows
- Standard library only

## Controls

- W: advance toward the opponent
- A / D: circle left / right
- S: retreat
- Space: strike
- Q: quit

The renderer uses a perspective dojo floor and foreground/back-view player silhouette with an opponent ahead. Combat includes range checks, wind-up and recovery, health, stamina, guarding, stun, and three AI difficulty settings. The view is a terminal-art approximation of the intended third-person camera, not actual 3D graphics.
