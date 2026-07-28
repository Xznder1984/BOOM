# BOOM - Terminal DOOM Clone

**The modern classic DOOM (1993) experience, playable in any terminal.**

BOOM is a faithful terminal-based recreation of id Software's legendary DOOM (1993), featuring a real raycasting 3D engine, WAD file support, enemies, weapons, items, and the iconic gameplay -- all rendered in your terminal using Unicode block characters and ANSI colors.

```
██████╗  ██████╗ ███████╗
██╔══██╗██╔═══██╗██╔════╝
██████╔╝██║   ██║███████╗
██╔══██╗██║   ██║╚════██║
██████╔╝╚██████╔╝███████║
╚═════╝  ╚═════╝ ╚══════╝
```

## Quick Install (One Command)

### Linux
```bash
curl -sSL https://raw.githubusercontent.com/Xznder1984/BOOM/main/boom_install.sh | bash
```

### macOS
```bash
curl -sSL https://raw.githubusercontent.com/Xznder1984/BOOM/main/boom_install.sh | bash
```

### Windows (PowerShell)
```powershell
irm https://raw.githubusercontent.com/Xznder1984/BOOM/main/boom_install.ps1 | iex
```

### Windows (Batch - Download & Run)
```cmd
curl -sSL -o boom_install.bat https://raw.githubusercontent.com/Xznder1984/BOOM/main/boom_install.bat && boom_install.bat
```

### Universal (Auto-detects OS)
```bash
curl -sSL https://raw.githubusercontent.com/Xznder1984/BOOM/main/boom_install.sh | bash
```

## Features

- **Real Raycasting Engine** - DDA algorithm, texture-mapped walls, floor/ceiling, sprites
- **WAD File Support** - Play original DOOM levels from `.wad` files
- **Built-in Maps** - 3 built-in E1M1-style maps (no WAD needed)
- **7 Weapons** - Fist, Pistol, Shotgun, Chaingun, Rocket Launcher, Plasma Rifle, BFG 9000
- **Enemy AI** - Zombiemen, Shotgun Guys, Imps, Demons, Cacodemons, Barons of Hell
- **Item Pickups** - Health, Armor, Ammo, Keys, Weapons
- **DOOM HUD** - Health bar, armor, ammo, keys, face indicator
- **Minimap** - Toggle with Tab
- **Terminal Audio** - Sound effects via terminal beeps
- **Optional Music** - MP3/FLAC music support (see Audio Setup)
- **Cross-Platform** - Works on Linux, macOS, Windows, WSL, any terminal

## Manual Install

```bash
git clone https://github.com/Xznder1984/BOOM.git
cd BOOM
pip install -r requirements.txt
python boom.py
```

## Usage

```bash
# Play built-in E1M1 map
python boom.py

# Play with a DOOM WAD file
python boom.py --wad /path/to/doom1.wad

# Play a specific map
python boom.py --wad doom1.wad --map E1M3

# List available maps in a WAD
python boom.py --wad doom1.wad --list-maps

# Disable audio
python boom.py --noaudio

# Set FPS target
python boom.py --fps 60
```

## Controls

| Key | Action |
|-----|--------|
| `W` / `S` | Move forward / backward |
| `A` / `D` | Strafe left / right |
| `Left` / `Right` | Turn |
| `F` | Fire weapon |
| `Space` / `Enter` | Use door / switch |
| `1` | Fist |
| `2` | Pistol |
| `3` | Shotgun |
| `4` | Chaingun |
| `5` | Rocket Launcher |
| `6` | Plasma Rifle |
| `7` | BFG 9000 |
| `Tab` | Toggle minimap |
| `+` / `-` | Adjust FPS |
| `Q` | Quit |

## WAD Files

BOOM supports standard DOOM WAD files. Place your `.wad` files anywhere and point to them:

```bash
python boom.py --wad doom1.wad
python boom.py --wad doom2.wad --map MAP01
```

The shareware DOOM1.WAD is freely available from various sources.

## Music (Optional)

BOOM uses terminal beeps for sound effects by default. To add the iconic DOOM soundtrack:

```bash
# Auto-download from Internet Archive
bash boom_audio_setup.sh

# Or manually place MP3/FLAC files in the music/ directory
```

Place files in `music/` named like:
- `02. At Doom's Gate.mp3`
- `04. Dark Halls.flac`

## Audio Setup Script

```bash
bash boom_audio_setup.sh
```

This downloads the DOOM OST from Internet Archive (MP3 format) into the `music/` directory.

## Requirements

- Python 3.6+
- `windows-curses` on Windows (auto-installed)
- A terminal with color support (any modern terminal)

## License

GPL-2.0 - See [LICENSE](LICENSE)

DOOM is a registered trademark of id Software LLC. This project is not affiliated with id Software LLC.

## Credits

- **id Software** - Original DOOM (1993)
- **Bobby Prince** - Original DOOM soundtrack
- **BOOM Contributors** - Terminal port

---

*"RIP AND TEAR... in your terminal!"*
