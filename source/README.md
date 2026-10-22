# Native terminal game source

This directory contains two standalone implementations of Bayeishuui: bayeishuui.c (C11) and bayeishuui.cpp (C++17). Both use POSIX terminal input and ANSI true-colour output, with no graphics framework or third-party runtime dependency.

## Fighter Guild campaign

The native game has a 15-opponent campaign. The HUD shows the current Fighter Guild rank, opponent, and distance. Each opponent becomes more aggressive and hits harder as the player advances. Defeating an opponent promotes the player to the next rank, restores stamina, and grants a limited health recovery. Health otherwise carries between bouts; losing ends the campaign.

| Rank | Opponent |
|---|---|
| 1 — Initiate | Milo Quickstep |
| 2 — Copper | Jax Ironhand |
| 3 — Bronze | Nia Swift |
| 4 — Silver | Bram Stone |
| 5 — Gold | Kira Storm |
| 6 — Platinum | Dante Cross |
| 7 — Jade | Sora Blade |
| 8 — Ruby | Viktor Voss |
| 9 — Sapphire | Mei Dragon |
| 10 — Emerald | Orion Vale |
| 11 — Obsidian | Zara Night |
| 12 — Master | Cassian Wolf |
| 13 — Grandmaster | Rhea Titan |
| 14 — Champion | Kael Thunder |
| 15 — Guild Legend | The First Fang |

## Build and run from the repository root

```sh
make build-c
make run-c

make build-cpp
make run-cpp
```

The renderer draws a perspective dojo floor, coloured fighter silhouettes, health and stamina meters, range feedback, and combat messages. Controls: **W** advance, **S** retreat, **A/D** circle, **Space** strike, **G** guard, **Q** quit. Run in a Linux terminal at least 60 columns by 24 rows. The presentation is terminal graphics (ANSI/Unicode), not hardware-accelerated 3D.

Compiler requirements: a C11 compiler such as GCC or Clang for the C version; a C++17 compiler for the C++ version.
