#!/usr/bin/env python3
"""Bayeishuui — a real-time, terminal-based martial-arts duel for Linux."""

from __future__ import annotations

import argparse
import math
import os
import random
import select
import shutil
import sys
import termios
import time
import tty
from dataclasses import dataclass

RESET = "\033[0m"
BOLD = "\033[1m"
DIM = "\033[2m"
CYAN = "\033[96m"
RED = "\033[91m"
GREEN = "\033[92m"
YELLOW = "\033[93m"
WHITE = "\033[97m"
BLUE = "\033[94m"
CLEAR = "\033[2J\033[H"
HIDE_CURSOR = "\033[?25l"
SHOW_CURSOR = "\033[?25h"

TICK = 0.075
ARENA_X = 4.2
START_DISTANCE = 3.2


@dataclass
class Fighter:
    name: str
    x: float
    distance: float
    health: float = 100.0
    stamina: float = 100.0
    facing: int = 1
    action: str = "ready"
    action_time: float = 0.0
    cooldown: float = 0.0
    stun: float = 0.0
    landed: bool = False
    guard: float = 0.0


class MartialArtsGame:
    """Small real-time combat simulation with readable, frame-independent rules."""

    def __init__(self, difficulty: str = "normal", seed: int | None = None):
        self.rng = random.Random(seed)
        self.difficulty = difficulty
        self.player = Fighter("YOU", 0.0, 0.0, facing=1)
        self.enemy = Fighter("DOJO OPPONENT", 0.5, START_DISTANCE, facing=-1)
        self.player.facing = self._direction_to(self.player, self.enemy)
        self.enemy.facing = self._direction_to(self.enemy, self.player)
        self.elapsed = 0.0
        self.message = "Read the stance. Manage distance. Strike when the opening appears."
        self.running = True
        self.result = ""
        self.ai_think = 0.8
        self.player_input = ""
        self.last_damage = 0.0
        self.hit_flash = 0.0

    @staticmethod
    def _direction_to(source: Fighter, target: Fighter) -> int:
        return 1 if target.x >= source.x else -1

    @staticmethod
    def _distance(a: Fighter, b: Fighter) -> float:
        return math.hypot(a.x - b.x, a.distance - b.distance)

    def _face_each_other(self) -> None:
        self.player.facing = self._direction_to(self.player, self.enemy)
        self.enemy.facing = self._direction_to(self.enemy, self.player)

    def _start_strike(self, fighter: Fighter, target: Fighter, is_player: bool) -> None:
        if fighter.health <= 0 or fighter.stun > 0 or fighter.cooldown > 0:
            return
        if fighter.action in ("windup", "active", "recovery"):
            return
        if fighter.stamina < 13:
            if is_player:
                self.message = "You're winded. Move and recover your stamina."
            return
        fighter.stamina -= 13
        fighter.action = "windup"
        fighter.action_time = 0.19 if is_player else self.rng.uniform(0.20, 0.30)
        fighter.landed = False
        fighter.guard = 0.0
        if is_player:
            self.message = "You commit to a straight punch — stay in range."

    def _resolve_strike(self, attacker: Fighter, defender: Fighter, is_player: bool) -> None:
        self._face_each_other()
        separation = self._distance(attacker, defender)
        lateral = abs(attacker.x - defender.x)
        reach = 1.28
        aligned = lateral <= 0.92 and separation <= reach
        defender_guarding = defender.action == "guard" and defender.guard > 0
        if not aligned:
            if is_player:
                self.message = "Your punch cuts through empty space. The opponent is out of range."
            else:
                self.message = "The opponent's strike misses. A brief opening!"
            return
        if defender_guarding:
            damage = self.rng.uniform(2.0, 5.0)
            defender.stamina = max(0.0, defender.stamina - 7.0)
            if is_player:
                self.message = "Your strike meets a tight guard. Little damage gets through."
            else:
                self.message = "You block most of the blow, but the impact costs stamina."
        else:
            base = self.rng.uniform(8.0, 13.0)
            # Good distance control rewards clean contact without one-hit knockouts.
            distance_bonus = max(0.0, 1.0 - abs(separation - 0.78) / 0.78)
            damage = base * (0.88 + 0.22 * distance_bonus)
            defender.stun = max(defender.stun, 0.24)
            defender.action = "stunned"
            defender.action_time = 0.24
            defender.guard = 0.0
            self.hit_flash = 0.13
            if is_player:
                self.message = f"CLEAN HIT — straight punch lands for {damage:.0f} damage."
            else:
                self.message = f"You're caught by a clean counter for {damage:.0f} damage."
        defender.health = max(0.0, defender.health - damage)
        attacker.landed = True
        self.last_damage = damage
        if defender.health <= 0:
            self.result = "VICTORY" if is_player else "DEFEAT"
            self.running = False

    def _player_action(self, key: str, dt: float) -> None:
        p, e = self.player, self.enemy
        if key == " ":
            self._start_strike(p, e, True)
            return
        if p.stun > 0:
            return
        # Movement is relative to the opponent's current position. W closes range,
        # S opens range, and A/D change lateral alignment for angles.
        move_speed = 1.65
        if p.action in ("windup", "active", "recovery"):
            move_speed *= 0.38
        elif p.stamina < 20:
            move_speed *= 0.75
        dx = 0.0
        dd = 0.0
        if key == "w":
            dd = move_speed * dt
        elif key == "s":
            dd = -move_speed * dt
        elif key == "a":
            dx = -move_speed * dt
        elif key == "d":
            dx = move_speed * dt
        if dx or dd:
            p.x = max(-ARENA_X, min(ARENA_X, p.x + dx))
            # distance is forward separation; lateral movement shifts relative x.
            e.x = max(-ARENA_X, min(ARENA_X, e.x))
            p.distance = max(0.48, min(6.5, p.distance - dd))
            # The player moves laterally around the opponent, whose world position
            # is the local origin in this compact arena model.
            p.x = max(-ARENA_X, min(ARENA_X, p.x))
            self.message = "You step in." if key == "w" else (
                "You create space." if key == "s" else "You circle for a better angle."
            )

    def _opponent_ai(self, dt: float) -> None:
        e, p = self.enemy, self.player
        if not self.running or e.health <= 0 or e.stun > 0:
            return
        self.ai_think -= dt
        separation = self._distance(p, e)
        difficulty_scale = {"easy": 1.15, "normal": 0.85, "hard": 0.58}[self.difficulty]
        if e.action in ("windup", "active", "recovery"):
            return
        if self.ai_think > 0:
            return
        self.ai_think = difficulty_scale * self.rng.uniform(0.55, 1.0)
        if e.stamina < 24:
            e.stamina = min(100.0, e.stamina + 11.0)
            e.action = "guard"
            e.guard = 0.35
            return
        if separation > 1.12:
            # Approach without teleporting; adjust the lateral line as well.
            e.distance = max(0.55, e.distance - self.rng.uniform(0.24, 0.48))
            if abs(e.x - p.x) > 0.7:
                e.x += 0.24 if p.x > e.x else -0.24
            e.action = "approach"
            e.action_time = 0.25
            self.message = "The opponent advances behind a raised guard."
        elif self.rng.random() < (0.60 if self.difficulty == "hard" else 0.47):
            self._start_strike(e, p, False)
        else:
            e.action = "guard"
            e.guard = self.rng.uniform(0.45, 0.85)
            e.action_time = e.guard
            self.message = "The opponent tightens their guard and waits."

    def _update_fighter(self, fighter: Fighter, target: Fighter, dt: float, is_player: bool) -> None:
        fighter.cooldown = max(0.0, fighter.cooldown - dt)
        fighter.stun = max(0.0, fighter.stun - dt)
        fighter.stamina = min(100.0, fighter.stamina + (5.0 if fighter.action in ("ready", "guard", "approach") else 1.5) * dt)
        if fighter.action_time > 0:
            fighter.action_time = max(0.0, fighter.action_time - dt)
        if fighter.action == "windup" and fighter.action_time <= 0:
            fighter.action = "active"
            fighter.action_time = 0.11
            self._resolve_strike(fighter, target, is_player)
        elif fighter.action == "active" and fighter.action_time <= 0:
            fighter.action = "recovery"
            fighter.action_time = 0.34
            fighter.cooldown = 0.12
        elif fighter.action == "recovery" and fighter.action_time <= 0:
            fighter.action = "ready"
        elif fighter.action in ("guard", "approach", "stunned") and fighter.action_time <= 0:
            fighter.action = "ready"
            fighter.guard = 0.0
        if fighter.action == "guard":
            fighter.guard = max(0.0, fighter.guard - dt)
            if fighter.guard <= 0:
                fighter.action = "ready"

    def update(self, key: str, dt: float) -> None:
        if not self.running:
            return
        self.elapsed += dt
        self.hit_flash = max(0.0, self.hit_flash - dt)
        self._player_action(key, dt)
        self._face_each_other()
        self._opponent_ai(dt)
        self._update_fighter(self.player, self.enemy, dt, True)
        if self.running:
            self._update_fighter(self.enemy, self.player, dt, False)
        self._face_each_other()
        if self.player.health <= 0:
            self.result = "DEFEAT"
            self.running = False
        elif self.enemy.health <= 0:
            self.result = "VICTORY"
            self.running = False

    @staticmethod
    def _bar(value: float, width: int = 22, color: str = GREEN) -> str:
        filled = max(0, min(width, round(width * value / 100)))
        return f"{color}{'█' * filled}{DIM}{'░' * (width - filled)}{RESET}"

    @staticmethod
    def _fighter_art(front: bool, action: str, scale: int = 1) -> list[str]:
        # Compact silhouette; stance changes give terminal animation some life.
        if front:
            if action == "windup":
                return ["   O   ", "  /|\\  ", "   |   ", "  / \\  "]
            if action == "guard":
                return ["  \\O/  ", "   |   ", "  /|\\  ", "  / \\  "]
            if action == "active":
                return ["   O---", "  /|   ", "   |   ", "  / \\  "]
            return ["  \\O/  ", "   |   ", "  /|\\  ", "  / \\  "]
        if action == "windup":
            return ["   O   ", "  /|\\  ", "   |   ", "  / \\  "]
        if action == "active":
            return ["   O   ", "  /|\\__ ", "   |   ", "  / \\  "]
        if action == "stunned":
            return ["  \\O   ", "   |\\  ", "  /|   ", "  / \\  "]
        return ["   O   ", "  /|\\  ", "  /|\\  ", "  / \\  "]

    def _scene(self, width: int, height: int) -> list[str]:
        canvas = [[" " for _ in range(width)] for _ in range(height)]
        horizon = max(4, height // 3)
        # Perspective dojo floor: converging side lines and cross-floor boards.
        center = width // 2
        for y in range(horizon, height - 2):
            progress = (y - horizon) / max(1, height - horizon - 2)
            half = int(3 + progress * (width * 0.46))
            left, right = max(0, center - half), min(width - 1, center + half)
            canvas[y][left] = "/"
            canvas[y][right] = "\\"
            if y % max(2, int(5 - progress * 3)) == 0:
                for x in range(left + 1, right):
                    if canvas[y][x] == " ":
                        canvas[y][x] = "-"
        # A quiet dojo skyline.
        for x in range(2, width - 2):
            if x % 8 == 0 and horizon > 2:
                canvas[horizon - 2][x] = "|"
                canvas[horizon - 1][x] = "|"
        # Enemy scale and vertical placement respond to distance.
        separation = self._distance(self.player, self.enemy)
        enemy_y = int(horizon + max(0.0, min(1.0, (5.8 - separation) / 5.8)) * (height * 0.22))
        enemy_x = int(center + (self.enemy.x - self.player.x) * width / 9.0)
        enemy_x = max(3, min(width - 10, enemy_x))
        enemy_art = self._fighter_art(True, self.enemy.action)
        enemy_color = RED if self.hit_flash > 0 else YELLOW
        for iy, line in enumerate(enemy_art):
            yy = enemy_y + iy
            if 0 <= yy < height:
                for ix, ch in enumerate(line):
                    xx = enemy_x + ix - 3
                    if 0 <= xx < width and ch != " ":
                        canvas[yy][xx] = ch
        # Player is the foreground, seen from behind.
        player_art = self._fighter_art(False, self.player.action)
        player_y = height - 7
        player_x = center - 3
        for iy, line in enumerate(player_art):
            yy = player_y + iy
            if 0 <= yy < height:
                for ix, ch in enumerate(line):
                    xx = player_x + ix - 3
                    if 0 <= xx < width and ch != " ":
                        canvas[yy][xx] = ch
        return ["".join(row) for row in canvas]

    def draw(self) -> None:
        size = shutil.get_terminal_size((80, 28))
        width = max(60, min(size.columns, 100))
        height = max(21, min(size.lines - 8, 32))
        print(CLEAR, end="")
        print(f"{CYAN}{BOLD}BAYEISHUUI{RESET}  {DIM}THE DOJO — STRIKING TRIAL{RESET}")
        print(f"YOU     {self._bar(self.player.health)} {self.player.health:5.1f} HP   "
              f"{BLUE}{self._bar(self.player.stamina, color=BLUE)}{RESET} {self.player.stamina:5.1f} ST")
        print(f"OPPONENT {self._bar(self.enemy.health, color=RED)} {self.enemy.health:5.1f} HP")
        print(f"{DIM}Distance {self._distance(self.player, self.enemy):.2f} m  |  "
              f"Opponent: {self.enemy.action.upper()}  |  Difficulty: {self.difficulty.upper()}{RESET}")
        print()
        for line in self._scene(width, height):
            print(f"{DIM}{line}{RESET}")
        print()
        print(f"{WHITE}{self.message}{RESET}")
        print(f"{BOLD}W{RESET} advance  {BOLD}A{RESET}/{BOLD}D{RESET} circle  {BOLD}S{RESET} retreat  "
              f"{BOLD}SPACE{RESET} strike  {BOLD}Q{RESET} quit")
        print(f"{DIM}Tip: close to striking range, watch the opponent's wind-up, and let stamina recover.{RESET}")

    def play(self) -> None:
        if not sys.stdin.isatty() or not sys.stdout.isatty():
            print("Bayeishuui needs an interactive Linux terminal. Run: python3 inventory/bayeshuui.py")
            return
        fd = sys.stdin.fileno()
        old = termios.tcgetattr(fd)
        try:
            tty.setcbreak(fd)
            sys.stdout.write(HIDE_CURSOR)
            sys.stdout.flush()
            last_draw = 0.0
            last = time.monotonic()
            while self.running:
                now = time.monotonic()
                dt = min(0.12, max(0.001, now - last))
                last = now
                key = ""
                ready, _, _ = select.select([sys.stdin], [], [], TICK)
                if ready:
                    key = sys.stdin.read(1)
                    if key.lower() == "q":
                        self.result = "QUIT"
                        self.running = False
                        break
                    if key not in ("w", "a", "s", "d", "W", "A", "S", "D", " "):
                        key = ""
                self.update(key.lower() if key != " " else key, dt)
                if now - last_draw >= 0.075:
                    self.draw()
                    last_draw = now
        finally:
            termios.tcsetattr(fd, termios.TCSADRAIN, old)
            sys.stdout.write(RESET + SHOW_CURSOR + "\n")
            sys.stdout.flush()
        if self.result == "VICTORY":
            print(f"{GREEN}{BOLD}VICTORY{RESET} — You won the bout in {self.elapsed:.1f} seconds.")
        elif self.result == "DEFEAT":
            print(f"{RED}{BOLD}DEFEAT{RESET} — Your opponent wins this round. Reset and try a different rhythm.")
        else:
            print("You leave the dojo. Train well, return stronger.")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Bayeishuui: a real-time, five-button terminal martial-arts duel."
    )
    parser.add_argument("--difficulty", choices=("easy", "normal", "hard"), default="normal")
    parser.add_argument("--seed", type=int, help="use a fixed random seed for repeatable testing")
    args = parser.parse_args()
    if not sys.stdout.isatty():
        print("Please launch Bayeishuui from an interactive Linux terminal.")
        raise SystemExit(2)
    if shutil.get_terminal_size((80, 24)).columns < 60 or shutil.get_terminal_size((80, 24)).lines < 25:
        print("Please use a terminal at least 60 columns wide and 25 rows tall.")
        raise SystemExit(2)
    MartialArtsGame(args.difficulty, args.seed).play()


if __name__ == "__main__":
    main()
