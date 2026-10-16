#!/usr/bin/env python3
"""Generates the launcher splash (128x96) and icon (8x8) for Minesweeper.

Run from the repo root:  python3 tools/make_art.py
Needs Pillow. The output PNGs are committed, so this only needs re-running
when the art changes.
"""
from PIL import Image

BG = (16, 20, 36)
HIDDEN = (78, 100, 152)
LIGHT = (134, 160, 212)
DARK = (40, 52, 90)
OPEN = (196, 202, 218)
LINE = (158, 166, 188)
RED = (235, 40, 40)
POLE = (24, 24, 34)
MINE = (20, 20, 30)
WHITE = (240, 240, 255)
NUM = {1: (30, 80, 230), 2: (20, 140, 40), 3: (220, 40, 40)}

DIGITS = {1: [2, 6, 2, 2, 7], 2: [7, 1, 7, 4, 7], 3: [7, 1, 3, 1, 7]}

# 3x5 capitals for the title.
FONT = {
    'M': ["#.#", "###", "###", "#.#", "#.#"],
    'I': ["###", ".#.", ".#.", ".#.", "###"],
    'N': ["##.", "#.#", "#.#", "#.#", "#.#"],
    'E': ["###", "#..", "##.", "#..", "###"],
    'S': [".##", "#..", ".#.", "..#", "##."],
    'W': ["#.#", "#.#", "###", "###", "#.#"],
    'P': ["##.", "#.#", "##.", "#..", "#.."],
    'R': ["##.", "#.#", "##.", "#.#", "#.#"],
}

FLAG8 = ["........", "..RRK...", ".RRRK...", "..RRK...", "....K...", "....K...", "..KKKK..", "........"]


def rect(img, x, y, w, h, col):
    for yy in range(y, y + h):
        for xx in range(x, x + w):
            if 0 <= xx < img.width and 0 <= yy < img.height:
                img.putpixel((xx, yy), col)


def hidden(img, x, y, s):
    rect(img, x, y, s, s, HIDDEN)
    rect(img, x, y, s - 1, 1, LIGHT)
    rect(img, x, y, 1, s - 1, LIGHT)
    rect(img, x, y + s - 1, s, 1, DARK)
    rect(img, x + s - 1, y, 1, s, DARK)


def opened(img, x, y, s):
    rect(img, x, y, s, s, OPEN)
    rect(img, x, y + s - 1, s, 1, LINE)
    rect(img, x + s - 1, y, 1, s, LINE)


def digit(img, n, cx, cy, scale):
    for r, bits in enumerate(DIGITS[n]):
        for c in range(3):
            if bits & (4 >> c):
                rect(img, cx - 3 * scale // 2 + c * scale, cy - 5 * scale // 2 + r * scale, scale, scale, NUM[n])


def flag(img, x, y, scale=1):
    for r, row in enumerate(FLAG8):
        for c, ch in enumerate(row):
            if ch != '.':
                rect(img, x + c * scale, y + r * scale, scale, scale, RED if ch == 'R' else POLE)


def mine(img, cx, cy, r):
    for yy in range(-r, r + 1):
        for xx in range(-r, r + 1):
            if xx * xx + yy * yy <= r * r + r:
                img.putpixel((cx + xx, cy + yy), MINE)
    rect(img, cx - r - 2, cy, 2 * r + 5, 1, MINE)
    rect(img, cx, cy - r - 2, 1, 2 * r + 5, MINE)
    rect(img, cx - r // 2 - 1, cy - r // 2 - 1, 2, 2, WHITE)


def text(img, s, x, y, scale, col):
    for ch in s:
        for r, row in enumerate(FONT[ch]):
            for c, p in enumerate(row):
                if p == '#':
                    rect(img, x + c * scale + 1, y + r * scale + 1, scale, scale, (0, 0, 0))
        for r, row in enumerate(FONT[ch]):
            for c, p in enumerate(row):
                if p == '#':
                    rect(img, x + c * scale, y + r * scale, scale, scale, col)
        x += 4 * scale


def splash():
    img = Image.new('RGB', (128, 96), BG)
    s = 16
    layout = [
        "HH112HHH",
        "H1..1F2H",
        "11..112H",
        "........",
        "11.112F1",
        "F1.1M211",
    ]
    for gy, row in enumerate(layout):
        for gx, ch in enumerate(row):
            x, y = gx * s, gy * s
            if ch in 'HF':
                hidden(img, x, y, s)
                if ch == 'F':
                    flag(img, x, y, 2)
            else:
                opened(img, x, y, s)
                if ch.isdigit():
                    digit(img, int(ch), x + s // 2, y + s // 2, 2)
                if ch == 'M':
                    rect(img, x, y, s - 1, s - 1, (230, 50, 40))
                    mine(img, x + s // 2, y + s // 2, 4)
    # Title band
    rect(img, 0, 36, 128, 26, BG)
    rect(img, 0, 36, 128, 1, (90, 120, 200))
    rect(img, 0, 61, 128, 1, (90, 120, 200))
    w = 11 * 4 * 2 - 2
    text(img, "MINESWEEPER", (128 - w) // 2, 44, 2, (255, 170, 70))
    return img


def icon():
    img = Image.new('RGB', (8, 8), HIDDEN)
    rect(img, 0, 0, 7, 1, LIGHT)
    rect(img, 0, 0, 1, 7, LIGHT)
    rect(img, 0, 7, 8, 1, DARK)
    rect(img, 7, 0, 1, 8, DARK)
    flag(img, 0, 0)
    return img


if __name__ == '__main__':
    splash().save('assets/splash.png')
    icon().save('assets/icon.png')
