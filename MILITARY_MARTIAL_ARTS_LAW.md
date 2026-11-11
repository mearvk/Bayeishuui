# Military Martial Arts Law (MMAL)

Bayeishuui now fights under **Military Martial Arts Law**: joints can be broken,
and the bout **fights on**. The rules live in one header-only module,
[`source/bayeishuui_law.h`](source/bayeishuui_law.h), shared by the C11 and C++17
games. The logic is pure and deterministic (random rolls are passed in), so it is
testable apart from rendering and input.

## The Articles

| Article | Rule |
| --- | --- |
| 1 | A fighter may attempt an **arm lock** (`B`) at clinch range (gap ≤ 2.0 m), or from ground control 2+. A successful lock breaks one arm. |
| 2 | A broken arm **never stops the bout**. The injured fighter fights on. |
| 3 | Each arm breaks once per bout. Both arms can be lost. |
| 4 | Injury has a price (below). Legs are unharmed, so teeps and round kicks stay at full strength. |
| 5 | Between bouts the corner sets the arms: injuries clear. Sudden Death keeps the injuries you carry. |

## Effects of a broken arm

| Arms broken | Punch / elbow / ground-and-pound | Guard leak | Rival attack tempo |
| --- | --- | --- | --- |
| 0 | 100% | 1x | 1.0x |
| 1 | 60% | 2x | 1.3x slower (injured rival) |
| 2 | 35%; **Space is blocked** (kick only) | 3x | 1.7x slower (injured rival) |

The bone-break itself deals 14 HP to the victim (7 HP when a rival breaks yours).

## Attempting a break (`B`)

- Costs 20 stamina and a 1.0 s recovery, win or lose.
- Success chance: standing `34 - rank`%, on the ground `40 + 8*control - rank`%,
  +10% against a tired rival (stamina < 30), clamped to 12–95%.
- A failed lock slips free and leaves you spent, so choose the moment.

## Rivals break arms too

From rank 3 up, a landed **unguarded** rival strike can also break one of your
arms: `1 + rank*2/3`% (+6% for the rank 7 Clinch Controller, +6% if your stamina
is below 25, max 20%). Guarding prevents it. Early opponents (ranks 1–2) never do.

## Seeing it

- The sprite hangs the broken arm limp from the shoulder. Both the glyph and the
  Kitty/Sixel pixel renderers use the same skeleton, so the injury shows everywhere.
- Row `h-2` shows `MMAL ARMS  YOU L/R  RIVAL L/R` with `OK` / `BROKEN`.
- At the end of a run the game prints the arms broken and taken.
- Preview a pose: `./build/bayeishuui-pixeldemo --player --broken=1|2|3 --ppm=out.ppm`
  (1 left, 2 right, 3 both).

## Perfect Fighter

The auto-pilot uses the law: it closes to clinch range when healthy and rested,
attempts locks when the rival is not winding up a strike (so its defensive
priorities are unchanged), and switches to kicks when both its own arms are broken.
See `bay_law_ai_action()` in the header.
