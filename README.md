# Minesweeper

[![Build](https://github.com/chili-chip/chili-minesweeper/actions/workflows/build.yml/badge.svg)](https://github.com/chili-chip/chili-minesweeper/actions/workflows/build.yml)

The classic logic puzzle for the ChiliChip **VGC Zero** (128x128), built on the
[chilichip fork of the 32blit SDK](https://github.com/chili-chip/32blit-sdk).
It also runs on desktop (SDL) and in the browser: [play it on chilichip.eu](https://chilichip.eu/games/minesweeper/).

Clear a 16x15 board hiding 40 mines. Numbers tell you how many mines touch each cell; flag the
ones you've found and reveal everything else. Your first reveal is always safe, and the clock
stops at 999 seconds.

## Controls

| Button | Menu | In game | After a win or loss |
| --- | --- | --- | --- |
| D-pad | Choose an option | Move the cursor | |
| A | Select | Reveal the cell | New game |
| B | | Flag or unflag the cell | Back to the menu |
| MENU | | Back to the menu | Back to the menu |

On desktop: arrow keys or WASD, **Z** = A, **X** = B, **Esc** = MENU.

## Code layout

| File | What |
| --- | --- |
| `src/game.cpp` | 32blit `init`/`update`/`render`, background music and the active scene |
| `src/menu_scenes.cpp` | Menu, credits and game scenes |
| `src/minesweeper.cpp` | Board rules: mine placement, flood reveal, flags, win and loss, header drawing |
| `src/input.cpp` | Cursor movement and cell actions |
| `src/music.cpp` | RTTTL tune player on the 32blit audio channels |
| `src/timer.cpp` | Game clock |
| `src/config.hpp` | Board size, mine count and time limit |
| `assets/sprites.png` | Cells, cursor, flag and face sprites |

Only `src/` is compiled; the top-level `game.cpp` / `game.hpp` are unused leftovers from the template.

## Building

New to the SDK? Start with the [Linux quickstart](https://chilichip.eu/docs/game-sdk/quickstart/linux-quickstart/) on chilichip.eu. It installs the toolchain and clones `32blit-sdk`, `pico-sdk` and `pico-extras` next to this repo. CMake 3.28 or newer is needed for the presets in `CMakePresets.json`.

Desktop (SDL):
```
mkdir build && cd build
cmake -D32BLIT_DIR=../../32blit-sdk ..
make
./minesweeper
```

VGC Zero:
```
mkdir build.vgc && cd build.vgc
cmake .. \
  -D32BLIT_DIR=../../32blit-sdk \
  -DPICO_SDK_PATH=../../pico-sdk \
  -DPICO_BOARD=chilichip_vgc \
  -DPICO_PLATFORM=rp2350-arm-s \
  -DCMAKE_TOOLCHAIN_FILE=../../32blit-sdk/pico2.toolchain
make
```
This produces a `.uf2` you can copy onto the console's `RP2350` drive. To build an installable `.blit` for the VGC Zero loader instead, add `-DBLIT_EXECUTABLE_PICO_BLIT=1` and install it with `32blit install minesweeper.blit`. See the [VGC build guide](https://github.com/chili-chip/32blit-sdk/blob/master/docs/vgc.md) for details, including flashing over SWD.

## Licence

MIT, see [LICENSE](LICENSE).
