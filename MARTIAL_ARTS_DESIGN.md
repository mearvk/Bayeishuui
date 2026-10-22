# Bayeishuui Martial-Arts Game Design

## Vision

Bayeishuui is a striking-focused, one-on-one martial-arts game. Its signature composition is an over-the-shoulder third-person view: the player's superior fighter is visible from behind in the foreground, facing an opponent whose stance and intent can be read clearly.

The gameplay should reward composure, footwork, distance management, timing, and tactical choices rather than button mashing.

## Camera and composition

- Place the player fighter in the lower foreground, viewed from behind and slightly above shoulder height.
- Keep the opponent visible ahead, generally facing the player.
- Track the action smoothly while preserving enough arena context to judge distance and lateral movement.
- Keep both fighters' feet and the contact area visible whenever possible.
- Avoid camera motion that obscures an incoming strike or makes left/right movement ambiguous.
- For a first graphical prototype, use a simple arena and placeholder fighter models before investing in detailed art.

## Core combat loop

1. Read the opponent's stance and distance.
2. Advance, retreat, or circle to establish a useful position.
3. Commit to a strike with Space.
4. Resolve whether the strike connects based on range, facing, and attack timing.
5. Let either fighter recover, reposition, guard through AI behavior, or counter.
6. End the match when one fighter's health reaches zero.

## Strike lifecycle

A strike should have three explicit phases:

- **Wind-up:** communicate intent; the strike cannot deal damage yet.
- **Active:** test range, facing, and target vulnerability; resolve one hit at most unless a move explicitly says otherwise.
- **Recovery:** temporarily limit repeat attacks so timing and missed strikes matter.

A strike should not hit through arbitrary distance or behind the attacking fighter. Hit detection should be deterministic and testable independently of animation.

## Opponent behavior

The first opponent AI should use a small, understandable state machine:

- **Approach** when too far away.
- **Circle or reposition** when alignment is poor.
- **Attack** when within range and an opening is available.
- **Recover** after attacking.
- **Respond** to player movement and punish obvious over-commitment, without perfect reaction or impossible tracking.

Difficulty should tune reaction delay, attack selection, and recovery windows—not secretly change hit outcomes.

## Player state

Track at minimum:

- Health for both fighters.
- Current movement direction and facing.
- Current action phase and remaining phase time.
- Strike cooldown/recovery.
- Match result and reset state.

Stamina may be added after the basic exchange is fun and reliable. Avoid adding complex meters before movement and strike feedback are solid.

## Presentation and feedback

- Use a clear arena floor and stable horizon.
- Distinguish idle, movement, wind-up, impact, and recovery.
- Provide restrained hit feedback (brief animation reaction, sound, and/or a short impact cue).
- Keep health and match status readable without covering the fighters.
- Include a title/menu screen and a quick rematch path once the prototype supports a graphical window.

## Technical phases

### Phase 1 — Combat model

Implement a small, testable combat simulation with positions, facing, health, strike phases, range checks, and a basic opponent state machine. The five-button contract is defined in `CONTROLS_AND_COMBAT.md`.

### Phase 2 — Visual prototype

Choose a suitable graphics engine and build the arena, third-person follow camera, placeholder player fighter, and opponent. Connect rendering to the combat model rather than embedding combat rules inside animation code.

### Phase 3 — Feel and polish

Tune movement acceleration, turn rate, camera damping, strike wind-up/recovery, hit reactions, audio, and opponent difficulty. Test edge cases such as simultaneous attacks, attacks at maximum range, and striking while turning.

### Phase 4 — Content

Add distinct martial artists, styles, arenas, and move variations only after the core duel is readable, responsive, and repeatable.

## Scope boundary

`inventory/bayeshuui.py` is the original terminal dungeon-adventure prototype. It is retained for reference and remains separate from the planned real-time martial-arts game. This document describes the intended design; it does not claim the third-person renderer or combat system has already been implemented.
