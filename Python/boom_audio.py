"""BOOM Audio - Terminal sound effects and optional music playback."""
import os
import sys
import threading
import time
import struct
import wave

_audio_enabled = True
_music_thread = None
_music_stop = threading.Event()
_music_vol = 0.5


def beep(freq=440, duration=0.05):
    if not _audio_enabled:
        return
    try:
        if sys.platform == "win32":
            import winsound
            winsound.Beep(int(freq), int(duration * 1000))
        else:
            sys.stdout.write(f"\a")
            sys.stdout.flush()
            time.sleep(duration)
    except Exception:
        pass


def play_shotgun():
    threading.Thread(target=lambda: (beep(120, 0.02), beep(80, 0.08)), daemon=True).start()

def play_pistol():
    threading.Thread(target=lambda: beep(300, 0.04), daemon=True).start()

def play_chaingun():
    threading.Thread(target=lambda: (beep(250, 0.02), beep(180, 0.03)), daemon=True).start()

def play_rocket():
    threading.Thread(target=lambda: (beep(60, 0.1), beep(40, 0.15)), daemon=True).start()

def play_plasma():
    threading.Thread(target=lambda: beep(600, 0.06), daemon=True).start()

def play_enemy_pain():
    threading.Thread(target=lambda: (beep(400, 0.02), beep(300, 0.03)), daemon=True).start()

def play_enemy_death():
    threading.Thread(target=lambda: (beep(300, 0.05), beep(200, 0.08), beep(100, 0.1)), daemon=True).start()

def play_player_pain():
    threading.Thread(target=lambda: (beep(200, 0.03), beep(150, 0.05)), daemon=True).start()

def play_player_death():
    threading.Thread(target=lambda: (beep(300, 0.1), beep(200, 0.1), beep(100, 0.15), beep(50, 0.2)), daemon=True).start()

def play_door():
    threading.Thread(target=lambda: (beep(100, 0.05), beep(120, 0.05), beep(140, 0.05)), daemon=True).start()

def play_item_pickup():
    threading.Thread(target=lambda: (beep(500, 0.03), beep(700, 0.03), beep(900, 0.05)), daemon=True).start()

def play_weapon_pickup():
    threading.Thread(target=lambda: (beep(400, 0.03), beep(600, 0.03), beep(800, 0.03), beep(1000, 0.05)), daemon=True).start()

def play_secret():
    threading.Thread(target=lambda: (beep(400, 0.08), beep(600, 0.08), beep(800, 0.08), beep(1000, 0.12)), daemon=True).start()

def play_switch():
    threading.Thread(target=lambda: (beep(200, 0.03), beep(400, 0.05)), daemon=True).start()

def play_no_ammo():
    threading.Thread(target=lambda: beep(100, 0.08), daemon=True).start()


MUSIC_MAP = {
    "d_e1m1": "02. At Doom's Gate",
    "d_e1m2": "03. The Imp's Song",
    "d_e1m3": "04. Dark Halls",
    "d_e1m4": "05. Kitchen Ace (and Taking Names)",
    "d_e1m5": "06. Suspense",
    "d_e1m6": "07. On the Hunt",
    "d_e1m7": "08. Demons on the Prey",
    "d_e1m8": "09. Sign of Evil",
    "d_e1m9": "10. Hiding the Secrets",
    "d_e2m1": "11. I Sawed the Demons",
    "d_e2m2": "12. The Demons from Adrian's Pen",
    "d_e2m3": "13. Intermission from DOOM",
    "d_e2m4": "14. They're Going to Get You",
    "d_e2m6": "15. Sinister",
    "d_e2m7": "16. Waltz of the Demons",
    "d_e2m8": "17. Nobody Told Me About id",
    "d_e3m1": "18. Hell Keep",
    "d_e3m2": "19. Donna to the Rescue",
    "d_e3m3": "20. Deep Into the Code",
    "d_e3m8": "21. Facing the Spider",
    "d_intro": "01. Introduction",
    "d_victor": "22. Victory",
    "d_bunny": "23. Sweet Little Dead Bunny",
    "d_runnin": "01. At Doom's Gate",
    "d_stalks": "02. The Imp's Song",
    "d_countd": "03. Dark Halls",
    "d_betwee": "04. Kitchen Ace (and Taking Names)",
    "d_doom": "05. Suspense",
    "d_theaim": "06. On the Hunt",
    "d_shawn": "07. Demons on the Prey",
    "d_ddtblu": "08. Sign of Evil",
    "d_ddtwhr": "09. Hiding the Secrets",
    "d_runnin": "10. I Sawed the Demons",
    "d_dead": "11. The Demons from Adrian's Pen",
    "d_stlks2": "12. Intermission from DOOM",
    "d_theda2": "13. They're Going to Get You",
    "d_bossc2": "14. Sinister",
    "d_vil": "15. Waltz of the Demons",
    "d_vic2": "16. Nobody Told Me About id",
}


def _find_music_file(lump_name):
    music_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "music")
    if not os.path.isdir(music_dir):
        return None
    base = MUSIC_MAP.get(lump_name, "")
    if not base:
        return None
    for ext in [".mp3", ".flac", ".ogg", ".wav"]:
        path = os.path.join(music_dir, base + ext)
        if os.path.isfile(path):
            return path
    for f in os.listdir(music_dir):
        if base.lower() in f.lower():
            return os.path.join(music_dir, f)
    return None


def play_music(lump_name="d_e1m1"):
    global _music_stop
    if not _audio_enabled:
        return
    path = _find_music_file(lump_name)
    if not path:
        return
    _music_stop.set()
    time.sleep(0.1)
    _music_stop.clear()
    _music_thread = threading.Thread(target=_music_loop, args=(path,), daemon=True)
    _music_thread.start()


def _music_loop(path):
    try:
        if sys.platform == "win32":
            try:
                from playsound import playsound
                while not _music_stop.is_set():
                    playsound(path)
                    if _music_stop.is_set():
                        break
                return
            except ImportError:
                pass
            os.startfile(path)
            return
        else:
            players = ["mpv --no-video --loop", "mpg123 -q --loop 0", "ffplay -nodisp -autoexit -loop 0", "afplay"]
            for player in players:
                cmd = player.split()
                if os.path.basename(cmd[0]) in ["afplay"]:
                    cmd.append(path)
                else:
                    cmd.extend([path])
                try:
                    import subprocess
                    proc = subprocess.Popen(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                    while not _music_stop.is_set():
                        if proc.poll() is not None:
                            break
                        time.sleep(0.5)
                    try:
                        proc.terminate()
                    except Exception:
                        pass
                    return
                except FileNotFoundError:
                    continue
    except Exception:
        pass


def stop_music():
    _music_stop.set()


def set_audio(enabled):
    global _audio_enabled
    _audio_enabled = enabled
