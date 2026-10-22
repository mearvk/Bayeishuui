# Controls and Combat Contract

## Primary controls

| Input | Meaning | Expected behavior |
| --- | --- | --- |
| W | Advance | Move toward the opponent along the arena's forward axis |
| A | Left | Move or circle to the player's left |
| S | Retreat | Move away from the opponent |
| D | Right | Move or circle to the player's right |
| Space | Strike | Start the currently selected basic strike if the fighter can act |

These are the five primary gameplay inputs. Menus may use mouse or standard UI navigation, but the core duel should not require extra combat buttons.

## Coordinate and facing rules

- The player has a position on the arena floor and a facing direction.
- W/S change distance relative to the opponent; A/D provide lateral footwork.
- The camera follows from behind the player, but camera movement must not reverse the meaning of A and D.
- Fighters should turn toward each other smoothly, with a configurable turn rate.
- The opponent should not snap instantly to face the player during an attack.

## Space: strike

A Space press initiates a strike only when the player is not already in a disallowed action phase. The initial move is a straightforward forward punch.

A hit is valid only when all required conditions hold:
1. The strike is in its active phase.
2. The opponent is within the strike's reach.
3. The attacker's facing direction is sufficiently aligned with the opponent.
4. The target is not invulnerable or already hit by that same strike instance.

Missed strikes still incur recovery. This creates a meaningful tradeoff between attacking and maintaining position.

## Movement and attack interactions

- Movement remains responsive during ordinary footwork.
- During strike wind-up and recovery, movement may be reduced but should not feel randomly disabled.
- The first prototype should favor predictable rules over elaborate combos.
- Simultaneous attacks must resolve consistently using a documented update order.
- Match reset returns health, positions, action phases, and opponent state to known starting values.

## Minimum acceptance checks

- Each of W/A/S/D changes position in its intended direction.
- Space initiates one strike, not repeated damage every frame while held.
- A strike out of range misses.
- A strike facing away from the opponent misses.
- A strike cannot deal damage during wind-up or recovery.
- A successful strike changes opponent health and produces visible feedback.
- The opponent can approach and attack without moving through arena boundaries.
- A fighter reaching zero health ends the round exactly once.
