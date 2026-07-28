/*
 * BOOM - Terminal DOOM Clone
 * A standalone first-person shooter for the terminal
 * Built-in levels, no external WAD files needed
 * GPL-2.0 License
 */
#ifndef BOOM_H
#define BOOM_H

#include <stdint.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define BOOM_VERSION "1.0.0"
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

/* Wall types */
enum {
    WALL_NONE = 0,
    WALL_METAL,
    WALL_BRICK,
    WALL_STONE,
    WALL_TECH,
    WALL_WOOD,
    WALL_RED,
    WALL_GREEN,
    WALL_BLUE,
    WALL_DOOR,
    WALL_EXIT,
    WALL_HELL,
    WALL_NUM_TYPES
};

/* Enemy types */
enum {
    ENEMY_GRUNT = 0,
    ENEMY_ZOMBIE = 0,
    ENEMY_HEAVY,
    ENEMY_IMP,
    ENEMY_DEMON,
    ENEMY_CACODEMON,
    ENEMY_BARON,
    ENEMY_NUM_TYPES
};

/* Item types */
enum {
    ITEM_HEALTH = 0,
    ITEM_MEDKIT,
    ITEM_ARMOR,
    ITEM_COMBAT_ARMOR,
    ITEM_CLIP,
    ITEM_AMMO_CLIP = ITEM_CLIP,
    ITEM_SHELLS,
    ITEM_AMMO_SHELLS = ITEM_SHELLS,
    ITEM_ROCKETS,
    ITEM_AMMO_ROCKETS = ITEM_ROCKETS,
    ITEM_CELLS,
    ITEM_AMMO_CELLS = ITEM_CELLS,
    ITEM_SHOTGUN,
    ITEM_WPN_SHOTGUN = ITEM_SHOTGUN,
    ITEM_CHAINGUN,
    ITEM_WPN_CHAINGUN = ITEM_CHAINGUN,
    ITEM_ROCKET_LAUNCHER,
    ITEM_WPN_ROCKET = ITEM_ROCKET_LAUNCHER,
    ITEM_PLASMA,
    ITEM_WPN_PLASMA = ITEM_PLASMA,
    ITEM_BFG,
    ITEM_WPN_BFG = ITEM_BFG,
    ITEM_KEY_RED,
    ITEM_KEY_BLUE,
    ITEM_KEY_YELLOW,
    ITEM_NUM_TYPES
};

/* Weapon types */
enum {
    WPN_FIST = 0,
    WPN_PISTOL,
    WPN_SHOTGUN,
    WPN_CHAINGUN,
    WPN_ROCKET,
    WPN_PLASMA,
    WPN_BFG,
    WPN_NUM_WEAPONS
};

/* Sound IDs */
enum {
    SND_PISTOL = 0,
    SND_SHOTGUN,
    SND_CHAINGUN,
    SND_ROCKET,
    SND_PLASMA,
    SND_BFG,
    SND_PUNCH,
    SND_DOOR,
    SND_ITEM,
    SND_HURT,
    SND_DEATH,
    SND_PICKUP
};

typedef struct {
    float x, y;
} Vec2;

typedef struct {
    float x, y, angle;
    int health;
    int armor;
    int weapon;
    int has_weapon[WPN_NUM_WEAPONS];
    int ammo[4]; /* bullets, shells, rockets, cells */
    int max_ammo[4];
    int keys[3]; /* red, blue, yellow */
    float attack_timer;
    float pain_timer;
    float bob;
    int kills, items, secrets;
    int dead;
    int bfg_charge;
    int bfg_charging;
    int ammo_type_for_weapon[WPN_NUM_WEAPONS];
} Player;

typedef struct {
    float x, y;
    int type;
    int health;
    int max_health;
    int damage;
    float speed;
    float radius;
    int color_r, color_g, color_b;
    int alive;
    int state; /* 0=idle, 1=chase, 2=attack, 3=pain, 4=dead */
    float attack_cooldown;
    float pain_timer;
    float state_timer;
    char name[32];
    /* additional fields for AI */
    float angle;
    int alert;
    int can_see;
    int shoot_timer;
    int timer;
    int score;
} Enemy;

typedef struct {
    float x, y;
    int type;
    int alive;
    int color_r, color_g, color_b;
    char name[32];
    /* type-specific data */
    int amount;
    int ammo_type;
    int weapon_type;
    int key_color;
    int value;
    int score;
} Item;

typedef struct {
    int x, y;
    int need_key; /* -1 = no key, 0=red, 1=blue, 2=yellow */
    int open;
    int state; /* 0=closed, 1=opening, 2=open, 3=closing */
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
    int player_spawn_x;
    int player_spawn_y;
    int num_enemies;
    Enemy enemies[MAX_ENEMIES];
    int num_items;
    Item items[MAX_ITEMS];
    int num_doors;
    Door doors[MAX_DOORS];
    int exit_x, exit_y;
    char name[64];
} Level;

typedef struct {
    float r, g, b;
} Color;

typedef struct {
    Level level;
    Player player;
    int screen_w, screen_h;
    int render_h;
    float *zbuffer;
    int show_map;
    int paused;
    char messages[MAX_MSGS][128];
    float msg_timers[MAX_MSGS];
    int target_fps;
    float dt;
    int frame_count;
    /* built-in level index */
    int current_level;
    int total_levels;
    /* extended fields */
    int score;
    int game_over;
    int game_over_timer;
    int running;
    int start_level;
    int difficulty;
    int attack_held;
    int map_timer;
    int game_time;
    int exit_reached;
    int exit_code;
} GameState;

/* boom_wad.c - Level data and loading */
void get_level(int idx, Level *lvl);
void level_init_default(Level *lvl, int level_index);
void level_init_colors(Level *lvl);
int level_get_wall(Level *lvl, int x, int y);
void level_set_wall(Level *lvl, int x, int y, uint8_t wall);
int level_in_bounds(Level *lvl, int x, int y);
int level_is_wall(Level *lvl, float x, float y);
int level_load_file(Level *lvl, const char *path);

/* boom_render.c - Raycasting renderer */
void render_init(GameState *gs);
void render_cleanup(GameState *gs);
void render_frame(GameState *gs);
void render_hud(GameState *gs);
void render_minimap(GameState *gs);
void render_messages(GameState *gs);
int make_color(int r, int g, int b);

/* boom_game.c - Game logic */
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

/* Additional game functions */
void player_move(GameState *gs, float mx, float my);
void weapon_fire(GameState *gs);
void check_doors(GameState *gs);

/* boom_audio.c - Sound effects */
void audio_init(void);
void audio_cleanup(void);
void audio_beep(int freq, int duration_ms);
void audio_play_sound(int sound_id);
void audio_set_volume(int vol);
int audio_get_volume(void);
void audio_toggle(void);
void audio_shotgun(void);
void audio_pistol(void);
void audio_chaingun(void);
void audio_rocket(void);
void audio_plasma(void);
void audio_enemy_pain(void);
void audio_enemy_death(void);
void audio_player_pain(void);
void audio_player_death(void);
void audio_door(void);
void audio_item_pickup(void);
void audio_secret(void);
void audio_no_ammo(void);

/* boom_main.c */
void boom_error(const char *msg);

#endif /* BOOM_H */
