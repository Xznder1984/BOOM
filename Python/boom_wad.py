"""BOOM WAD File Parser - Parses DOOM WAD files."""
import struct

THING_TYPES = {
    1: {"name": "Player 1 Start", "cat": "start"},
    2: {"name": "Player 2 Start", "cat": "start"},
    3: {"name": "Player 3 Start", "cat": "start"},
    4: {"name": "Player 4 Start", "cat": "start"},
    5: {"name": "Red Keycard", "cat": "key", "color": "red"},
    6: {"name": "Yellow Keycard", "cat": "key", "color": "yellow"},
    7: {"name": "Blue Keycard", "cat": "key", "color": "blue"},
    8: {"name": "Clip", "cat": "ammo", "type": "bullets", "amount": 10},
    9: {"name": "Box of Bullets", "cat": "ammo", "type": "bullets", "amount": 50},
    10: {"name": "Shells", "cat": "ammo", "type": "shells", "amount": 4},
    11: {"name": "Box of Shells", "cat": "ammo", "type": "shells", "amount": 20},
    12: {"name": "Rocket", "cat": "ammo", "type": "rockets", "amount": 1},
    13: {"name": "Box of Rockets", "cat": "ammo", "type": "rockets", "amount": 5},
    14: {"name": "Cell", "cat": "ammo", "type": "cells", "amount": 40},
    15: {"name": "Cell Pack", "cat": "ammo", "type": "cells", "amount": 200},
    16: {"name": "Backpack", "cat": "backpack"},
    17: {"name": "Stimpack", "cat": "health", "amount": 10},
    18: {"name": "Medikit", "cat": "health", "amount": 25},
    19: {"name": "Health Bonus", "cat": "health_bonus", "amount": 1},
    20: {"name": "Security Armor", "cat": "armor", "amount": 100},
    21: {"name": "Combat Armor", "cat": "armor", "amount": 200},
    22: {"name": "Barrel", "cat": "decoration", "block": True},
    23: {"name": "Tall Pillar", "cat": "decoration", "block": True},
    24: {"name": "Tall Green Pillar", "cat": "decoration", "block": True},
    25: {"name": "Tall Red Pillar", "cat": "decoration", "block": True},
    26: {"name": "Short Green Pillar", "cat": "decoration", "block": True},
    27: {"name": "Short Red Pillar", "cat": "decoration", "block": True},
    28: {"name": "Short Blue Pillar", "cat": "decoration", "block": True},
    39: {"name": "Red Skull", "cat": "key", "color": "red"},
    40: {"name": "Yellow Skull", "cat": "key", "color": "yellow"},
    41: {"name": "Blue Skull", "cat": "key", "color": "blue"},
    53: {"name": "Health Bonus", "cat": "health_bonus", "amount": 1},
    54: {"name": "Security Armor", "cat": "armor", "amount": 100},
    55: {"name": "Health Bonus", "cat": "health_bonus", "amount": 1},
    56: {"name": "Combat Armor", "cat": "armor", "amount": 200},
    57: {"name": "Health Bonus", "cat": "health_bonus", "amount": 1},
    2001: {"name": "Chainsaw", "cat": "weapon", "wtype": "chainsaw", "slot": 1},
    2002: {"name": "Super Shotgun", "cat": "weapon", "wtype": "sshotgun", "slot": 3},
    2003: {"name": "Chaingun", "cat": "weapon", "wtype": "chaingun", "slot": 4},
    2004: {"name": "Rocket Launcher", "cat": "weapon", "wtype": "rocket", "slot": 5},
    2005: {"name": "Plasma Rifle", "cat": "weapon", "wtype": "plasma", "slot": 6},
    2006: {"name": "BFG 9000", "cat": "weapon", "wtype": "bfg", "slot": 7},
    2007: {"name": "Shotgun", "cat": "weapon", "wtype": "shotgun", "slot": 3},
    3001: {"name": "Zombieman", "cat": "enemy", "health": 20, "damage": 3, "speed": 1.5, "radius": 20, "color": (0, 200, 0)},
    3002: {"name": "Shotgun Guy", "cat": "enemy", "health": 30, "damage": 5, "speed": 1.5, "radius": 20, "color": (100, 200, 0)},
    3003: {"name": "Imp", "cat": "enemy", "health": 60, "damage": 3, "speed": 1.2, "radius": 20, "color": (180, 80, 30)},
    3004: {"name": "Demon", "cat": "enemy", "health": 150, "damage": 10, "speed": 1.8, "radius": 30, "color": (200, 50, 50)},
    3005: {"name": "Lost Soul", "cat": "enemy", "health": 100, "damage": 5, "speed": 2.0, "radius": 16, "color": (200, 200, 200)},
    3006: {"name": "Baron of Hell", "cat": "enemy", "health": 1000, "damage": 12, "speed": 1.2, "radius": 24, "color": (200, 0, 0)},
    3007: {"name": "Cacodemon", "cat": "enemy", "health": 400, "damage": 8, "speed": 1.0, "radius": 31, "color": (200, 0, 0)},
    3008: {"name": "Spider Mastermind", "cat": "enemy", "health": 3000, "damage": 15, "speed": 1.0, "radius": 40, "color": (100, 100, 100)},
    3009: {"name": "Cyberdemon", "cat": "enemy", "health": 4000, "damage": 20, "speed": 1.2, "radius": 40, "color": (180, 100, 80)},
}

TEXTURE_COLORS = {
    "BROWN1": (101, 67, 33), "BROWN99": (139, 90, 43), "STARTAN": (160, 160, 160),
    "STARG3": (128, 128, 128), "METAL": (100, 100, 110), "COMP": (80, 80, 140),
    "TEK": (50, 50, 180), "BRIK": (160, 80, 60), "STONE": (128, 128, 112),
    "MARBLE": (80, 160, 80), "SKIN": (180, 100, 100), "WOOD": (120, 80, 40),
    "EXIT": (200, 200, 0), "DOOR": (160, 140, 60), "LITE": (200, 200, 100),
    "NUKAGE": (0, 180, 0), "WATER": (0, 80, 180), "BLOOD": (180, 0, 0),
    "FLAT": (90, 90, 90), "CEIL": (70, 70, 70), "ROCK": (120, 80, 40),
}


def get_texture_color(texture_name):
    if not texture_name:
        return (128, 128, 128)
    name = texture_name.upper()
    for tex, color in TEXTURE_COLORS.items():
        if tex in name:
            return color
    h = hash(name) & 0xFFFFFF
    return (max(40, (h >> 16) & 0xFF), max(40, (h >> 8) & 0xFF), max(40, h & 0xFF))


def get_sector_floor_color(sector):
    light = sector.get("light", 160)
    mul = light / 255.0
    name = sector.get("floor_tex", "").upper()
    if "NUKAGE" in name or "SLIME" in name:
        return (int(0 * mul), int(180 * mul), int(0 * mul))
    if "BLOOD" in name:
        return (int(180 * mul), int(0 * mul), int(0 * mul))
    if "WATER" in name or "FLAT14" in name:
        return (int(40 * mul), int(40 * mul), int(160 * mul))
    if "FLAT5" in name or "FLAT10" in name:
        return (int(100 * mul), int(100 * mul), int(100 * mul))
    if "RROCK" in name or "BROWN" in name:
        return (int(120 * mul), int(70 * mul), int(30 * mul))
    return (int(80 * mul), int(80 * mul), int(80 * mul))


def get_sector_ceil_color(sector):
    light = sector.get("light", 160)
    mul = light / 255.0
    name = sector.get("ceil_tex", "").upper()
    if "TEK" in name or "COMP" in name:
        return (int(30 * mul), int(30 * mul), int(120 * mul))
    if "LITE" in name or "LIGHT" in name:
        return (int(200 * mul), int(200 * mul), int(100 * mul))
    if "FLAT" in name or "CEIL" in name:
        return (int(60 * mul), int(60 * mul), int(60 * mul))
    return (int(50 * mul), int(50 * mul), int(50 * mul))


class WADParser:
    def __init__(self):
        self.header = None
        self.directory = []
        self.data = None

    def parse(self, filename):
        with open(filename, "rb") as f:
            self.data = f.read()
        sig = self.data[0:4].decode("ascii", errors="replace")
        num_lumps = struct.unpack_from("<I", self.data, 4)[0]
        dir_offset = struct.unpack_from("<I", self.data, 8)[0]
        self.header = {"signature": sig, "num_lumps": num_lumps, "directory_offset": dir_offset}
        self.directory = []
        for i in range(num_lumps):
            off = dir_offset + i * 16
            lump_off = struct.unpack_from("<I", self.data, off)[0]
            lump_size = struct.unpack_from("<I", self.data, off + 4)[0]
            lump_name = self.data[off + 8:off + 16].split(b"\x00")[0].decode("ascii", errors="replace")
            self.directory.append({"name": lump_name, "offset": lump_off, "size": lump_size})
        return True

    def get_lump_index(self, name):
        for i, e in enumerate(self.directory):
            if e["name"] == name:
                return i
        return -1

    def get_lump_data(self, name):
        for e in self.directory:
            if e["name"] == name:
                return self.data[e["offset"]:e["offset"] + e["size"]]
        return None

    def get_lump_by_index(self, idx):
        if 0 <= idx < len(self.directory):
            e = self.directory[idx]
            return self.data[e["offset"]:e["offset"] + e["size"]]
        return None

    def get_map_names(self):
        maps = []
        for prefix in ["E", "MAP"]:
            if prefix == "E":
                for ep in range(1, 5):
                    for ml in range(1, 10):
                        n = f"E{ep}M{ml}"
                        if self.get_lump_index(n) >= 0:
                            maps.append(n)
            else:
                for ml in range(1, 33):
                    n = f"MAP{ml:02d}"
                    if self.get_lump_index(n) >= 0:
                        maps.append(n)
        return maps

    def get_map_data(self, mapname):
        midx = self.get_lump_index(mapname)
        if midx < 0:
            return None
        lump_names = ["THINGS", "LINEDEFS", "SIDEDEFS", "VERTEXES", "SEGS", "SSECTORS",
                      "NODES", "SECTORS", "BLOCKMAP", "REJECT"]
        lumps = {}
        for i, name in enumerate(lump_names):
            idx = midx + 1 + i
            if idx < len(self.directory) and self.directory[idx]["name"] == name:
                lumps[name] = self.get_lump_by_index(idx)
        return self._parse_map(lumps)

    def _parse_map(self, lumps):
        result = {}

        if "THINGS" in lumps and lumps["THINGS"]:
            things = []
            data = lumps["THINGS"]
            for i in range(0, len(data) - 9, 10):
                x, y, angle, tid, flags = struct.unpack_from("<5h", data, i)
                info = THING_TYPES.get(tid, {"name": f"Thing {tid}", "cat": "unknown"})
                things.append({"x": float(x), "y": float(y), "angle": angle,
                               "type": tid, "flags": flags, "info": info})
            result["things"] = things

        if "LINEDEFS" in lumps and lumps["LINEDEFS"]:
            linedefs = []
            data = lumps["LINEDEFS"]
            for i in range(0, len(data) - 13, 14):
                v1, v2, flags, special, tag, right, left = struct.unpack_from("<7h", data, i)
                linedefs.append({"v1": v1, "v2": v2, "flags": flags,
                                 "special": special, "tag": tag,
                                 "right": right, "left": left})
            result["linedefs"] = linedefs

        if "SIDEDEFS" in lumps and lumps["SIDEDEFS"]:
            sidedefs = []
            data = lumps["SIDEDEFS"]
            for i in range(0, len(data) - 29, 30):
                xo, yo = struct.unpack_from("<2h", data, i)
                upper = data[i + 4:i + 12].split(b"\x00")[0].decode("ascii", errors="replace")
                lower = data[i + 12:i + 20].split(b"\x00")[0].decode("ascii", errors="replace")
                middle = data[i + 20:i + 28].split(b"\x00")[0].decode("ascii", errors="replace")
                sector = struct.unpack_from("<H", data, i + 28)[0]
                sidedefs.append({"x_off": xo, "y_off": yo, "upper": upper,
                                 "lower": lower, "middle": middle, "sector": sector})
            result["sidedefs"] = sidedefs

        if "VERTEXES" in lumps and lumps["VERTEXES"]:
            vertexes = []
            data = lumps["VERTEXES"]
            for i in range(0, len(data) - 3, 4):
                x, y = struct.unpack_from("<2h", data, i)
                vertexes.append({"x": float(x), "y": float(y)})
            result["vertexes"] = vertexes

        if "SECTORS" in lumps and lumps["SECTORS"]:
            sectors = []
            data = lumps["SECTORS"]
            for i in range(0, len(data) - 25, 26):
                fh, ch = struct.unpack_from("<2h", data, i)
                ft = data[i + 4:i + 12].split(b"\x00")[0].decode("ascii", errors="replace")
                ct = data[i + 12:i + 20].split(b"\x00")[0].decode("ascii", errors="replace")
                light, special, tag = struct.unpack_from("<3h", data, i + 20)
                sectors.append({"floor_h": fh, "ceil_h": ch, "floor_tex": ft,
                                "ceil_tex": ct, "light": max(32, light),
                                "special": special, "tag": tag})
            result["sectors"] = sectors

        if "BLOCKMAP" in lumps and lumps["BLOCKMAP"]:
            result["blockmap_raw"] = lumps["BLOCKMAP"]

        return result
