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

## Muay Thai foundation and individual styles

Muay Thai is the core ruleset: a balanced guard, range management, teep (push kick), round kick, straight punches, knees, and close-range elbows. The terminal controls include **Space** for straight punches/close-range strikes, **R** for a long-range teep, and **E** for a heavier round kick. These are simplified game mechanics, not a detailed anatomical simulation.

Each Fighter Guild opponent has a tactical identity shown in the HUD. Their behavior varies through pressure and closing distance, long-guard spacing, counter timing, lateral movement, clinch-range pressure, or body-kick attrition. Higher ranks combine tactics and attack more quickly.

| Rank | Fighter | Style |
|---:|---|---|
| 1 | Milo Quickstep | Muay Thai Fundamentals |
| 2 | Jax Ironhand | Pressure Muay Mat |
| 3 | Nia Swift | Long-Guard Muay Femur |
| 4 | Bram Stone | Counter-Kicking Muay Tae |
| 5 | Kira Storm | Knee-Forward Muay Khao |
| 6 | Dante Cross | Elbow Specialist Muay Sok |
| 7 | Sora Blade | Clinch Controller |
| 8 | Viktor Voss | Southpaw Angle Fighter |
| 9 | Mei Dragon | Teep-and-Reset Tactician |
| 10 | Orion Vale | High-Guard Pressure Fighter |
| 11 | Zara Night | Switch-Kick Specialist |
| 12 | Cassian Wolf | Counter-Elbow Technician |
| 13 | Rhea Titan | Body-Kick Attrition Fighter |
| 14 | Kael Thunder | Advanced Muay Femur |
| 15 | The First Fang | Complete Nak Muay |

## Build and run from the repository root

```sh
make build-c
make run-c

make build-cpp
make run-cpp
```

The renderer draws a perspective dojo floor, coloured fighter silhouettes, health and stamina meters, range feedback, and combat messages. Controls: **W** advance, **S** retreat, **A/D** circle, **Space** strike, **G** guard, **Q** quit. Run in a Linux terminal at least 60 columns by 24 rows. The presentation is terminal graphics (ANSI/Unicode), not hardware-accelerated 3D.

Compiler requirements: a C11 compiler such as GCC or Clang for the C version; a C++17 compiler for the C++ version.
