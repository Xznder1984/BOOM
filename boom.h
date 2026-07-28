/*
 * BOOM - Terminal DOOM Clone
 * A standalone first-person shooter for the terminal
 * True color ANSI rendering with half-block characters
 * GPL-2.0 License
 */
#ifndef BOOM_H
#define BOOM_H

#include <stdint.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define BOOM_VERSION "2.0.0"
#define BOOM_TITLE "BOOM"
#define PI 3.14159265358979323846
#define FOV (PI / 3.0)
#define HALF_FOV (FOV / 2.0)
#define MAX_DEPTH 1024
#define PLAYER_RADIUS 10
#define PLAYER_SPEED 3.5
#define MOVE_SPEED 3.5
#define TURN_SPEED 0.05
#define ROT_SPEED 0.05
#define MOUSE_SENSITIVITY 0.003f
#define STRAFE_SPEED 0.8
#define ITEM_PICKUP_DIST 32.0
#define DOOR_USE_DIST 64.0
#define ENEMY_ATTACK_DIST 80.0
#define ENEMY_SIGHT_DIST 512.0
#define ENEMY_SPEED 100.0
#define MAX_MSGS 4
#define MSG_DURATION 2.5
#define MSG_LIFE 90
#define TARGET_FPS 30
#define NUM_LEVELS 3
#define MAX_DOORS 32

#define MAP_MAX_W 64
#define MAP_MAX_H 64

#define MAX_ENEMIES 128
#define MAX_ITEMS 128

#define RGB(r,g,b) (((uint32_t)(r)<<16)|((uint32_t)(g)<<8)|(uint32_t)(b))

/* Wall types */
enum {
    WALL_NONE = 0, WALL_METAL, WALL_BRICK, WALL_STONE, WALL_TECH,
    WALL_WOOD, WALL_RED, WALL_GREEN, WALL_BLUE, WALL_DOOR,
    WALL_EXIT, WALL_HELL, WALL_NUM_TYPES
};

/* Enemy types */
enum {
    ENEMY_GRUNT = 0, ENEMY_ZOMBIE = 0, ENEMY_HEAVY, ENEMY_IMP,
    ENEMY_DEMON, ENEMY_CACODEMON, ENEMY_BARON, ENEMY_NUM_TYPES
};

/* Item types */
enum {
    ITEM_HEALTH = 0, ITEM_MEDKIT, ITEM_ARMOR, ITEM_COMBAT_ARMOR,
    ITEM_CLIP, ITEM_AMMO_CLIP = ITEM_CLIP,
    ITEM_SHELLS, ITEM_AMMO_SHELLS = ITEM_SHELLS,
    ITEM_ROCKETS, ITEM_AMMO_ROCKETS = ITEM_ROCKETS,
    ITEM_CELLS, ITEM_AMMO_CELLS = ITEM_CELLS,
    ITEM_SHOTGUN, ITEM_WPN_SHOTGUN = ITEM_SHOTGUN,
    ITEM_CHAINGUN, ITEM_WPN_CHAINGUN = ITEM_CHAINGUN,
    ITEM_ROCKET_LAUNCHER, ITEM_WPN_ROCKET = ITEM_ROCKET_LAUNCHER,
    ITEM_PLASMA, ITEM_WPN_PLASMA = ITEM_PLASMA,
    ITEM_BFG, ITEM_WPN_BFG = ITEM_BFG,
    ITEM_KEY_RED, ITEM_KEY_BLUE, ITEM_KEY_YELLOW, ITEM_NUM_TYPES
};

/* Weapon types */
enum {
    WPN_FIST = 0, WPN_PISTOL, WPN_SHOTGUN, WPN_CHAINGUN,
    WPN_ROCKET, WPN_PLASMA, WPN_BFG, WPN_NUM_WEAPONS
};

/* Sound IDs */
enum {
    SND_PISTOL = 0, SND_SHOTGUN, SND_CHAINGUN, SND_ROCKET,
    SND_PLASMA, SND_BFG, SND_PUNCH, SND_DOOR,
    SND_ITEM, SND_HURT, SND_DEATH, SND_PICKUP
};

typedef struct { float x, y; } Vec2;

typedef struct {
    float x, y, angle;
    int health, armor, weapon;
    int has_weapon[WPN_NUM_WEAPONS];
    int ammo[4];
    int max_ammo[4];
    int keys[3];
    float attack_timer, pain_timer, bob;
    int kills, items, secrets, dead;
    int bfg_charge, bfg_charging;
    int ammo_type_for_weapon[WPN_NUM_WEAPONS];
} Player;

typedef struct {
    float x, y;
    int type, health, max_health, damage;
    float speed, radius;
    int color_r, color_g, color_b;
    int alive, state;
    float attack_cooldown, pain_timer, state_timer;
    char name[32];
    float angle;
    int alert, can_see, shoot_timer, timer, score;
} Enemy;

typedef struct {
    float x, y;
    int type, alive;
    int color_r, color_g, color_b;
    char name[32];
    int amount, ammo_type, weapon_type, key_color, value, score;
} Item;

typedef struct {
    int x, y, need_key, open, state;
    float anim;
    int timer;
    uint8_t (*grid_ref)[MAP_MAX_W][MAP_MAX_H];
} Door;

typedef struct {
    int w, h;
    uint8_t grid[MAP_MAX_W][MAP_MAX_H];
    int wall_colors[WALL_NUM_TYPES][3];
    Vec2 player_start;
    float player_angle;
    int player_spawn_x, player_spawn_y;
    int num_enemies;
    Enemy enemies[MAX_ENEMIES];
    int num_items;
    Item items[MAX_ITEMS];
    int num_doors;
    Door doors[MAX_DOORS];
    int exit_x, exit_y;
    char name[64];
} Level;

typedef struct { float r, g, b; } Color;

/* Frame buffer pixel for true color rendering */
typedef struct {
    uint8_t r, g, b;     /* foreground (top half-block) */
    uint8_t br, bg, bb;  /* background (bottom half-block) */
    char ch;              /* character to display */
} FramePixel;

typedef struct {
    Level level;
    Player player;
    int screen_w, screen_h;
    int render_h;
    float *zbuffer;
    int show_map, paused;
    char messages[MAX_MSGS][128];
    float msg_timers[MAX_MSGS];
    int target_fps;
    float dt;
    int frame_count;
    int current_level, total_levels;
    int score, game_over, game_over_timer;
    int running, start_level, difficulty;
    int attack_held, map_timer, game_time;
    int exit_reached, exit_code;
    /* true color frame buffer: 2 pixels per cell (half-block) */
    FramePixel *framebuf;
    int fb_w, fb_h;     /* framebuffer dimensions in pixels */
    /* mouse state */
    int mouse_enabled;
    int mouse_dx, mouse_dy;
    int mouse_fire;
    int mouse_prev_x;
    /* rendering */
    int flash_timer;
    int weapon_flash;
} GameState;

/* boom_wad.c */
void get_level(int idx, Level *lvl);
void level_init_default(Level *lvl, int level_index);
void level_init_colors(Level *lvl);
int level_get_wall(Level *lvl, int x, int y);
void level_set_wall(Level *lvl, int x, int y, uint8_t wall);
int level_in_bounds(Level *lvl, int x, int y);
int level_is_wall(Level *lvl, float x, float y);

/* boom_render.c - True color ANSI raycasting renderer */
void render_init(GameState *gs);
void render_cleanup(GameState *gs);
void render_frame(GameState *gs);
void render_hud(GameState *gs);
void render_minimap(GameState *gs);
void render_messages(GameState *gs);

/* boom_render_asm.S - Assembly optimized DDA raycaster */
void cast_rays_asm(float *zbuffer, float px, float py, float angle,
                   uint8_t grid[][MAP_MAX_H], int map_w, int map_h,
                   int screen_w, float *dists, int *wall_types);

/* boom_game.c */
void game_init(GameState *gs);
void game_update(GameState *gs, float dt);
void game_move_player(GameState *gs, float forward, float strafe);
void game_rotate_player(GameState *gs, float turn);
void game_fire_weapon(GameState *gs);
void game_use_door(GameState *gs);
void game_pickup_items(GameState *gs);
void game_update_enemies(GameState *gs);
void game_add_message(GameState *gs, const char *msg);
void game_switch_weapon(GameState *gs, int weapon);
void game_restart(GameState *gs);
void player_move(GameState *gs, float mx, float my);
void weapon_fire(GameState *gs);
void check_doors(GameState *gs);

/* BOOM WAD file format (.boomwad) for custom levels */
#define BOOMWAD_MAGIC "BOOM"
#define BOOMWAD_VERSION 1

/* WAD lump types */
#define LUMP_MAPDATA  1
#define LUMP_ENEMIES  2
#define LUMP_ITEMS    3
#define LUMP_DOORS    4
#define LUMP_INFO     5

typedef struct {
    char name[8];
    uint32_t offset;
    uint32_t size;
    uint8_t  type;
    uint8_t  pad[3];
} WadEntry;

typedef struct {
    char magic[4];
    uint32_t version;
    uint32_t num_lumps;
    uint32_t dir_offset;
} WadHeader;

/* boom_wad.c - WAD level file support */
int wad_save_level(const char *path, Level *lvl);
int wad_load_level(const char *path, Level *lvl);
int wad_file_exists(const char *path);

/* boom_audio.c */
void audio_init(void);
void audio_cleanup(void);
void audio_beep(int freq, int duration_ms);
void audio_play_sound(int sound_id);
void audio_set_volume(int vol);
int audio_get_volume(void);
void audio_toggle(void);
int audio_load_doom_wad(const char *wad_path);
int audio_load_sounds_dir(const char *dir);
void audio_play_raw(const char *name, int sample_rate, const int8_t *data, int len);

/* boom_main.c */
void boom_error(const char *msg);

#endif /* BOOM_H */
