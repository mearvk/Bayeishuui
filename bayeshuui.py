#!/usr/bin/env python3
"""Bayeishuui — a small, dependency-free terminal adventure for Linux."""

from __future__ import annotations

import os
import random
import shutil
import sys
import termios
import tty
from dataclasses import dataclass

WIDTH, HEIGHT =  forty = 40, 16
WALL, FLOOR, PLAYER, EXIT, ENEMY, POTION, RELIC = "#", ".", "@", ">", "g", "!", "*"
COLORS = {
    WALL: "\033[90m", FLOOR: "\033[2m", PLAYER: "\033[96;1m",
    EXIT: "\033[92;1m", ENEMY: "\033[91;1m", POTION: "\033[95;1m",
    RELIC: "\033[93;1m",
}
RESET = "\033[0m"


@dataclass
class Enemy:
    x: int
    y: int
    hp: int = 3
    glyph: str = ENEMY


class Game:
    def __init__(self) -> None:
        self.rng = random.Random()
        self.level = 1
        self.hp = 10
        self.max_hp = 10
        self.potions = 2
        self.relics = 0
        self.turns = 0
        self.message = "Find the relic, then reach the gate (>)."
        self.running = True
        self.won = False
        self.new_level()

    def new_level(self) -> None:
        self.grid = [[WALL for _ in range(WIDTH)] for _ in range(HEIGHT)]
        # Carve a connected, winding cave with a random-walk digger.
        x, y = 1, 1
        self.grid[y][x] = FLOOR
        for _ in range(WIDTH * HEIGHT * 3):
            dx, dy = self.rng.choice(((1, 0), (-1, 0), (0, 1), (0, -1)))
            x = max(1, min(WIDTH - 2, x + dx))
            y = max(1, min(HEIGHT - 2, y + dy))
            self.grid[y][x] = FLOOR
        floors = [(cx, cy) for cy in range(1, HEIGHT - 1)
                  for cx in range(1, WIDTH - 1) if self.grid[cy][cx] == FLOOR]
        self.x, self.y = 1, 1
        self.grid[self.y][self.x] = FLOOR
        # Ensure the spawn is open and place objectives far from it.
        floors = [(cx, cy) for cx, cy in floors if (cx, cy) != (self.x, self.y)]
        self.exit_pos = max(floors, key=lambda p: abs(p[0] - self.x) + abs(p[1] - self.y))
        remaining = [p for p in floors if p != self.exit_pos]
        self.relic_pos = self.rng.choice(remaining)
        self.grid[self.exit_pos[1]][self.exit_pos[0]] = EXIT
        self.grid[self.relic_pos[1]][self.relic_pos[0]] = RELIC
        self.enemies = []
        candidates = [p for p in remaining if p != self.relic_pos and
                      abs(p[0] - self.x) + abs(p[1] - self.y) > 7]
        self.rng.shuffle(candidates)
        for ex, ey in candidates[:min(4 + self.level, 9)]:
            self.enemies.append(Enemy(ex, ey, hp=2 + self.level // 2))
        self.message = f"Level {self.level}: recover the relic (*) and find the gate (>)."

    def draw(self) -> None:
        # ANSI clear-screen and cursor-home; works in common Linux terminals.
        print("\033[2J\033[H", end="")
        print("\033[96;1mBAYEISHUUI\033[0m  |  The Hollow Gate")
        print(f"Level {self.level}   HP {self.hp}/{self.max_hp}   Potions {self.potions}   Relics {self.relics}")
        enemy_map = {(e.x, e.y): e.glyph for e in self.enemies}
        for yy, row in enumerate(self.grid):
            line = []
            for xx, cell in enumerate(row):
                glyph = PLAYER if (xx, yy) == (self.x, self.y) else enemy_map.get((xx, yy), cell)
                line.append(f"{COLORS.get(glyph, '')}{glyph}{RESET}")
            print("".join(line))
        print("\n" + self.message)
        print("Move: W/A/S/D or arrows | F: attack | P: potion | Q: quit")
        if not self.relics:
            print("Objective: collect the golden relic (*) before using the gate.")
        else:
            print("Objective: the relic is yours — reach the gate (>).")

    def enemy_at(self, x: int, y: int):
        return next((e for e in self.enemies if (e.x, e.y) == (x, y)), None)

    def move(self, dx: int, dy: int) -> None:
        nx, ny = self.x + dx, self.y + dy
        if not (0 <= nx < WIDTH and 0 <= ny < HEIGHT):
            return
        enemy = self.enemy_at(nx, ny)
        if enemy:
            self.attack(enemy)
            return
        tile = self.grid[ny][nx]
        if tile == WALL:
            self.message = "Stone blocks your way."
            return
        if tile == EXIT:
            if self.relics:
                self.level += 1
                if self.level > 3:
                    self.won = True
                    self.running = False
                    return
                self.new_level()
            else:
                self.message = "The gate is sealed. Find the relic (*) first."
            return
        if tile == RELIC:
            self.relics += 1
            self.grid[ny][nx] = FLOOR
            self.message = "The relic warms in your hand. The gate is open!"
        elif tile == POTION:
            self.potions += 1
            self.grid[ny][nx] = FLOOR
            self.message = "You found a healing potion."
        else:
            self.message = "You move carefully through the hollow."
        self.x, self.y = nx, ny

    def attack(self, enemy: Enemy) -> None:
        damage = self.rng.randint(1, 3)
        enemy.hp -= damage
        self.message = f"You strike the cave warden for {damage} damage."
        if enemy.hp <= 0:
            self.enemies.remove(enemy)
            self.message += " It dissolves into ash."
        else:
            self.hp -= self.rng.randint(1, 2)
            self.message += " It retaliates!"
            if self.hp <= 0:
                self.running = False

    def use_potion(self) -> None:
        if self.potions <= 0:
            self.message = "No potions remain."
        elif self.hp >= self.max_hp:
            self.message = "You are already at full health."
        else:
            self.potions -= 1
            healed = min(5, self.max_hp - self.hp)
            self.hp += healed
            self.message = f"The potion restores {healed} HP."

    def step(self, key: str) -> None:
        key = key.lower()
        moves = {"w": (0, -1), "a": (-1, 0), "s": (0, 1), "d": (1, 0),
                 "up": (0, -1), "left": (-1, 0), "down": (0, 1), "right": (1, 0)}
        if key in moves:
            self.move(*moves[key])
        elif key == "f":
            target = next((e for e in self.enemies
                           if abs(e.x - self.x) + abs(e.y - self.y) == 1), None)
            if target:
                self.attack(target)
            else:
                self.message = "There is no warden beside you."
        elif key == "p":
            self.use_potion()
        elif key == "q":
            self.running = False
            return
        else:
            self.message = "Unknown command. Use W/A/S/D, F, P or Q."
        self.turns += 1
        # Wandering wardens take a step toward the player, but cannot pass walls.
        for enemy in list(self.enemies):
            if abs(enemy.x - self.x) + abs(enemy.y - self.y) <= 1:
                continue
            if self.rng.random() < 0.55:
                options = [(enemy.x + dx, enemy.y + dy) for dx, dy in
                           ((1, 0), (-1, 0), (0, 1), (0, -1))]
                options = [(ex, ey) for ex, ey in options
                           if self.grid[ey][ex] != WALL and (ex, ey) != (self.x, self.y)
                           and not self.enemy_at(ex, ey)]
                if options:
                    enemy.x, enemy.y = min(options, key=lambda p: abs(p[0] - self.x) + abs(p[1] - self.y))
            if abs(enemy.x - self.x) + abs(enemy.y - self.y) == 1:
                self.hp -= 1
                self.message = "A cave warden claws you for 1 HP!"
                if self.hp <= 0:
                    self.running = False
                    return

    def play(self) -> None:
        if not sys.stdin.isatty():
            print("Bayeishuui needs an interactive terminal. Run: python3 bayeshuui.py")
            return
        fd = sys.stdin.fileno()
        old = termios.tcgetattr(fd)
        try:
            tty.setcbreak(fd)
            while self.running:
                self.draw()
                key = read_key()
                self.step(key)
        finally:
            termios.tcsetattr(fd, termios.TCSADRAIN, old)
            print("\033[0m\033[?25h\n", end="")
        if self.won:
            print(f"\033[92;1mVICTORY!\033[0m You escaped the Hollow Gate in {self.turns} turns.")
        elif self.hp <= 0:
            print("\033[91;1mGAME OVER\033[0m The hollow has claimed another traveler.")
        else:
            print("You leave the hollow. Until next time.")


def read_key() -> str:
    key = sys.stdin.read(1)
    if key == "\x1b":
        # Read the common ANSI arrow-key sequence without blocking on a timer.
        import select
        seq = ""
        for _ in range(2):
            ready, _, _ = select.select([sys.stdin], [], [], 0.025)
            if not ready:
                break
            seq += sys.stdin.read(1)
        return {"[A": "up", "[B": "down", "[C": "right", "[D": "left"}.get(seq, "")
    return key


def main() -> None:
    if "--help" in sys.argv or "-h" in sys.argv:
        print("Bayeishuui — terminal dungeon adventure\n\nUsage: ./bayeshuui.py\n\nControls: WASD/arrows move, F attack, P potion, Q quit.")
        return
    if not sys.stdout.isatty():
        print("Please launch Bayeishuui from a Linux terminal.")
        return
    if shutil.get_terminal_size((80, 24)).columns < 42:
        print("Please widen your terminal to at least 42 columns.")
        return
    Game().play()


if __name__ == "__main__":
    main()
