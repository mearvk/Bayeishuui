# Player 1 Computer Fighter and Input API

The Player 1 tactical controller is `source/bayeishuui_ai.py`. It exposes a small
engine-neutral interface so the C, C++, or Python game can connect without making
the AI depend on terminal rendering.

## Launch the native Perfect Fighter

From the repository root:

```sh
make run-ai
# or make run-cpp
```

The native game opens a 21-second startup choice. Double-tap Space within 650 ms to play manually; otherwise Perfect Fighter controls Player 1. Escape ends an active AI session. At session end, a unique .cfg is written in the current working directory as RANDOMTOKEN_YYYYMMDD_DURATION_ACCOUNT.cfg; duration is formatted as HHhMMmSSs. The record includes result, control mode, wins, losses, and account. Existing files are never overwritten.

source/bayeishuui_ai.py remains the optional engine-neutral API module; running it directly prints usage guidance rather than launching a fight.

## Two separate policies

There are two distinct Perfect Fighter implementations, and they are **not** the
same code:

- `source/bayeishuui_ai.h` — the C/C++ header the native games actually run via
  `make run-ai` / `make run-cpp`. It decides from health, stamina, range, lateral
  alignment, cooldown, and the opponent's wind-up timer.
- `source/bayeishuui_ai.py` — an independent, engine-neutral **reference policy**
  exposed as `choose_action(state)`. It models extra signals (enemy stun/recovery,
  enemy guard, punish windows) and is intended for embedding in another engine or
  for offline evaluation. Nothing in the native loop calls it; running it directly
  just prints usage guidance.

The two policies share the same tactical intent (defend imminent attacks, fix
alignment and range, conserve stamina, establish ground control before finishing)
but may choose different actions in the same situation. Treat the Python module as
an alternate/reference controller, not a mirror of the native one.

## Engine bridge (Python reference policy)

Import `choose_action(state)` and call it once per decision tick. Supply a mapping
with these normalized values:

- `player_hp`, `enemy_hp`: health, 0–100.
- `player_stamina`, `enemy_stamina`: stamina, 0–100.
- `distance`: non-negative separation in the same units used by the engine.
- `lateral`: signed lateral offset; zero means aligned.
- `player_action`, `enemy_action`: action-state strings such as `ready`,
  `windup`, `active`, `recovery`, `guard`, or `stunned`.
- Optional booleans/numbers: `incoming_attack`, `enemy_guard`,
  `enemy_recovery`, `ground_unlocked`, `ground_mode`, `ground_control`,
  `player_stun`, `enemy_stun`.

The returned action is one of the existing gameplay inputs: `w`, `a`, `s`,
`d`, space, `g`, `r`, `e`, `j`, `k`, or `l`; an empty string means wait.
The bridge must clamp/validate state values, reject actions unsupported by the
current game mode, and map one AI decision to one engine tick. Never allow the AI
to bypass cooldowns, reach checks, stamina costs, or combat rules.

## Tactical policy

The heuristic prioritizes defense against imminent attacks, alignment and range,
stamina conservation, punish windows after stun/recovery, and ground control before
ground-and-pound or submission attempts. It is a strong baseline, **not a guarantee
of perfect play**: the game has stochastic opponent behavior and no formal proof
that any policy is optimal. A truly optimal controller requires a complete engine
state/action model and repeated simulation-based evaluation.

## Native terminal behavior

The native C and C++ loops use `source/bayeishuui_ai.h` for tactical decisions (not the Python reference policy above). At startup, a 21-second choice window lets the user double-tap Space (two taps within 650 ms) to choose manual Player 1; otherwise Perfect Fighter takes control. Escape ends an active AI session. The terminal renders the fight and updates movement, attack, and defense messages in real time.

At session end, a `.cfg` is written in the immediate working directory with filename shape `RANDOMTOKEN_YYYYMMDD_DURATION_ACCOUNT.cfg`. Duration is formatted as `HHhMMmSSs`; the record includes duration in seconds, result, mode, wins, losses, and account. Exclusive file creation prevents overwrites. The controller is a strong heuristic, not a mathematical proof of perfect play.
