# Controls and Combat Contract

## Primary controls

| Input | Meaning | Expected behavior |
| --- | --- | --- |
| W | Advance | Close distance toward the opponent |
| A | Circle left | Shift laterally to the player's left |
| S | Retreat | Increase distance from the opponent |
| D | Circle right | Shift laterally to the player's right |
| Space | Strike | Initiate one straight punch when the fighter can act |
| Q | Quit | Leave the current bout |

W/A/S/D and Space are the five primary gameplay controls. Q is a utility command to exit the terminal game, not a combat action.

## Combat rules

- A fighter tracks health, stamina, facing, action state, wind-up/recovery time, and stun.
- A strike only connects when its active impact resolves, the target is close enough, and lateral alignment is within the attack window.
- A missed strike still incurs recovery and consumes stamina.
- A clean hit damages and briefly stuns the target.
- Guarding reduces incoming damage at the cost of stamina.
- Stamina recovers faster while fighters are ready, guarding, or repositioning.
- The opponent approaches when distant and may strike or guard when close.
- Difficulty changes the opponent's decision interval and attack tendency, not damage outcomes after a hit is determined.
- Arena lateral movement is bounded; forward distance is clamped to prevent overlapping fighters.

## Minimum acceptance checks

- W reduces separation; S increases it.
- A and D change lateral alignment in opposite directions.
- Space cannot start another strike during wind-up, active, recovery, or stun.
- Out-of-range and poorly aligned strikes miss.
- A clean strike reduces the target's health and applies a brief stun.
- Guarding reduces damage.
- Zero health ends the bout once and reports victory or defeat.
- The terminal cursor and input mode are restored when the bout ends or the user quits.
