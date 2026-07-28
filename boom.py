"""BOOM - Terminal DOOM Clone. Run with: python boom.py"""
import argparse
import curses
import math
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from boom_engine import Raycaster, PLAYER_SPEED, ROT_SPEED, STRIFE_SPEED
from boom_game import (
    create_player, build_wall_grid, move_player, try_use_door,
    fire_weapon, update_enemies, update_doors, check_item_pickups,
    load_map_entities, WEAPON_DEFS,
)
from boom_maps import get_builtin_map, get_builtin_map_names
from boom_wad import WADParser
import boom_audio


def parse_args():
    p = argparse.ArgumentParser(
        prog="boom",
        description="BOOM - A Terminal DOOM Clone",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="Examples:\n"
               "  python boom.py                    Play built-in E1M1\n"
               "  python boom.py --wad doom1.wad    Play from WAD file\n"
               "  python boom.py --map E1M2          Play specific map\n"
               "  python boom.py --list-maps          List maps in WAD\n"
               "  python boom.py --noaudio            Disable all audio\n"
    )
    p.add_argument("--wad", type=str, help="Path to DOOM WAD file")
    p.add_argument("--map", type=str, default="E1M1", help="Map to play (default: E1M1)")
    p.add_argument("--list-maps", action="store_true", help="List available maps in WAD")
    p.add_argument("--noaudio", action="store_true", help="Disable all audio")
    p.add_argument("--nomusic", action="store_true", help="Disable music only")
    p.add_argument("--fps", type=int, default=30, help="Target FPS (default: 30)")
    p.add_argument("--difficulty", type=int, default=2, choices=[1, 2, 3],
                   help="Difficulty: 1=easy 2=medium 3=hard")
    return p.parse_args()


def main():
    args = parse_args()

    if args.noaudio:
        boom_audio.set_audio(False)
    if args.nomusic:
        boom_audio.stop_music()

    map_data = None
    wad_parser = None

    if args.wad:
        if not os.path.isfile(args.wad):
            print(f"Error: WAD file not found: {args.wad}")
            sys.exit(1)
        try:
            wad_parser = WADParser()
            wad_parser.parse(args.wad)
            print(f"Loaded WAD: {args.wad}")
            print(f"  Signature: {wad_parser.header['signature']}")
            print(f"  Lumps: {wad_parser.header['num_lumps']}")
        except Exception as e:
            print(f"Error parsing WAD: {e}")
            sys.exit(1)

        if args.list_maps:
            maps = wad_parser.get_map_names()
            if maps:
                print(f"\nAvailable maps ({len(maps)}):")
                for m in maps:
                    print(f"  {m}")
            else:
                print("No maps found in WAD.")
            sys.exit(0)

        map_data = wad_parser.get_map_data(args.map)
        if not map_data:
            print(f"Map '{args.map}' not found in WAD.")
            maps = wad_parser.get_map_names()
            if maps:
                print(f"Available: {', '.join(maps[:10])}")
                args.map = maps[0]
                map_data = wad_parser.get_map_data(args.map)
                print(f"Loading {args.map} instead.")
            else:
                print("No maps available. Using built-in map.")
                map_data = None

    if not map_data:
        if not args.wad:
            print("BOOM - Terminal DOOM Clone")
            print("=" * 40)
            print("No WAD file specified. Using built-in maps.")
            print(f"  Built-in maps: {', '.join(get_builtin_map_names())}")
            print()
            print("To play with original DOOM levels:")
            print("  python boom.py --wad /path/to/doom1.wad")
            print()

        map_data = get_builtin_map(args.map)

    builtin_walls = map_data.get("_builtin_walls")
    if builtin_walls:
        walls = builtin_walls
        sector_colors_list = map_data.get("_builtin_sectors", [])
        sector_colors = {}
        for i, sc in enumerate(sector_colors_list):
            sector_colors[i] = sc
        map_meta = {"width": len(walls), "height": len(walls[0]) if walls else 0, "ox": 0, "oy": 0}
    else:
        walls, sector_colors, map_meta, sector_list = build_wall_grid(map_data, [])
        if not walls:
            print("Error: Could not build map geometry.")
            sys.exit(1)

    start = map_data.get("_builtin_start")
    if start:
        px, py, pa = start
    else:
        start_info, _, _, _ = load_map_entities(map_data)
        px, py, pa = start_info

    player = create_player(px, py, pa)
    player_start, enemies, items, decorations = load_map_entities(map_data)

    if not builtin_walls:
        player["x"], player["y"], player["angle"] = player_start

    all_entities = enemies + items + decorations

    map_name = map_data.get("_builtin_name", args.map)
    print(f"Playing: {map_name}")
    print(f"  Enemies: {len(enemies)}")
    print(f"  Items: {len(items)}")
    print(f"  Map size: {map_meta['width']}x{map_meta['height']}")
    print()
    print("Starting BOOM... Press any key in the terminal to begin.")
    print("Controls: WASD=Move Arrows=Turn Space=Use F=Fire 1-7=Weapon Tab=Map Q=Quit")
    time.sleep(1.5)

    def run(stdscr):
        curses.curs_set(0)
        curses.start_color()
        curses.use_default_colors()
        stdscr.nodelay(True)
        stdscr.timeout(1000 // args.fps)

        try:
            curses.mousemask(0)
        except Exception:
            pass

        try:
            curses.init_pair(1, curses.COLOR_WHITE, -1)
            curses.init_pair(2, curses.COLOR_GREEN, -1)
            curses.init_pair(3, curses.COLOR_RED, -1)
            curses.init_pair(4, curses.COLOR_YELLOW, -1)
            curses.init_pair(5, curses.COLOR_CYAN, -1)
        except Exception:
            pass

        screen_w, screen_h = stdscr.getmaxyx()
        rc = Raycaster(screen_w, screen_h)

        show_map = False
        messages = []
        msg_timer = 0.0
        forward = 0.0
        strafe = 0.0
        turn = 0.0
        attacking = False
        total_time = 0.0
        dt = 1.0 / args.fps
        frame_count = 0
        last_fps_time = time.time()
        current_fps = args.fps

        boom_audio.play_music("d_e1m1")

        while True:
            t_start = time.time()
            total_time += dt
            frame_count += 1

            if time.time() - last_fps_time >= 1.0:
                current_fps = frame_count
                frame_count = 0
                last_fps_time = time.time()

            new_w, new_h = stdscr.getmaxyx()
            if new_w != screen_w or new_h != screen_h:
                screen_w, screen_h = new_w, new_h
                rc.resize(screen_w, screen_h)

            key = stdscr.getch()
            while key != -1:
                if key == ord("q") or key == ord("Q"):
                    boom_audio.stop_music()
                    return
                elif key == ord("w") or key == ord("W"):
                    forward = 1.0
                elif key == ord("s") or key == ord("S"):
                    forward = -0.7
                elif key == ord("a") or key == ord("A"):
                    strafe = -0.8
                elif key == ord("d") or key == ord("D"):
                    strafe = 0.8
                elif key == curses.KEY_LEFT:
                    turn = -1.0
                elif key == curses.KEY_RIGHT:
                    turn = 1.0
                elif key == ord(" ") or key == 10:
                    ok, msg = try_use_door(player, map_data, {})
                    if msg:
                        messages.append(msg)
                        msg_timer = 2.0
                        if ok:
                            boom_audio.play_door()
                elif key == ord("f") or key == ord("F"):
                    attacking = True
                elif key == ord("1"):
                    _switch_weapon(player, "fist")
                elif key == ord("2"):
                    _switch_weapon(player, "pistol")
                elif key == ord("3"):
                    _switch_weapon(player, "shotgun")
                elif key == ord("4"):
                    _switch_weapon(player, "chaingun")
                elif key == ord("5"):
                    _switch_weapon(player, "rocket")
                elif key == ord("6"):
                    _switch_weapon(player, "plasma")
                elif key == ord("7"):
                    _switch_weapon(player, "bfg")
                elif key == ord("\t"):
                    show_map = not show_map
                elif key == ord("+") or key == ord("="):
                    args.fps = min(60, args.fps + 5)
                    stdscr.timeout(1000 // args.fps)
                elif key == ord("-"):
                    args.fps = max(10, args.fps - 5)
                    stdscr.timeout(1000 // args.fps)
                key = stdscr.getch()

            if forward != 0 or strafe != 0:
                move_player(player, walls, forward, strafe, dt)
                forward *= 0.85
                strafe *= 0.85
                if abs(forward) < 0.05:
                    forward = 0.0
                if abs(strafe) < 0.05:
                    strafe = 0.0

            if turn != 0:
                player["angle"] += turn * ROT_SPEED * dt * 60
                turn *= 0.8
                if abs(turn) < 0.05:
                    turn = 0.0

            if attacking:
                wdef = WEAPON_DEFS.get(player["weapon"], {})
                auto = wdef.get("auto", False)
                at = player.get("attack_timer", 0)
                if at <= 0:
                    hit, msg = fire_weapon(player, walls, enemies)
                    if msg:
                        messages.append(msg)
                        msg_timer = 2.0
                    if hit:
                        boom_audio.play_enemy_pain()
                        if not hit.get("alive", True):
                            boom_audio.play_enemy_death()
                            player["kills"] = player.get("kills", 0) + 1
                    elif msg == "":
                        boom_audio.play_no_ammo()
                    player["attack_timer"] = wdef.get("fire_rate", 0.2)
                    if not auto:
                        attacking = False
                else:
                    player["attack_timer"] = max(0, at - dt)

            player["attack_timer"] = max(0, player.get("attack_timer", 0) - dt)
            player["pain_timer"] = max(0, player.get("pain_timer", 0) - dt)

            update_enemies(enemies, player, walls, dt)

            if player["health"] <= 0:
                messages.append("YOU DIED! Press R to restart or Q to quit.")
                boom_audio.play_player_death()
                rc.render(stdscr, player, walls, all_entities, sector_colors, show_map, messages)
                while True:
                    k = stdscr.getch()
                    if k == ord("q") or k == ord("Q"):
                        return
                    elif k == ord("r") or k == ord("R"):
                        player["health"] = 100
                        player["armor"] = 0
                        player["ammo"] = {"bullets": 50, "shells": 0, "rockets": 0, "cells": 0}
                        player["weapon"] = "pistol"
                        player["keys"] = set()
                        player["kills"] = 0
                        sx, sy, sa = start if start else (player_start[0], player_start[1], player_start[2])
                        player["x"], player["y"], player["angle"] = sx, sy, sa
                        for e in enemies:
                            e["alive"] = True
                            e["health"] = e.get("health", 60)
                            e["state"] = "idle"
                        for it in items:
                            it["alive"] = True
                        messages.clear()
                        break
                    time.sleep(0.05)

            picked = check_item_pickups(player, items)
            for item in picked:
                name = item.get("name", "item")
                messages.append(f"Picked up {name}!")
                msg_timer = 2.0
                boom_audio.play_item_pickup()
                player["items"] = player.get("items", 0) + 1

            if player.get("pain_timer", 0) > 0 and player.get("pain_timer", 0) > dt:
                boom_audio.play_player_pain()

            msg_timer -= dt
            if msg_timer <= 0 and messages:
                messages = messages[1:]

            fps_str = f" FPS:{current_fps}"
            try:
                stdscr.addstr(screen_h - 1, screen_w - len(fps_str) - 1, fps_str,
                             curses.color_pair(1) | curses.A_DIM)
            except curses.error:
                pass

            rc.render(stdscr, player, walls, all_entities, sector_colors, show_map, messages)

            elapsed = time.time() - t_start
            sleep_time = max(0, (1.0 / args.fps) - elapsed)
            if sleep_time > 0:
                time.sleep(sleep_time)
            dt = time.time() - t_start

    curses.wrapper(run)
    print("\nThanks for playing BOOM!")
    print(f"Kills: {player.get('kills', 0)}/{len(enemies)}  Items: {player.get('items', 0)}/{len(items)}")


def _switch_weapon(player, weapon):
    if weapon in player.get("weapons", {}):
        player["weapon"] = weapon


if __name__ == "__main__":
    main()
