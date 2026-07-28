"""BOOM Raycasting Engine - Terminal-based 3D renderer."""
import math
import curses

FOV = math.pi / 3
HALF_FOV = FOV / 2
MAX_DEPTH = 2000
DELTA_ANGLE = FOV / 60
PLAYER_SPEED = 3.0
ROT_SPEED = 0.04
STRIFE_SPEED = 2.5
MOUSE_SENS = 0.003
WALL_CHARS = " \u2591\u2592\u2593\u2588"


def get_wall_char(dist_ratio):
    idx = min(len(WALL_CHARS) - 1, int(dist_ratio * (len(WALL_CHARS) - 1)))
    return WALL_CHARS[idx]


class Raycaster:
    def __init__(self, screen_w, screen_h):
        self.screen_w = screen_w
        self.screen_h = screen_h
        self.render_h = screen_h - 4
        self.z_buffer = [MAX_DEPTH] * screen_w

    def resize(self, w, h):
        self.screen_w = w
        self.screen_h = h
        self.render_h = h - 4
        self.z_buffer = [MAX_DEPTH] * w

    def cast_ray(self, px, py, pa, angle_off, walls):
        ray_a = pa + angle_off
        sin_a = math.sin(ray_a)
        cos_a = math.cos(ray_a)
        dx = cos_a if cos_a != 0 else 1e-8
        dy = sin_a if sin_a != 0 else 1e-8

        for depth in range(1, MAX_DEPTH, 2):
            tx = int(px + dx * depth)
            ty = int(py + dy * depth)
            if tx < 0 or ty < 0 or tx >= len(walls) or ty >= len(walls[0]):
                return MAX_DEPTH, (128, 128, 128)
            if walls[tx][ty]:
                sector = walls[tx][ty]
                fixed = depth * math.cos(pa + angle_off - pa)
                return max(1, fixed), sector
        return MAX_DEPTH, (128, 128, 128)

    def render(self, stdscr, player, walls, entities, sector_colors, show_map, messages):
        stdscr.erase()
        px, py, pa = player["x"], player["y"], player["angle"]

        for col in range(self.screen_w):
            angle_off = -HALF_FOV + col * DELTA_ANGLE
            dist, sector = self.cast_ray(px, py, pa, angle_off, walls)
            self.z_buffer[col] = dist

            wall_h = min(self.render_h, int(self.render_h / (dist / 64.0))) if dist > 0 else self.render_h
            wall_h = max(1, wall_h)

            floor_color, ceil_color = (60, 60, 60), (30, 30, 30)
            wall_color = (128, 128, 128)

            if sector:
                floor_color = sector.get("fc", (80, 80, 80))
                ceil_color = sector.get("cc", (40, 40, 40))
                wc = sector.get("wc", None)
                if wc:
                    wall_color = wc
                light = sector.get("light", 160)
                lm = min(1.0, light / 200.0 + 0.2)
                dm = max(0.15, 1.0 - dist / 800.0)
                f = dm * lm
                wall_color = (int(wall_color[0] * f), int(wall_color[1] * f), int(wall_color[2] * f))

            top = (self.render_h - wall_h) // 2
            bot = top + wall_h

            for row in range(self.render_h):
                if row < top:
                    cf = max(0.1, 1.0 - (top - row) / (top + 1))
                    ci = ceil_color
                    ch = get_wall_char(1.0 - cf)
                    color_pair = self._get_color(stdscr, int(ci[0] * cf), int(ci[1] * cf), int(ci[2] * cf), True)
                    try:
                        stdscr.addstr(row, col, ch, curses.color_pair(color_pair))
                    except curses.error:
                        pass
                elif row <= bot:
                    ratio = abs(row - self.render_h // 2) / (wall_h / 2) if wall_h > 0 else 0
                    ch = get_wall_char(min(1.0, dist / 400.0))
                    color_pair = self._get_color(stdscr, int(wall_color[0]), int(wall_color[1]), int(wall_color[2]), False)
                    try:
                        stdscr.addstr(row, col, ch, curses.color_pair(color_pair))
                    except curses.error:
                        pass
                else:
                    ff = max(0.1, 1.0 - (row - bot) / (self.render_h - bot + 1))
                    fc = floor_color
                    ch = get_wall_char(1.0 - ff)
                    color_pair = self._get_color(stdscr, int(fc[0] * ff), int(fc[1] * ff), int(fc[2] * ff), True)
                    try:
                        stdscr.addstr(row, col, ch, curses.color_pair(color_pair))
                    except curses.error:
                        pass

        self._render_sprites(stdscr, player, entities)
        self._render_hud(stdscr, player)
        if show_map:
            self._render_minimap(stdscr, player, walls, entities)
        self._render_messages(stdscr, messages)
        stdscr.refresh()

    def _render_sprites(self, stdscr, player, entities):
        px, py, pa = player["x"], player["y"], player["angle"]
        sorted_ents = []
        for e in entities:
            if not e.get("alive", True) and e.get("cat") != "item":
                continue
            dx = e["x"] - px
            dy = e["y"] - py
            dist = math.sqrt(dx * dx + dy * dy)
            if dist < 1 or dist > 600:
                continue
            angle = math.atan2(dy, dx) - pa
            while angle > math.pi:
                angle -= 2 * math.pi
            while angle < -math.pi:
                angle += 2 * math.pi
            if abs(angle) > HALF_FOV + 0.3:
                continue
            sorted_ents.append((dist, e, angle))
        sorted_ents.sort(key=lambda x: -x[0])

        for dist, e, angle in sorted_ents:
            screen_x = int((angle / FOV + 0.5) * self.screen_w)
            sprite_h = max(2, int(self.render_h / (dist / 32.0)))
            sprite_w = max(1, sprite_h // 2)
            top = (self.render_h - sprite_h) // 2
            color = e.get("color", (200, 0, 0))
            dm = max(0.2, 1.0 - dist / 500.0)
            rc = (int(color[0] * dm), int(color[1] * dm), int(color[2] * dm))

            cat = e.get("cat", "")
            if cat == "enemy":
                ch = "\u2620" if not e.get("alive", True) else "\u2694"
            elif cat == "item":
                ch = "\u2665"
            elif cat == "key":
                ch = "\u2666"
            elif cat == "decoration":
                ch = "\u25cf"
            else:
                ch = "\u25cf"

            for sx in range(max(0, screen_x - sprite_w // 2), min(self.screen_w, screen_x + sprite_w // 2)):
                if dist < self.z_buffer[sx]:
                    for sy in range(max(0, top), min(self.render_h, top + sprite_h)):
                        try:
                            cp = self._get_color(stdscr, rc[0], rc[1], rc[2], False)
                            stdscr.addstr(sy, sx, ch, curses.color_pair(cp))
                        except curses.error:
                            pass

    def _render_hud(self, stdscr, player):
        h = self.screen_h
        w = self.screen_w
        hud_y = h - 4
        ammo = player.get("ammo", 0)
        health = player.get("health", 0)
        armor = player.get("armor", 0)
        weapon = player.get("weapon", "pistol").upper()
        keys = player.get("keys", set())
        face = self._get_face(player)

        hud_line1 = "\u2550" * w
        try:
            stdscr.addstr(hud_y, 0, hud_line1[:w], curses.color_pair(self._get_color(stdscr, 100, 100, 100, False)))
        except curses.error:
            pass

        key_str = ""
        if "red" in keys:
            key_str += "\u2665 "
        if "yellow" in keys:
            key_str += "\u2666 "
        if "blue" in keys:
            key_str += "\u2663 "

        sec_w = w // 5
        parts = [
            f"AMMO:{ammo:>4}",
            face,
            f"HEALTH:{health:>3}%",
            f"ARMOR:{armor:>3}%",
            f"{weapon:<12}{key_str}"
        ]
        for i, part in enumerate(parts):
            x = i * sec_w
            try:
                cp = self._get_color(stdscr, 200, 200, 50, False)
                stdscr.addstr(hud_y + 1, x, part[:sec_w].ljust(sec_w), curses.color_pair(cp))
            except curses.error:
                pass

        hp_color = (200, 0, 0) if health < 25 else (200, 200, 0) if health < 50 else (0, 200, 0)
        bar_w = w - 4
        hp_fill = int(bar_w * health / 100)
        try:
            stdscr.addstr(hud_y + 2, 0, "[", curses.color_pair(self._get_color(stdscr, 100, 100, 100, False)))
            stdscr.addstr(hud_y + 2, 1, "\u2588" * hp_fill + "\u2591" * (bar_w - hp_fill),
                         curses.color_pair(self._get_color(stdscr, *hp_color, False)))
            stdscr.addstr(hud_y + 2, w - 1, "]", curses.color_pair(self._get_color(stdscr, 100, 100, 100, False)))
        except curses.error:
            pass

        help_text = "WASD:Move Arrows:Turn Space:Use 1-7:Weapon F:Fire Q:Quit Tab:Map"
        try:
            cp = self._get_color(stdscr, 80, 80, 80, False)
            stdscr.addstr(hud_y + 3, 0, help_text[:w].ljust(w), curses.color_pair(cp))
        except curses.error:
            pass

    def _get_face(self, player):
        health = player.get("health", 100)
        pain = player.get("pain_timer", 0) > 0
        attacking = player.get("attack_timer", 0) > 0
        if health <= 0:
            return " X_x "
        if pain:
            return " >_< "
        if attacking:
            return " >_O "
        if health < 20:
            return " ;_; "
        if health < 50:
            return " :-| "
        return " :-) "

    def _render_minimap(self, stdscr, player, walls, entities):
        mw, mh = 16, 12
        ox = 1
        oy = 1
        px, py = int(player["x"]), int(player["y"])
        try:
            stdscr.addstr(oy - 1, ox, "\u250C" + "\u2500" * mw + "\u2510",
                         curses.color_pair(self._get_color(stdscr, 100, 100, 100, False)))
        except curses.error:
            pass
        for my in range(mh):
            for mx in range(mw):
                wx = px - mw // 2 + mx
                wy = py - mh // 2 + my
                ch = " "
                try:
                    if wx == px and wy == py:
                        ch = "\u25cf"
                        cp = self._get_color(stdscr, 0, 255, 0, False)
                    elif 0 <= wx < len(walls) and 0 <= wy < len(walls[0]) and walls[wx][wy]:
                        ch = "\u2588"
                        cp = self._get_color(stdscr, 100, 100, 100, False)
                    else:
                        ch = "."
                        cp = self._get_color(stdscr, 40, 40, 40, False)
                    stdscr.addstr(oy + my, ox + mx, ch, curses.color_pair(cp))
                except curses.error:
                    pass
        try:
            stdscr.addstr(oy + mh, ox, "\u2514" + "\u2500" * mw + "\u2518",
                         curses.color_pair(self._get_color(stdscr, 100, 100, 100, False)))
        except curses.error:
            pass

    def _render_messages(self, stdscr, messages):
        for i, msg in enumerate(messages[-3:]):
            try:
                cp = self._get_color(stdscr, 255, 255, 100, False)
                stdscr.addstr(i, 0, msg[:self.screen_w - 1], curses.color_pair(cp) | curses.A_BOLD)
            except curses.error:
                pass

    def _get_color(self, stdscr, r, g, b, is_bg=False):
        r = max(0, min(5, int(r * 5 / 255)))
        g = max(0, min(5, int(g * 5 / 255)))
        b = max(0, min(5, int(b * 5 / 255)))
        idx = 1 + r * 36 + g * 6 + b
        try:
            curses.init_pair(idx, idx, 0)
        except Exception:
            pass
        return idx
