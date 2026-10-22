#!/usr/bin/env python3
"""Bayeishuui Player 1 tactical AI and engine-input adapter.

This module is deliberately engine-agnostic: the game supplies a state mapping
and consumes one action from choose_action(). It never writes outside the
process working directory unless the caller explicitly changes directories.
"""
from __future__ import annotations

import getpass
import json
import math
import os
import secrets
import string
from datetime import datetime
from pathlib import Path
from typing import Any, Mapping

ACTIONS = ("", "w", "a", "s", "d", " ", "g", "r", "e", "j", "k", "l", "q")


def _number(state: Mapping[str, Any], key: str, default: float) -> float:
    try:
        value = float(state.get(key, default))
        return value if math.isfinite(value) else default
    except (TypeError, ValueError):
        return default


def _ready(state: Mapping[str, Any], prefix: str) -> bool:
    return str(state.get(f"{prefix}_action", "ready")).lower() in {
        "ready", "guard", "approach"
    }


def choose_action(state: Mapping[str, Any]) -> str:
    """Choose a strong, deterministic tactical action from a normalized state.

    Required normalized fields: player_hp, enemy_hp, player_stamina,
    enemy_stamina, distance. Optional fields: lateral, player_action,
    enemy_action, player_stun, enemy_stun, ground_unlocked, ground_mode,
    ground_control, incoming_attack, enemy_guard, enemy_recovery.
    This is a strong heuristic, not a claim of mathematically perfect play.
    """
    hp = max(0.0, min(100.0, _number(state, "player_hp", 100)))
    enemy_hp = max(0.0, min(100.0, _number(state, "enemy_hp", 100)))
    stamina = max(0.0, min(100.0, _number(state, "player_stamina", 100)))
    distance = max(0.0, _number(state, "distance", 3.0))
    lateral = abs(_number(state, "lateral", 0))
    player_stun = _number(state, "player_stun", 0)
    enemy_stun = _number(state, "enemy_stun", 0)
    ground = bool(state.get("ground_mode", False))
    unlocked = bool(state.get("ground_unlocked", False))
    control = int(max(0, _number(state, "ground_control", 0)))
    incoming = bool(state.get("incoming_attack", False))
    enemy_guard = bool(state.get("enemy_guard", False))
    recovery = bool(state.get("enemy_recovery", False))

    if hp <= 0 or enemy_hp <= 0:
        return ""
    if player_stun > 0 or not _ready(state, "player"):
        return ""

    # Defensive timing beats trading while the opponent's attack is imminent.
    if incoming and not recovery:
        return "g" if stamina >= 4 else "s"

    if ground and unlocked:
        if control < 2:
            return "j"
        if stamina >= 10:
            return "k" if enemy_hp > 18 or enemy_guard else "l"
        return "g"

    if stamina < 18:
        return "g" if distance <= 2.4 else "s"

    # Correct alignment and select the best available tool for the range.
    if lateral > 2:
        return "a" if _number(state, "lateral", 0) > 0 else "d"
    if distance > 3.0:
        return "w"
    if enemy_stun > 0 or recovery:
        if distance <= 2.25 and stamina >= 13:
            return " "
        if distance <= 2.7 and stamina >= 19:
            return "e"
        if distance <= 3.15 and stamina >= 10:
            return "r"
    if enemy_guard:
        # Use the low-cost teep to probe range; avoid exhausting on a covered target.
        return "r" if distance <= 3.15 and stamina >= 10 else "s"
    if distance > 2.7:
        return "r" if stamina >= 10 else "w"
    if distance > 2.25:
        return "e" if stamina >= 19 else "r"
    return " " if stamina >= 13 else "g"


def create_run_config(
    directory: str | os.PathLike[str] = ".",
    duration_seconds: float = 0,
    result: str = "session-ended",
) -> Path:
    """Create a uniquely named fight record after a caller knows elapsed duration."""
    root = Path(directory).resolve()
    root.mkdir(parents=True, exist_ok=True)
    elapsed = max(0, int(duration_seconds))
    hours, remainder = divmod(elapsed, 3600)
    minutes, seconds = divmod(remainder, 60)
    duration_tag = f"{hours}h{minutes:02d}m{seconds:02d}s"
    date = datetime.now().astimezone().strftime("%Y%m%d")
    try:
        account = getpass.getuser()
    except Exception:
        account = "unknown"
    safe_account = "".join(c if c.isalnum() or c in "-." else "-" for c in account)
    safe_account = safe_account.strip("-.")[:40] or "unknown"
    alphabet = string.ascii_uppercase + string.digits
    for _ in range(20):
        token = "".join(secrets.choice(alphabet) for _ in range(8))
        path = root / f"{token}_{date}_{duration_tag}_{safe_account}.cfg"
        try:
            # Exclusive creation prevents accidental overwrites even on a collision.
            with path.open("x", encoding="utf-8") as handle:
                json.dump({
                    "format": "bayeishuui-ai-config-v1",
                    "created_at": datetime.now().astimezone().isoformat(timespec="seconds"),
                    "run_date": date,
                    "duration": duration_tag,
                    "duration_seconds": elapsed,
                    "result": result,
                    "account": safe_account,
                    "player": 1,
                    "controller": "bayeishuui_ai",
                    "strategy": "range-stamina-defense-ground-game",
                    "api": {
                        "input": "choose_action(state: Mapping[str, Any]) -> str",
                        "actions": ["w", "a", "s", "d", " ", "g", "r", "e", "j", "k", "l"],
                        "state_fields": [
                            "player_hp", "enemy_hp", "player_stamina", "enemy_stamina",
                            "distance", "lateral", "player_action", "enemy_action",
                            "player_stun", "enemy_stun", "ground_unlocked", "ground_mode",
                            "ground_control", "incoming_attack", "enemy_guard", "enemy_recovery"
                        ]
                    },
                    "notes": "Heuristic AI; engine adapter must map normalized state and validate actions."
                }, handle, indent=2)
                handle.write("\n")
            return path
        except FileExistsError:
            continue
    raise RuntimeError("Could not allocate a unique run configuration filename")


def main() -> int:
    print("Bayeishuui Player 1 AI API module.")
    print("Launch the live Perfect Fighter with: make run-ai")
    print("Or use make run-cpp for the same native game.")
    print("Import choose_action(state) to use the optional engine-neutral API.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
