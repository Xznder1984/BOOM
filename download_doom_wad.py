#!/usr/bin/env python3
"""
BOOM Audio Downloader
Downloads DOOM1.WAD (shareware, freely redistributable) and extracts
all sound effects for use with the BOOM terminal FPS game.

Usage:
    python download_doom_wad.py [--output-dir DIR] [--sounds-only] [--music-only]

The DOOM1.WAD file is the shareware DOOM WAD. id Software released it
as freeware, so it is legally redistributable.
"""

import argparse
import hashlib
import os
import struct
import sys
import urllib.request
import zipfile
import io

# DOOM1.WAD known checksums (there are a few versions)
DOOM1_WAD_URLS = [
    # Primary: idgames archive mirror
    "https://archive.org/download/DoomBFGShareware/DoomBFGShareware/DoomBFGShareware/DOOM.WAD",
    # Fallback: another archive.org source
    "https://archive.org/download/doom_shareware/doom19s.zip",
]

# DOOM sound lump names and what they correspond to in our game
DOOM_SOUND_MAP = {
    "DPOSACT":  "pistol_fire",   # Pistol
    "DPOSACT":  "pistol_fire",   # Pistol (alt spelling)
    "DSSHACT":  "shotgun_fire",  # Shotgun
    "DCHACT":   "chaingun_fire", # Chaingun
    "DRLACT":   "rocket_fire",   # Rocket
    "DPLACT":   "plasma_fire",   # Plasma
    "DBFGACT":  "bfg_fire",      # BFG
    "DPUNCH":   "punch",         # Fist
    "DDOROPN":  "door_open",     # Door
    "DDORCLS":  "door_close",    # Door close
    "DITEMUP":  "item_pickup",   # Item pickup
    "DPOWHIT":  "player_hurt",   # Player hurt
    "DPLDETH":  "player_death",  # Player death
    "DPLEN":    "pistol_empty",  # Pistol empty
    "DGETPOW":  "powerup",       # Powerup
    "DBOPN":    "box_open",      # Box open
    "DSCAMN":   "secret",        # Secret found
    "DSDACT":   "enemy_sight",   # Enemy spots player
    "DPODTH1":  "grunt_death1",  # Grunt death
    "DPODTH2":  "grunt_death2",  # Grunt death
    "DPODTH3":  "grunt_death3",  # Grunt death
    "DPOPAIN":  "grunt_pain",    # Grunt pain
    "DSDPAIN":  "demon_pain",    # Demon pain
    "DSDEATH":  "demon_death",   # Demon death
    "DSACT":    "demon_sight",   # Demon sight
    "DSLOBSHT": "imp_fireball",  # Imp fireball
    "DSIMPACT": "fireball_hit",  # Fireball impact
    "DSCLAW":   "melee_hit",     # Melee hit
    "DSDTHSCREAM": "scream",     # Death scream
    "DSNONE":   "no_ammo",       # No ammo click
    "DSBFG":    "bfg_fire",      # BFG fire
    "DSBFGEXP": "bfg_explode",   # BFG explosion
    "DSSHROOM": "baron_sight",   # Baron of Hell sight
    "DSBOSPN":  "baron_pain",    # Baron of Hell pain
    "DSBOSDTH": "baron_death",   # Baron of Hell death
    "DSCACPN":  "cacodemon_pain",# Cacodemon pain
    "DSCACDTH": "cacodemon_death",# Cacodemon death
    "DSCACACT": "cacodemon_sight",# Cacodemon sight
    "DSDPACT":  "demon_sight",   # Demon sight
    "DSDPDTH":  "demon_death",   # Demon death
    "DSPEDTH":  "heavy_death",   # Heavy weapon guy death
    "DSPEPAIN": "heavy_pain",    # Heavy weapon guy pain
    "DSPEACT":  "heavy_sight",   # Heavy weapon guy sight
    "DSPEASHT": "heavy_fire",    # Heavy weapon guy fire
    "DSSIPAIN": "zombie_pain",   # Zombieman pain
    "DSIDTH":   "zombie_death",  # Zombieman death
    "DSISACT":  "zombie_sight",  # Zombieman sight
    "DSISHOOT": "zombie_fire",   # Zombieman fire
}


def compute_wad_checksum(data):
    """Compute MD5 of WAD data."""
    return hashlib.md5(data).hexdigest()


def download_file(url, description="file"):
    """Download a file from URL, return bytes."""
    print(f"  Downloading {description}...")
    print(f"  URL: {url}")
    try:
        req = urllib.request.Request(url, headers={
            "User-Agent": "BOOM-Downloader/2.0 (terminal DOOM clone)"
        })
        with urllib.request.urlopen(req, timeout=60) as response:
            data = response.read()
            print(f"  Downloaded {len(data):,} bytes")
            return data
    except Exception as e:
        print(f"  Failed: {e}")
        return None


def download_doom1_wad():
    """Download DOOM1.WAD from available sources."""
    print("Downloading DOOM1.WAD (shareware, freely redistributable)...")
    print()

    for url in DOOM1_WAD_URLS:
        if url.endswith(".zip"):
            data = download_file(url, "DOOM1.WAD (ZIP)")
            if data:
                try:
                    with zipfile.ZipFile(io.BytesIO(data)) as zf:
                        # Find WAD file inside
                        for name in zf.namelist():
                            if name.upper().endswith(".WAD"):
                                print(f"  Extracting {name} from ZIP...")
                                wad_data = zf.read(name)
                                print(f"  Extracted {len(wad_data):,} bytes")
                                return wad_data
                except Exception as e:
                    print(f"  Failed to extract ZIP: {e}")
        else:
            data = download_file(url, "DOOM1.WAD")
            if data and len(data) > 1000:
                return data

    print("ERROR: Could not download DOOM1.WAD")
    print("You can manually download DOOM1.WAD from:")
    print("  https://archive.org/details/DoomBFGShareware")
    print("and place it in the game directory.")
    return None


def parse_wad(data):
    """Parse a DOOM WAD file. Returns (wad_type, num_lumps, directory_offset).
    Lump entries: list of (name, offset, size)."""
    if len(data) < 12:
        return None, None, None

    # WAD header: 4-byte type, 4-byte num_lumps, 4-byte dir_offset
    wad_type = data[0:4]
    num_lumps = struct.unpack_from("<I", data, 4)[0]
    dir_offset = struct.unpack_from("<I", data, 8)[0]

    print(f"  WAD type: {wad_type}")
    print(f"  Lumps: {num_lumps}")
    print(f"  Directory offset: {dir_offset}")

    # Parse directory
    lumps = []
    for i in range(num_lumps):
        entry_offset = dir_offset + i * 16
        if entry_offset + 16 > len(data):
            break
        lump_offset = struct.unpack_from("<I", data, entry_offset)[0]
        lump_size = struct.unpack_from("<I", data, entry_offset + 4)[0]
        lump_name = data[entry_offset + 8:entry_offset + 16]
        # Strip null bytes from name
        lump_name = lump_name.split(b'\x00')[0].decode('ascii', errors='replace')
        lumps.append((lump_name, lump_offset, lump_size))

    return wad_type, num_lumps, lumps


def extract_sound_lumps(wad_data, lumps, output_dir):
    """Extract sound lumps from WAD to individual files."""
    sounds_dir = os.path.join(output_dir, "sounds")
    os.makedirs(sounds_dir, exist_ok=True)

    extracted = 0
    for name, offset, size in lumps:
        if not name.startswith("D") and not name.startswith("S"):
            continue
        if size < 8:
            continue

        # DOOM sound format:
        # Byte 0-2: padding (0x00, 0x00, 0x00)
        # Byte 3: format type (3 = PCM)
        # Byte 4-5: sample rate (little-endian uint16)
        # Byte 6-7: number of samples (little-endian uint16)
        # Byte 8+: 8-bit signed PCM samples
        header = wad_data[offset:offset + 8]
        if len(header) < 8:
            continue

        padding = header[0:3]
        fmt_type = header[3]
        sample_rate = struct.unpack_from("<H", header, 4)[0]
        num_samples = struct.unpack_from("<H", header, 6)[0]

        # Sanity checks
        if fmt_type != 3:
            continue
        if sample_rate < 8000 or sample_rate > 48000:
            continue
        if num_samples > 1000000:
            continue

        samples = wad_data[offset + 8:offset + 8 + num_samples]
        if len(samples) < num_samples:
            continue

        # Save as raw PCM (we'll write a simple header)
        safe_name = name.replace("/", "_").replace("\\", "_")
        wav_path = os.path.join(sounds_dir, f"{safe_name}.raw")

        with open(wav_path, "wb") as f:
            # Write simple header: sample_rate(2 bytes LE), num_samples(2 bytes LE)
            f.write(struct.pack("<HH", sample_rate, num_samples))
            f.write(samples)

        extracted += 1

    return extracted


def extract_music_lumps(wad_data, lumps, output_dir):
    """Extract music lumps from WAD."""
    music_dir = os.path.join(output_dir, "music")
    os.makedirs(music_dir, exist_ok=True)

    extracted = 0
    for name, offset, size in lumps:
        if not name.startswith("D_"):
            continue
        if size < 100:
            continue

        # Save raw music data
        safe_name = name.replace("/", "_").replace("\\", "_")
        music_path = os.path.join(music_dir, f"{safe_name}.mus")

        music_data = wad_data[offset:offset + size]
        with open(music_path, "wb") as f:
            f.write(music_data)

        extracted += 1

    return extracted


def main():
    parser = argparse.ArgumentParser(
        description="Download DOOM1.WAD and extract audio for BOOM"
    )
    parser.add_argument(
        "--output-dir", "-o",
        default=".",
        help="Output directory (default: current directory)"
    )
    parser.add_argument(
        "--sounds-only",
        action="store_true",
        help="Only extract sound effects"
    )
    parser.add_argument(
        "--music-only",
        action="store_true",
        help="Only extract music"
    )
    parser.add_argument(
        "--wad-only",
        action="store_true",
        help="Only download the WAD file, don't extract"
    )
    args = parser.parse_args()

    print("=" * 50)
    print("  BOOM Audio Downloader")
    print("  Downloads DOOM1.WAD (shareware) and extracts audio")
    print("=" * 50)
    print()

    # Create output directories
    os.makedirs(args.output_dir, exist_ok=True)

    # Download WAD
    wad_data = download_doom1_wad()
    if not wad_data:
        print("\nFailed to download DOOM1.WAD")
        print("Sound effects will use terminal beeps instead.")
        sys.exit(1)

    # Save WAD file
    wad_path = os.path.join(args.output_dir, "DOOM1.WAD")
    with open(wad_path, "wb") as f:
        f.write(wad_data)
    print(f"\nSaved DOOM1.WAD ({len(wad_data):,} bytes)")

    if args.wad_only:
        print("Done (--wad-only).")
        return

    # Parse WAD
    print("\nParsing WAD directory...")
    wad_type, num_lumps, lumps = parse_wad(wad_data)
    if not lumps:
        print("ERROR: Could not parse WAD directory")
        sys.exit(1)

    # Extract sounds
    if not args.music_only:
        print("\nExtracting sound effects...")
        num_sounds = extract_sound_lumps(wad_data, lumps, args.output_dir)
        print(f"  Extracted {num_sounds} sound effects")

    # Extract music
    if not args.sounds_only:
        print("\nExtracting music...")
        num_music = extract_music_lumps(wad_data, lumps, args.output_dir)
        print(f"  Extracted {num_music} music tracks")

    print("\nDone!")
    print(f"\nFiles saved to: {os.path.abspath(args.output_dir)}")
    print(f"  DOOM1.WAD      - Original WAD file")
    if not args.music_only:
        print(f"  sounds/        - Sound effects (.raw format)")
    if not args.sounds_only:
        print(f"  music/         - Music tracks (.mus format)")
    print(f"\nThe BOOM game will use these sounds automatically.")


if __name__ == "__main__":
    main()
