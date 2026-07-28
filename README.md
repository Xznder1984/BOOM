# BOOM - Terminal DOOM Clone

![Version](https://img.shields.io/badge/version-1.0.0-red)
![License](https://img.shields.io/badge/license-GPL--2.0-blue)
![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS%20%7C%20Windows-green)

A standalone terminal-based DOOM (1993) clone built from scratch with raycasting 3D engine, built-in levels (no external WAD dependency), and one-command installer.

## Features

- **Raycasting 3D Engine** - Smooth first-person perspective rendering in terminal
- **Built-in Levels** - 3 complete levels with enemies, weapons, keys, and secrets (no WAD files needed!)
- **Full Weapon Arsenal** - Fist, Pistol, Shotgun, Chaingun, Rocket Launcher, Plasma Rifle, BFG9000
- **Enemy AI** - Zombies, Imps, Demons, Cacodemons, Barons with pathfinding and combat
- **Terminal Audio** - Beep sounds for weapons, items, doors
- **Automap** - Tab key reveals overhead map
- **Cross-Platform** - Linux, macOS, Windows, WSL

## Quick Install

### One-Command Install (curl)

**Linux:**
```bash
curl -fsSL https://raw.githubusercontent.com/Xznder1984/BOOM/main/boom_install.sh | bash
```

**macOS:**
```bash
curl -fsSL https://raw.githubusercontent.com/Xznder1984/BOOM/main/boom_install.sh | bash
```

**Windows (PowerShell):**
```powershell
iex (iwr -Uri "https://raw.githubusercontent.com/Xznder1984/BOOM/main/boom_install.ps1").Content
```

**Windows (Batch):**
```cmd
curl -fsSL https://raw.githubusercontent.com/Xznder1984/BOOM/main/boom_install.bat -o boom_install.bat && boom_install.bat
```

### Manual Install

**Linux/macOS:**
```bash
git clone https://github.com/Xznder1984/BOOM.git
cd BOOM
make
sudo make install
```

**Windows:**
```cmd
git clone https://github.com/Xznder1984/BOOM.git
cd BOOM
gcc -o boom.exe boom_main.c boom_render.c boom_game.c boom_wad.c boom_audio.c -lncursesw -lm
```

## Usage

```bash
boom                          # Start at level 1
boom -l 0                     # Start at level 0 (Hangar)
boom -l 1                     # Start at level 1 (Nuclear Plant)
boom -l 2                     # Start at level 2 (Toxic Refinery)
boom -f 30                    # Set target FPS to 30
boom -w 120 -t 40             # Set screen dimensions
```

## Controls

| Key | Action |
|-----|--------|
| `W` `A` `S` `D` | Move / Strafe |
| `<-` `->` | Turn left/right |
| `F` / `Space` | Fire weapon |
| `E` | Use/Open door |
| `1` - `7` | Select weapon |
| `Tab` | Toggle automap |
| `+` / `-` | Adjust FPS |
| `Esc` | Pause / Quit |
| `Q` | Quit |

## Weapons

| # | Weapon | Damage | Fire Rate | Ammo |
|---|--------|--------|-----------|------|
| 1 | Fist | 20 | Slow | None |
| 2 | Pistol | 15 | Medium | Clips |
| 3 | Shotgun | 7*3 pellets | Slow | Shells |
| 4 | Chaingun | 10 | Fast | Clips |
| 5 | Rocket Launcher | 100 | Slow | Rockets |
| 6 | Plasma Rifle | 25 | Fast | Cells |
| 7 | BFG9000 | 10-400 | Very Slow | Cells |

## Enemies

| Type | Health | Speed | Attack | Color |
|------|--------|-------|--------|-------|
| Zombie | 20 | Slow | Hitscan | Green |
| Imp | 60 | Medium | Fireball | Orange |
| Demon | 150 | Fast | Melee | Red |
| Cacodemon | 400 | Slow | Fireball | Red |
| Baron | 1000 | Slow | Fireball | Dark Red |

## Levels

1. **Hangar** - Introduction level, learn controls
2. **Nuclear Plant** - Medium difficulty, keys and doors
3. **Toxic Refinery** - Hard, more enemies, secrets

## Optional Music Setup

The game includes terminal beep sounds. For full MIDI music (Bobby Prince OST), run:

```bash
./boom_audio_setup.sh
```

This will download the DOOM soundtrack from Archive.org for use with an external MIDI player.

## Building from Source

### Dependencies

- **C Compiler**: GCC or Clang
- **ncurses**: Terminal UI library
- **Math library**: `-lm`

**Linux (Debian/Ubuntu):**
```bash
sudo apt install gcc libncurses-dev
```

**Linux (Fedora/RHEL):**
```bash
sudo dnf install gcc ncurses-devel
```

**macOS:**
```bash
xcode-select --install
```

**Windows:**
- Install MinGW or MSYS2 with gcc and ncurses

### Build

```bash
make
```

## Python Version

A Python version is available in the `Python/` folder:

```bash
cd Python
pip install -r requirements.txt
python boom.py
```

Requires Python 3.7+ and `windows-curses` on Windows.

## License

This project is licensed under the GNU General Public License v2.0 - see the [LICENSE](LICENSE) file for details.

## Credits

- Inspired by id Software's DOOM (1993)
- Raycasting engine based on classic techniques
- Built from scratch in C and Python

## Star History

If you find BOOM useful, please give it a star on GitHub!
