# NESPC

A cycle-accurate NES emulator for macOS, written in C++17 with SDL2.

![Launcher screenshot placeholder](docs/screenshot.png)

---

## Features

- Cycle-accurate 6502 CPU emulation
- PPU with sprite and background rendering
- APU audio output (square, triangle, noise, DMC channels)
- iNES ROM format support
- Mappers 0, 1, 3, and 4 (covers a large portion of the NES library)
- Graphical ROM launcher with recent-ROM tracking
- Configurable key bindings for two players
- Windowed and fullscreen modes (toggle any time with F11)
- Per-game window scaling (1×, 2×, 3×)
- Persistent config saved to `~/.nespc/config.json`

---

## Requirements

- macOS (tested on macOS 12+)
- Xcode command-line tools (`xcode-select --install`)
- SDL2 and SDL2_ttf, installed via Homebrew:

```bash
brew install sdl2 sdl2_ttf
```

---

## Building

```bash
make
```

The binary is output to `./nespc`. To clean build artifacts:

```bash
make clean
```

---

## Running

### Launcher (recommended)

```bash
./nespc
```

The launcher opens in a 1280×800 window. Browse to a folder containing `.nes` ROM files, then click a ROM once to select it and again to launch it (or press Enter).

### Direct ROM launch

```bash
./nespc path/to/game.nes
```

Skips the launcher and boots straight into the game.

---

## Launcher

| Action | How |
|---|---|
| Select ROM | Single click or ↑/↓ arrow keys |
| Launch ROM | Double-click or press Enter |
| Open ROM folder | Click **Open Folder** or press **O** |
| Open Settings | Click **Settings** or press **Esc** |
| Toggle fullscreen | Click **F11** |
| Quit | Click **Exit** |

Scroll the ROM list with the mouse wheel. Recently played ROMs are tagged **RECENT**.

---

## In-Game Controls

### Player 1 (defaults)

| NES Button | Key |
|---|---|
| A | Z |
| B | X |
| Select | Right Shift |
| Start | Enter |
| Up | ↑ |
| Down | ↓ |
| Left | ← |
| Right | → |

### Player 2 (defaults)

| NES Button | Key |
|---|---|
| A | K |
| B | L |
| Up | W |
| Down | S |
| Left | A |
| Right | D |

### Emulator shortcuts

| Key | Action |
|---|---|
| Esc | Return to launcher |
| R | Reset the current game |
| F11 | Toggle fullscreen |

---

## Settings

Open Settings from the launcher (Settings button or Esc).

### Controls tab

- Switch between **P1** and **P2** with the player tabs.
- Click any **key badge** to begin rebinding that button, then press the desired key. Press Esc to cancel.
- Click **Reset Defaults** to restore both players to the default bindings above.

### Emulator tab

| Setting | Description |
|---|---|
| Window Scale | Windowed game size: 1× (256×240), 2× (512×480), 3× (768×720) |
| Audio Volume | Click or drag the slider |
| Fullscreen Mode | Toggle switch — applies to both the launcher and the game. F11 also works anywhere. |
| ROM Folder | The directory the launcher scans for `.nes` files |

---

## Supported Mappers

| Mapper | ID | Notable games |
|---|---|---|
| NROM | 0 | Donkey Kong, Super Mario Bros., Pac-Man |
| MMC1 / SxROM | 1 | Mega Man 2, The Legend of Zelda, Metroid |
| CNROM | 3 | Paperboy, Track & Field |
| MMC3 / TxROM | 4 | Super Mario Bros. 3, Mega Man 3–6, Kirby's Adventure |

ROMs using other mappers will fail to load or behave incorrectly.

---

## Config file

Settings are saved automatically to `~/.nespc/config.json`. Delete this file to reset everything to defaults.

```json
{
  "romDirectory": "/Users/you/roms",
  "windowScale": 2,
  "audioVolume": 0.800,
  "fullscreen": 0,
  "p0_btn0": 122,
  "p0_btn1": 120,
  ...
}
```

---

## Project structure

```
nespc/
├── main.cpp          Entry point, emulator loop, audio callback
├── bus.cpp/h         System bus — ties CPU, PPU, APU and cartridge together
├── 6502.cpp/h        MOS 6502 CPU core
├── instruction.cpp   6502 instruction set
├── address.cpp       6502 addressing modes
├── ppu.cpp/h         Picture Processing Unit
├── apu.cpp/h         Audio Processing Unit
├── rom.cpp/h         iNES ROM loader
├── mapper.h          Mapper base class
├── mapper_000.*      NROM
├── mapper_001.*      MMC1
├── mapper_003.*      CNROM
├── mapper_004.*      MMC3
├── gui.cpp/h         SDL2 launcher and settings UI
├── config.cpp/h      Persistent configuration
└── Makefile
```

---

## License

For personal use.

 ROM files are not included and must be obtained legally.
