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

## Guest fighter: Joe Biden (White Belt wildcard)

A parody guest fighter whose rules are layered on the Articles above. They live
in [`source/bayeishuui_joe.h`](source/bayeishuui_joe.h) and the character sheet
[`fighters/joe_biden.sleela`](fighters/joe_biden.sleela). Everything below applies
only in a bout where Joe is the rival.

| Rule | Detail |
| --- | --- |
| Entry | Each bout has a **1 in 4** chance that Joe takes the rival slot in place of that bout's Guild opponent. He fights with White Belt (rank 1) tactics, and your arm-lock odds use rank 1. The campaign record and rank progression are unchanged. |
| Hit points | **1500 HP** (Guild fighters have 100). The rival bar is still a percentage; the HUD adds `JOE n/1500 HP`. Every point of damage you deal is converted at 100/1500. |
| Snapped arm | An arm break costs Joe **750 HP** instead of 14. |
| Cigarette | After his first arm snap in a bout he smokes for **1d6 + 8 seconds** (9–14 s). One cigarette per bout. |
| Falling over | When the cigarette burns out there is a **50%** chance he falls: the bout enters the ground game with control already at 2 (`J` control, `K` ground-and-pound, `L` submission, `B` arm lock). |
| Speech bubbles | Arm snaps show a line such as "Ow, You owe me Meanies!". After he falls there is a **1 in 4** chance of "You beftta!". Bubbles last 3.5 s and are drawn beside him. |
| Hat | Red cowboy hat until his **first arm snap, ever**. From then on it is **white for the rest of his career**; the flag is saved in the profile (`joe_white_hat=1`). |
| Size | **14% taller** than every other fighter (glyph and pixel renderers). White skin, white belt, aviators. |

### The Deal for Joe (`zz`)

1. Type **`zz`** (two `z` presses within 0.65 s) **within the first 2 seconds** of
   a Joe bout. Later, outside a Joe bout, or a second time in the same bout, the
   game only shows a refusal message.
2. The bout pauses and the game offers **Ribs and Sauce or Steak and Potatoes** if
   you agree that Joe is main and kosher. Answer **`y`** or **`n`**; there is no
   time limit on the answer.
3. Either way Joe's arm is fractured and he loses the usual **750 HP**, even if an
   arm was already snapped (if both arms are already broken, only the HP is
   lost). The fracture also lights the cigarette and turns the hat white, as for
   any arm snap.
4. On **`n`** he also loses **900 HP**. A full-health Joe has 1500 HP, so 750 + 900
   knocks him out and ends the bout.
5. Joe shouts "Bark" from his dialog bubble, then it is back to fighting.

The Deal works in manual play and in Perfect Fighter mode (you type the `zz`; the
auto-pilot never opens it on its own).

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
