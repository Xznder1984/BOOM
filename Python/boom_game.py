"""BOOM Game Logic - Player, enemies, items, doors, collision."""
import math
import random

WEAPON_DEFS = {
    "fist":     {"slot": 1, "damage": 10, "ammo_type": None, "range": 64, "fire_rate": 0.4, "auto": False, "spread": 0},
    "pistol":   {"slot": 2, "damage": 5,  "ammo_type": "bullets", "range": 2000, "fire_rate": 0.15, "auto": True, "spread": 0.02},
    "shotgun":  {"slot": 3, "damage": 7,  "ammo_type": "shells", "range": 1000, "fire_rate": 0.8, "auto": False, "spread": 0.08, "pellets": 7},
    "chaingun": {"slot": 4, "damage": 5,  "ammo_type": "bullets", "range": 2000, "fire_rate": 0.07, "auto": True, "spread": 0.04},
    "rocket":   {"slot": 5, "damage": 20, "ammo_type": "rockets", "range": 2000, "fire_rate": 0.9, "auto": False, "spread": 0},
    "plasma":   {"slot": 6, "damage": 8,  "ammo_type": "cells", "range": 2000, "fire_rate": 0.08, "auto": True, "spread": 0.02},
    "bfg":      {"slot": 7, "damage": 60, "ammo_type": "cells", "range": 2000, "fire_rate": 1.5, "auto": False, "spread": 0.05, "ammo_use": 40},
}

MAX_AMMO = {"bullets": 200, "shells": 50, "rockets": 50, "cells": 300}


def create_player(x, y, angle=0):
    return {
        "x": float(x), "y": float(y), "angle": angle,
        "health": 100, "armor": 0, "weapon": "pistol",
        "weapons": {"fist": True, "pistol": True},
        "ammo": {"bullets": 50, "shells": 0, "rockets": 0, "cells": 0},
        "max_ammo": {"bullets": 200, "shells": 50, "rockets": 50, "cells": 300},
        "keys": set(),
        "attack_timer": 0, "pain_timer": 0,
        "move_speed": 0, "bob": 0.0,
        "kills": 0, "items": 0, "secrets": 0,
    }


def build_wall_grid(map_data, fallback_sectors):
    vertexes = map_data.get("vertexes", [])
    linedefs = map_data.get("linedefs", [])
    sidedefs = map_data.get("sidedefs", [])
    sectors = map_data.get("sectors", [])

    if not vertexes or not linedefs:
        return None, [], {}, []

    all_x = [v["x"] for v in vertexes]
    all_y = [v["y"] for v in vertexes]
    min_x, max_x = min(all_x), max(all_x)
    min_y, max_y = min(all_y), max(all_y)

    map_w = int(max_x - min_x) + 2
    map_h = int(max_y - min_y) + 2
    ox, oy = min_x - 0.5, min_y - 0.5

    walls = [[None] * map_h for _ in range(map_w)]
    sector_colors = {}

    for i, s in enumerate(sectors):
        from boom_wad import get_sector_floor_color, get_sector_ceil_color
        fc = get_sector_floor_color(s)
        cc = get_sector_ceil_color(s)
        sector_colors[i] = {"fc": fc, "cc": cc, "light": s.get("light", 160)}

    for ld in linedefs:
        v1 = vertexes[ld["v1"]] if ld["v1"] < len(vertexes) else None
        v2 = vertexes[ld["v2"]] if ld["v2"] < len(vertexes) else None
        if not v1 or not v2:
            continue

        sector = None
        wc = None
        if ld["right"] >= 0 and ld["right"] < len(sidedefs):
            sd = sidedefs[ld["right"]]
            si = sd["sector"]
            if si < len(sectors):
                sector = sectors[si]
                wc_name = sd.get("middle", "") or sd.get("upper", "") or sd.get("lower", "")
                from boom_wad import get_texture_color
                wc = get_texture_color(wc_name)
                if si in sector_colors:
                    lc = sector_colors[si].get("light", 160)
                    lm = min(1.0, lc / 200.0 + 0.2)
                    wc = (int(wc[0] * lm), int(wc[1] * lm), int(wc[2] * lm))
        elif ld["left"] >= 0 and ld["left"] < len(sidedefs):
            sd = sidedefs[ld["left"]]
            si = sd["sector"]
            if si < len(sectors):
                sector = sectors[si]
                wc_name = sd.get("middle", "") or sd.get("upper", "") or sd.get("lower", "")
                from boom_wad import get_texture_color
                wc = get_texture_color(wc_name)

        sc = None
        if sector:
            si_idx = -1
            for idx, s in enumerate(sectors):
                if s is sector:
                    si_idx = idx
                    break
            if si_idx in sector_colors:
                sc = sector_colors[si_idx].copy()
                if wc:
                    sc["wc"] = wc

        x1, y1 = int(v1["x"] - ox), int(v1["y"] - oy)
        x2, y2 = int(v2["x"] - ox), int(v2["y"] - oy)

        _rasterize_line(walls, x1, y1, x2, y2, sc, map_w, map_h)

    _fill_floor_colors(walls, sector_colors, map_w, map_h)

    return walls, sector_colors, {"width": map_w, "height": map_h, "ox": ox, "oy": oy}, list(sector_colors.values())


def _rasterize_line(walls, x1, y1, x2, y2, sector, mw, mh):
    dx = abs(x2 - x1)
    dy = -abs(y2 - y1)
    sx = 1 if x1 < x2 else -1
    sy = 1 if y1 < y2 else -1
    err = dx + dy
    steps = max(abs(x2 - x1), abs(y2 - y1)) + 1
    for _ in range(steps):
        if 0 <= x1 < mw and 0 <= y1 < mh:
            if not walls[x1][y1]:
                walls[x1][y1] = sector or {"wc": (128, 128, 128), "fc": (80, 80, 80), "cc": (40, 40, 40), "light": 160}
        if x1 == x2 and y1 == y2:
            break
        e2 = 2 * err
        if e2 >= dy:
            err += dy
            x1 += sx
        if e2 <= dx:
            err += dx
            y1 += sy


def _fill_floor_colors(walls, sector_colors, mw, mh):
    default = {"wc": (128, 128, 128), "fc": (80, 80, 80), "cc": (40, 40, 40), "light": 160}
    for x in range(mw):
        for y in range(mh):
            if not walls[x][y]:
                walls[x][y] = default


def check_collision(x, y, walls, radius=10):
    ix, iy = int(x), int(y)
    for dx in range(-1, 2):
        for dy in range(-1, 2):
            nx, ny = ix + dx, iy + dy
            if 0 <= nx < len(walls) and 0 <= ny < len(walls[0]):
                if walls[nx][ny]:
                    cell_x, cell_y = nx + 0.5, ny + 0.5
                    dd_x = x - cell_x * 64 if False else x - (nx * 64 + 32)
                    dd_y = y - (ny * 64 + 32)
                    dist = math.sqrt(dd_x * dd_x + dd_y * dd_y)
                    if dist < radius + 20:
                        return True
    return False


def move_player(player, walls, forward, strafe, dt):
    speed = PLAYER_SPEED * dt * 60
    new_x = player["x"]
    new_y = player["y"]

    if forward != 0:
        new_x += math.cos(player["angle"]) * forward * speed
        new_y += math.sin(player["angle"]) * forward * speed
    if strafe != 0:
        new_x += math.cos(player["angle"] + math.pi / 2) * strafe * speed
        new_y += math.sin(player["angle"] + math.pi / 2) * strafe * speed

    if not check_collision(new_x, player["y"], walls):
        player["x"] = new_x
    if not check_collision(player["x"], new_y, walls):
        player["y"] = new_y

    if forward != 0 or strafe != 0:
        player["bob"] += dt * 10
    else:
        player["bob"] *= 0.9


def try_use_door(player, map_data, doors):
    dx = math.cos(player["angle"]) * 48
    dy = math.sin(player["angle"]) * 48
    tx = int((player["x"] + dx) / 64)
    ty = int((player["y"] + dy) / 64)

    key = (tx, ty)
    if key in doors:
        door = doors[key]
        if door.get("locked"):
            needed = door.get("lock_color")
            if needed and needed not in player["keys"]:
                return False, f"Need {needed} key!"
        door["opening"] = True
        door["timer"] = 0
        return True, "Door opened!"
    return False, ""


def try_use_switch(player, map_data):
    dx = math.cos(player["angle"]) * 48
    dy = math.sin(player["angle"]) * 48
    tx = int((player["x"] + dx) / 64)
    ty = int((player["y"] + dy) / 64)
    return False, ""


def fire_weapon(player, walls, enemies):
    weapon = player.get("weapon", "pistol")
    wdef = WEAPON_DEFS.get(weapon)
    if not wdef:
        return None, "No weapon!"

    ammo_type = wdef.get("ammo_type")
    ammo_use = wdef.get("ammo_use", 1)
    if ammo_type:
        if player["ammo"].get(ammo_type, 0) < ammo_use:
            return None, ""
        player["ammo"][ammo_type] -= ammo_use

    pellets = wdef.get("pellets", 1)
    damage = wdef["damage"]
    spread = wdef["spread"]
    hit_enemy = None

    for _ in range(pellets):
        angle = player["angle"] + random.uniform(-spread, spread)
        best_dist = wdef["range"]
        for e in enemies:
            if not e.get("alive", True):
                continue
            dx = e["x"] - player["x"]
            dy = e["y"] - player["y"]
            dist = math.sqrt(dx * dx + dy * dy)
            if dist > wdef["range"]:
                continue
            e_angle = math.atan2(dy, dx)
            diff = angle - e_angle
            while diff > math.pi:
                diff -= 2 * math.pi
            while diff < -math.pi:
                diff += 2 * math.pi
            hit_width = e.get("radius", 20) / dist
            if abs(diff) < hit_width + 0.1 and dist < best_dist:
                best_dist = dist
                hit_enemy = e

    if hit_enemy:
        hit_enemy["health"] = hit_enemy.get("health", 0) - damage
        hit_enemy["pain_timer"] = 0.2
        hit_enemy["state"] = "pain"
        if hit_enemy["health"] <= 0:
            hit_enemy["alive"] = False
            hit_enemy["state"] = "dead"
            return hit_enemy, f"Killed {hit_enemy.get('name', 'enemy')}!"
        return hit_enemy, f"Hit {hit_enemy.get('name', 'enemy')}!"
    return None, ""


def update_enemies(enemies, player, walls, dt):
    for e in enemies:
        if not e.get("alive", True):
            continue
        if e.get("cat") != "enemy":
            continue

        state = e.get("state", "idle")
        e["pain_timer"] = max(0, e.get("pain_timer", 0) - dt)
        e["attack_cooldown"] = max(0, e.get("attack_cooldown", 0) - dt)

        if e["pain_timer"] > 0:
            e["state"] = "pain"
            continue

        dx = player["x"] - e["x"]
        dy = player["y"] - e["y"]
        dist = math.sqrt(dx * dx + dy * dy)

        if dist < 400:
            e["state"] = "chase"
        elif dist > 600:
            e["state"] = "idle"

        if e["state"] == "chase":
            speed = e.get("speed", 1.5) * dt * 60 * 16
            if dist > 48:
                move_x = (dx / dist) * speed
                move_y = (dy / dist) * speed
                nx, ny = e["x"] + move_x, e["y"] + move_y
                if not check_collision(nx, e["y"], walls, e.get("radius", 20)):
                    e["x"] = nx
                if not check_collision(e["x"], ny, walls, e.get("radius", 20)):
                    e["y"] = ny

            if dist < 80 and e.get("attack_cooldown", 0) <= 0:
                damage = e.get("damage", 3)
                player["health"] -= damage
                player["pain_timer"] = 0.3
                e["attack_cooldown"] = 1.0
                if player["health"] <= 0:
                    player["health"] = 0


def update_doors(doors, dt):
    for key, door in doors.items():
        if door.get("opening"):
            door["timer"] = door.get("timer", 0) + dt
            door["open_amount"] = min(1.0, door.get("open_amount", 0) + dt * 2)
            if door["open_amount"] >= 1.0:
                door["opening"] = False
                door["open"] = True
                door["close_timer"] = 3.0
        elif door.get("open") and door.get("close_timer", 0) > 0:
            door["close_timer"] -= dt
            if door["close_timer"] <= 0:
                door["open"] = False
                door["opening"] = False
                door["open_amount"] = 0


def check_item_pickups(player, items):
    picked = []
    for item in items:
        if not item.get("alive", True):
            continue
        dx = player["x"] - item["x"]
        dy = player["y"] - item["y"]
        dist = math.sqrt(dx * dx + dy * dy)
        if dist < 32:
            cat = item.get("cat", "")
            if cat == "health":
                if player["health"] < 100:
                    player["health"] = min(100, player["health"] + item.get("amount", 10))
                    item["alive"] = False
                    picked.append(item)
            elif cat == "health_bonus":
                if player["health"] < 200:
                    player["health"] = min(200, player["health"] + item.get("amount", 1))
                    item["alive"] = False
                    picked.append(item)
            elif cat == "armor":
                if player["armor"] < 200:
                    player["armor"] = min(200, player["armor"] + item.get("amount", 100))
                    item["alive"] = False
                    picked.append(item)
            elif cat == "ammo":
                atype = item.get("type", "bullets")
                if player["ammo"].get(atype, 0) < player["max_ammo"].get(atype, 200):
                    player["ammo"][atype] = min(
                        player["max_ammo"].get(atype, 200),
                        player["ammo"].get(atype, 0) + item.get("amount", 10)
                    )
                    item["alive"] = False
                    picked.append(item)
            elif cat == "key":
                color = item.get("color", "")
                if color:
                    player["keys"].add(color)
                    item["alive"] = False
                    picked.append(item)
            elif cat == "weapon":
                wtype = item.get("wtype", "")
                if wtype and wtype not in player["weapons"]:
                    player["weapons"][wtype] = True
                    player["weapon"] = wtype
                    item["alive"] = False
                    picked.append(item)
    return picked


def load_map_entities(map_data):
    things = map_data.get("things", [])
    player_start = (256, 256, 0)
    enemies = []
    items = []
    decorations = []

    for t in things:
        info = t.get("info", {})
        cat = info.get("cat", "unknown")
        x, y = t["x"], t["y"]
        angle_rad = math.radians(t.get("angle", 0))

        if cat == "start":
            player_start = (x, y, angle_rad)
        elif cat == "enemy":
            enemies.append({
                "x": x, "y": y, "cat": "enemy",
                "name": info.get("name", "Monster"),
                "health": info.get("health", 60),
                "damage": info.get("damage", 5),
                "speed": info.get("speed", 1.5),
                "radius": info.get("radius", 20),
                "color": info.get("color", (200, 0, 0)),
                "alive": True, "state": "idle",
                "attack_cooldown": 0, "pain_timer": 0,
            })
        elif cat in ("health", "health_bonus", "armor", "ammo", "key", "backpack"):
            items.append({
                "x": x, "y": y, "cat": cat,
                "name": info.get("name", "Item"),
                "amount": info.get("amount", 10),
                "type": info.get("type", ""),
                "color": info.get("color", (200, 200, 0)) if cat == "key" else (200, 200, 50),
                "alive": True,
            })
        elif cat == "weapon":
            items.append({
                "x": x, "y": y, "cat": "weapon",
                "name": info.get("name", "Weapon"),
                "wtype": info.get("wtype", ""),
                "slot": info.get("slot", 3),
                "color": (0, 200, 200),
                "alive": True,
            })
        elif cat == "decoration":
            decorations.append({
                "x": x, "y": y, "cat": "decoration",
                "color": (100, 100, 100),
                "alive": True,
            })

    return player_start, enemies, items, decorations


def check_exit(player, map_data):
    sector_specials = map_data.get("sectors", [])
    px, py = int(player["x"] / 64), int(player["y"] / 64)
    return player.get("health", 0) > 0 and False
