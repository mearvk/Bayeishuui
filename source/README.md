# Native terminal game source

This directory contains two standalone implementations of Bayeishuui: bayeishuui.c (C11) and bayeishuui.cpp (C++17). Both use POSIX terminal input and ANSI true-colour output, with no graphics framework or third-party runtime dependency.

## Fighter Guild campaign

The campaign has 15 ranked opponents and 17 bouts per opponent (255 scheduled bouts). Both fighters have visible HP bars. After 150 cumulative striking damage against the current opponent (150% of 100 starting HP), Ground Game unlocks: J builds control, K uses ground-and-pound, and L attempts a submission. Opponents rate the player from 0–100 after each series; 95+ is superior.

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

## Branded terminal sprite system

The native renderer now uses `source/bayeishuui_sprites.h`, a terminal-native sprite engine with a **244-frame consecutive cycle per combatant** (frame indices `0–243`). The player and all 15 Fighter Guild opponents use the same frame contract, for **3,904 indexed combatant frames** across the 16 fighter identities.

Rather than bundling thousands of redundant bitmap files, each frame is rendered as a fixed-width ANSI/Unicode terminal sprite. The sequence uses connected stance and transition beats covering guard, footwork, straight punches, teep chamber, round kicks, knees, elbows, slips/counters, impact, recoil, ground control, ground-and-pound, and finishing poses. Each opponent receives a signature palette and stance/animation timing tied to their Muay Thai identity. The visual language uses Bayeishuui's own deep-navy arena, cyan player identity, and warm opponent accents.

The frames are consecutive and terminal-friendly; these are authored text-sprite poses and procedural transition frames, not 3,904 separate raster images.

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

The renderer draws a perspective dojo floor, coloured fighter silhouettes, health and stamina meters, range feedback, and combat messages. Controls: **W** advance, **S** retreat, **A/D** circle, **Space** punch, **R** teep, **E** round kick, **G** guard, **J/K/L** ground game, **Q** quit. After each 17-bout series, the opponent classifies the player as Striker, Finisher, or Balanced Striker / Finisher. Sudden Death after a knockout requires at least 11 wins and a lead in the overall record. Run in a Linux terminal at least 60 columns by 24 rows. The presentation is terminal graphics (ANSI/Unicode), not hardware-accelerated 3D.

Compiler requirements: a C11 compiler such as GCC or Clang for the C version; a C++17 compiler for the C++ version.
