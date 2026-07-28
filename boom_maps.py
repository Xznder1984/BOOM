"""BOOM Built-in Maps - Fallback maps when no WAD file is provided."""


def get_builtin_map(map_name="E1M1"):
    maps = {
        "E1M1": _e1m1(),
        "E1M2": _e1m2(),
        "E1M3": _e1m3(),
    }
    return maps.get(map_name, _e1m1())


def get_builtin_map_names():
    return ["E1M1", "E1M2", "E1M3"]


def _e1m1():
    w = 40
    h = 40
    walls = [[None] * h for _ in range(w)]

    def fill_room(x1, y1, x2, y2, sector):
        for x in range(x1, x2 + 1):
            for y in range(y1, y2 + 1):
                if x == x1 or x == x2 or y == y1 or y == y2:
                    if 0 <= x < w and 0 <= y < h:
                        walls[x][y] = sector

    def fill_rect(x1, y1, x2, y2):
        for x in range(x1, x2 + 1):
            for y in range(y1, y2 + 1):
                if 0 <= x < w and 0 <= y < h:
                    walls[x][y] = None

    metal = {"wc": (140, 140, 150), "fc": (60, 60, 60), "cc": (30, 30, 30), "light": 160}
    brown = {"wc": (140, 90, 50), "fc": (80, 50, 25), "cc": (40, 30, 20), "light": 140}
    green = {"wc": (50, 140, 50), "fc": (30, 80, 30), "cc": (20, 40, 20), "light": 120}
    red = {"wc": (160, 50, 50), "fc": (80, 25, 25), "cc": (40, 15, 15), "light": 100}
    tech = {"wc": (50, 50, 180), "fc": (25, 25, 80), "cc": (15, 15, 120), "light": 180}
    dark = {"wc": (80, 80, 80), "fc": (40, 40, 40), "cc": (20, 20, 20), "light": 80}
    hell = {"wc": (160, 60, 40), "fc": (100, 30, 20), "cc": (60, 20, 10), "light": 100}
    door_s = {"wc": (180, 160, 60), "fc": (60, 60, 60), "cc": (30, 30, 30), "light": 160}

    fill_room(1, 1, 14, 14, metal)
    fill_rect(2, 2, 13, 13)
    fill_room(14, 6, 22, 14, brown)
    fill_rect(15, 7, 21, 13)
    fill_room(22, 1, 32, 12, green)
    fill_rect(23, 2, 31, 11)
    fill_room(22, 14, 32, 24, red)
    fill_rect(23, 15, 31, 23)
    fill_room(6, 18, 18, 28, tech)
    fill_rect(7, 19, 17, 27)
    fill_room(18, 28, 32, 36, hell)
    fill_rect(19, 29, 31, 35)
    fill_room(2, 28, 8, 36, dark)
    fill_rect(3, 29, 7, 35)

    walls[14][9] = door_s
    walls[22][6] = door_s
    walls[22][19] = door_s
    walls[18][23] = door_s
    walls[8][28] = door_s
    walls[10][18] = door_s

    sectors = [metal, brown, green, red, tech, dark, hell]

    things = [
        {"x": 6 * 64 + 32, "y": 7 * 64 + 32, "angle": 0, "type": 1, "flags": 0,
         "info": {"name": "Player 1 Start", "cat": "start"}},
        {"x": 18 * 64 + 32, "y": 10 * 64 + 32, "angle": 270, "type": 3001, "flags": 0,
         "info": {"name": "Zombieman", "cat": "enemy", "health": 20, "damage": 3, "speed": 1.5, "radius": 20, "color": (0, 200, 0)}},
        {"x": 27 * 64 + 32, "y": 6 * 64 + 32, "angle": 180, "type": 3001, "flags": 0,
         "info": {"name": "Zombieman", "cat": "enemy", "health": 20, "damage": 3, "speed": 1.5, "radius": 20, "color": (0, 200, 0)}},
        {"x": 27 * 64 + 32, "y": 9 * 64 + 32, "angle": 180, "type": 3003, "flags": 0,
         "info": {"name": "Imp", "cat": "enemy", "health": 60, "damage": 3, "speed": 1.2, "radius": 20, "color": (180, 80, 30)}},
        {"x": 27 * 64 + 32, "y": 19 * 64 + 32, "angle": 90, "type": 3002, "flags": 0,
         "info": {"name": "Shotgun Guy", "cat": "enemy", "health": 30, "damage": 5, "speed": 1.5, "radius": 20, "color": (100, 200, 0)}},
        {"x": 12 * 64 + 32, "y": 23 * 64 + 32, "angle": 0, "type": 3003, "flags": 0,
         "info": {"name": "Imp", "cat": "enemy", "health": 60, "damage": 3, "speed": 1.2, "radius": 20, "color": (180, 80, 30)}},
        {"x": 12 * 64 + 32, "y": 24 * 64 + 32, "angle": 0, "type": 3003, "flags": 0,
         "info": {"name": "Imp", "cat": "enemy", "health": 60, "damage": 3, "speed": 1.2, "radius": 20, "color": (180, 80, 30)}},
        {"x": 25 * 64 + 32, "y": 32 * 64 + 32, "angle": 270, "type": 3004, "flags": 0,
         "info": {"name": "Demon", "cat": "enemy", "health": 150, "damage": 10, "speed": 1.8, "radius": 30, "color": (200, 50, 50)}},
        {"x": 28 * 64 + 32, "y": 32 * 64 + 32, "angle": 270, "type": 3004, "flags": 0,
         "info": {"name": "Demon", "cat": "enemy", "health": 150, "damage": 10, "speed": 1.8, "radius": 30, "color": (200, 50, 50)}},
        {"x": 5 * 64 + 32, "y": 32 * 64 + 32, "angle": 0, "type": 3007, "flags": 0,
         "info": {"name": "Cacodemon", "cat": "enemy", "health": 400, "damage": 8, "speed": 1.0, "radius": 31, "color": (200, 0, 0)}},
        {"x": 8 * 64 + 32, "y": 5 * 64 + 32, "angle": 0, "type": 8, "flags": 0,
         "info": {"name": "Clip", "cat": "ammo", "type": "bullets", "amount": 10, "color": (200, 200, 50)}},
        {"x": 10 * 64 + 32, "y": 5 * 64 + 32, "angle": 0, "type": 10, "flags": 0,
         "info": {"name": "Shells", "cat": "ammo", "type": "shells", "amount": 4, "color": (200, 200, 50)}},
        {"x": 18 * 64 + 32, "y": 5 * 64 + 32, "angle": 0, "type": 2007, "flags": 0,
         "info": {"name": "Shotgun", "cat": "weapon", "wtype": "shotgun", "slot": 3, "color": (0, 200, 200)}},
        {"x": 27 * 64 + 32, "y": 4 * 64 + 32, "angle": 0, "type": 17, "flags": 0,
         "info": {"name": "Stimpack", "cat": "health", "amount": 10, "color": (200, 200, 50)}},
        {"x": 12 * 64 + 32, "y": 22 * 64 + 32, "angle": 0, "type": 18, "flags": 0,
         "info": {"name": "Medikit", "cat": "health", "amount": 25, "color": (200, 200, 50)}},
        {"x": 12 * 64 + 32, "y": 26 * 64 + 32, "angle": 0, "type": 2003, "flags": 0,
         "info": {"name": "Chaingun", "cat": "weapon", "wtype": "chaingun", "slot": 4, "color": (0, 200, 200)}},
        {"x": 27 * 64 + 32, "y": 19 * 64 + 32, "angle": 0, "type": 11, "flags": 0,
         "info": {"name": "Box of Shells", "cat": "ammo", "type": "shells", "amount": 20, "color": (200, 200, 50)}},
        {"x": 25 * 64 + 32, "y": 33 * 64 + 32, "angle": 0, "type": 18, "flags": 0,
         "info": {"name": "Medikit", "cat": "health", "amount": 25, "color": (200, 200, 50)}},
        {"x": 5 * 64 + 32, "y": 33 * 64 + 32, "angle": 0, "type": 21, "flags": 0,
         "info": {"name": "Combat Armor", "cat": "armor", "amount": 200, "color": (200, 200, 50)}},
        {"x": 28 * 64 + 32, "y": 30 * 64 + 32, "angle": 0, "type": 2004, "flags": 0,
         "info": {"name": "Rocket Launcher", "cat": "weapon", "wtype": "rocket", "slot": 5, "color": (0, 200, 200)}},
        {"x": 16 * 64 + 32, "y": 33 * 64 + 32, "angle": 0, "type": 39, "flags": 0,
         "info": {"name": "Red Skull", "cat": "key", "color": "red", "color": (200, 50, 50)}},
        {"x": 30 * 64 + 32, "y": 18 * 64 + 32, "angle": 0, "type": 5, "flags": 0,
         "info": {"name": "Red Keycard", "cat": "key", "color": "red", "color": (200, 50, 50)}},
        {"x": 5 * 64 + 32, "y": 5 * 64 + 32, "angle": 0, "type": 2001, "flags": 0,
         "info": {"name": "Chainsaw", "cat": "weapon", "wtype": "chainsaw", "slot": 1, "color": (0, 200, 200)}},
    ]

    linedefs = []
    sidedefs = []
    vertexes = []
    sectors_list = []

    return {
        "things": things,
        "linedefs": linedefs,
        "sidedefs": sidedefs,
        "vertexes": vertexes,
        "sectors": sectors_list,
        "_builtin_walls": walls,
        "_builtin_sectors": sectors,
        "_builtin_start": (6 * 64 + 32, 7 * 64 + 32, 0.0),
        "_builtin_name": "E1M1 - Hangar (Built-in)",
    }


def _e1m2():
    w = 36
    h = 36
    walls = [[None] * h for _ in range(w)]

    metal = {"wc": (160, 160, 170), "fc": (70, 70, 70), "cc": (35, 35, 35), "light": 150}
    brown = {"wc": (150, 100, 55), "fc": (85, 55, 30), "cc": (45, 30, 15), "light": 130}
    green = {"wc": (60, 150, 60), "fc": (35, 85, 35), "cc": (20, 45, 20), "light": 110}
    tech = {"wc": (60, 60, 190), "fc": (30, 30, 85), "cc": (20, 20, 130), "light": 170}
    door_s = {"wc": (180, 160, 60), "fc": (60, 60, 60), "cc": (30, 30, 30), "light": 160}

    for x in range(1, 18):
        for y in range(1, 18):
            if x == 1 or x == 17 or y == 1 or y == 17:
                walls[x][y] = metal
            else:
                walls[x][y] = None

    for x in range(17, 35):
        for y in range(1, 18):
            if x == 17 or x == 34 or y == 1 or y == 17:
                walls[x][y] = brown
            else:
                walls[x][y] = None

    for x in range(1, 18):
        for y in range(17, 35):
            if x == 1 or x == 17 or y == 17 or y == 34:
                walls[x][y] = green
            else:
                walls[x][y] = None

    for x in range(17, 35):
        for y in range(17, 35):
            if x == 17 or x == 34 or y == 17 or y == 34:
                walls[x][y] = tech
            else:
                walls[x][y] = None

    walls[17][9] = door_s
    walls[9][17] = door_s
    walls[17][25] = door_s

    things = [
        {"x": 5 * 64 + 32, "y": 5 * 64 + 32, "angle": 0, "type": 1, "flags": 0,
         "info": {"name": "Player 1 Start", "cat": "start"}},
        {"x": 25 * 64 + 32, "y": 5 * 64 + 32, "angle": 180, "type": 3001, "flags": 0,
         "info": {"name": "Zombieman", "cat": "enemy", "health": 20, "damage": 3, "speed": 1.5, "radius": 20, "color": (0, 200, 0)}},
        {"x": 25 * 64 + 32, "y": 12 * 64 + 32, "angle": 90, "type": 3003, "flags": 0,
         "info": {"name": "Imp", "cat": "enemy", "health": 60, "damage": 3, "speed": 1.2, "radius": 20, "color": (180, 80, 30)}},
        {"x": 5 * 64 + 32, "y": 25 * 64 + 32, "angle": 0, "type": 3004, "flags": 0,
         "info": {"name": "Demon", "cat": "enemy", "health": 150, "damage": 10, "speed": 1.8, "radius": 30, "color": (200, 50, 50)}},
        {"x": 25 * 64 + 32, "y": 25 * 64 + 32, "angle": 270, "type": 3007, "flags": 0,
         "info": {"name": "Cacodemon", "cat": "enemy", "health": 400, "damage": 8, "speed": 1.0, "radius": 31, "color": (200, 0, 0)}},
        {"x": 10 * 64 + 32, "y": 10 * 64 + 32, "angle": 0, "type": 2007, "flags": 0,
         "info": {"name": "Shotgun", "cat": "weapon", "wtype": "shotgun", "slot": 3, "color": (0, 200, 200)}},
        {"x": 10 * 64 + 32, "y": 12 * 64 + 32, "angle": 0, "type": 18, "flags": 0,
         "info": {"name": "Medikit", "cat": "health", "amount": 25, "color": (200, 200, 50)}},
        {"x": 28 * 64 + 32, "y": 28 * 64 + 32, "angle": 0, "type": 2005, "flags": 0,
         "info": {"name": "Plasma Rifle", "cat": "weapon", "wtype": "plasma", "slot": 6, "color": (0, 200, 200)}},
        {"x": 28 * 64 + 32, "y": 26 * 64 + 32, "angle": 0, "type": 15, "flags": 0,
         "info": {"name": "Cell Pack", "cat": "ammo", "type": "cells", "amount": 200, "color": (200, 200, 50)}},
        {"x": 10 * 64 + 32, "y": 28 * 64 + 32, "angle": 0, "type": 2003, "flags": 0,
         "info": {"name": "Chaingun", "cat": "weapon", "wtype": "chaingun", "slot": 4, "color": (0, 200, 200)}},
        {"x": 10 * 64 + 32, "y": 30 * 64 + 32, "angle": 0, "type": 9, "flags": 0,
         "info": {"name": "Box of Bullets", "cat": "ammo", "type": "bullets", "amount": 50, "color": (200, 200, 50)}},
    ]

    return {
        "things": things, "linedefs": [], "sidedefs": [], "vertexes": [], "sectors": [],
        "_builtin_walls": walls, "_builtin_sectors": [metal, brown, green, tech],
        "_builtin_start": (5 * 64 + 32, 5 * 64 + 32, 0.0),
        "_builtin_name": "E1M2 - Nuclear Plant (Built-in)",
    }


def _e1m3():
    w = 32
    h = 44
    walls = [[None] * h for _ in range(w)]

    metal = {"wc": (130, 130, 140), "fc": (55, 55, 55), "cc": (28, 28, 28), "light": 155}
    brown = {"wc": (145, 95, 52), "fc": (82, 52, 28), "cc": (42, 28, 15), "light": 135}
    red = {"wc": (170, 55, 55), "fc": (85, 28, 28), "cc": (45, 15, 15), "light": 95}
    dark = {"wc": (75, 75, 75), "fc": (38, 38, 38), "cc": (18, 18, 18), "light": 70}
    door_s = {"wc": (180, 160, 60), "fc": (60, 60, 60), "cc": (30, 30, 30), "light": 160}

    for x in range(1, 16):
        for y in range(1, 22):
            if x == 1 or x == 15 or y == 1 or y == 21:
                walls[x][y] = metal
            else:
                walls[x][y] = None
    for x in range(16, 31):
        for y in range(1, 22):
            if x == 16 or x == 30 or y == 1 or y == 21:
                walls[x][y] = brown
            else:
                walls[x][y] = None
    for x in range(1, 16):
        for y in range(22, 43):
            if x == 1 or x == 15 or y == 22 or y == 42:
                walls[x][y] = red
            else:
                walls[x][y] = None
    for x in range(16, 31):
        for y in range(22, 43):
            if x == 16 or x == 30 or y == 22 or y == 42:
                walls[x][y] = dark
            else:
                walls[x][y] = None

    walls[15][10] = door_s
    walls[8][22] = door_s
    walls[15][32] = door_s

    things = [
        {"x": 5 * 64 + 32, "y": 5 * 64 + 32, "angle": 0, "type": 1, "flags": 0,
         "info": {"name": "Player 1 Start", "cat": "start"}},
        {"x": 22 * 64 + 32, "y": 10 * 64 + 32, "angle": 180, "type": 3003, "flags": 0,
         "info": {"name": "Imp", "cat": "enemy", "health": 60, "damage": 3, "speed": 1.2, "radius": 20, "color": (180, 80, 30)}},
        {"x": 22 * 64 + 32, "y": 14 * 64 + 32, "angle": 180, "type": 3003, "flags": 0,
         "info": {"name": "Imp", "cat": "enemy", "health": 60, "damage": 3, "speed": 1.2, "radius": 20, "color": (180, 80, 30)}},
        {"x": 5 * 64 + 32, "y": 30 * 64 + 32, "angle": 0, "type": 3004, "flags": 0,
         "info": {"name": "Demon", "cat": "enemy", "health": 150, "damage": 10, "speed": 1.8, "radius": 30, "color": (200, 50, 50)}},
        {"x": 22 * 64 + 32, "y": 30 * 64 + 32, "angle": 90, "type": 3006, "flags": 0,
         "info": {"name": "Baron of Hell", "cat": "enemy", "health": 1000, "damage": 12, "speed": 1.2, "radius": 24, "color": (200, 0, 0)}},
        {"x": 12 * 64 + 32, "y": 18 * 64 + 32, "angle": 0, "type": 2007, "flags": 0,
         "info": {"name": "Shotgun", "cat": "weapon", "wtype": "shotgun", "slot": 3, "color": (0, 200, 200)}},
        {"x": 22 * 64 + 32, "y": 5 * 64 + 32, "angle": 0, "type": 18, "flags": 0,
         "info": {"name": "Medikit", "cat": "health", "amount": 25, "color": (200, 200, 50)}},
        {"x": 12 * 64 + 32, "y": 38 * 64 + 32, "angle": 0, "type": 2006, "flags": 0,
         "info": {"name": "BFG 9000", "cat": "weapon", "wtype": "bfg", "slot": 7, "color": (0, 200, 200)}},
        {"x": 10 * 64 + 32, "y": 38 * 64 + 32, "angle": 0, "type": 15, "flags": 0,
         "info": {"name": "Cell Pack", "cat": "ammo", "type": "cells", "amount": 200, "color": (200, 200, 50)}},
        {"x": 14 * 64 + 32, "y": 38 * 64 + 32, "angle": 0, "type": 15, "flags": 0,
         "info": {"name": "Cell Pack", "cat": "ammo", "type": "cells", "amount": 200, "color": (200, 200, 50)}},
        {"x": 22 * 64 + 32, "y": 38 * 64 + 32, "angle": 0, "type": 21, "flags": 0,
         "info": {"name": "Combat Armor", "cat": "armor", "amount": 200, "color": (200, 200, 50)}},
    ]

    return {
        "things": things, "linedefs": [], "sidedefs": [], "vertexes": [], "sectors": [],
        "_builtin_walls": walls, "_builtin_sectors": [metal, brown, red, dark],
        "_builtin_start": (5 * 64 + 32, 5 * 64 + 32, 0.0),
        "_builtin_name": "E1M3 - Toxin Refinery (Built-in)",
    }
