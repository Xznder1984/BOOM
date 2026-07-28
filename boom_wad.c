/*
 * BOOM - Level data, loading, and .boomwad file format
 * Built-in levels + custom level WAD file support
 */
#include "boom.h"

static void level_add_enemy(Level *lvl, float x, float y, int type);
static void level_add_item(Level *lvl, float x, float y, int type);

void level_init_colors(Level *lvl) {
    lvl->wall_colors[WALL_NONE][0] = 0;    lvl->wall_colors[WALL_NONE][1] = 0;    lvl->wall_colors[WALL_NONE][2] = 0;
    lvl->wall_colors[WALL_METAL][0] = 140;  lvl->wall_colors[WALL_METAL][1] = 140;  lvl->wall_colors[WALL_METAL][2] = 155;
    lvl->wall_colors[WALL_BRICK][0] = 160;  lvl->wall_colors[WALL_BRICK][1] = 80;   lvl->wall_colors[WALL_BRICK][2] = 50;
    lvl->wall_colors[WALL_STONE][0] = 130;  lvl->wall_colors[WALL_STONE][1] = 130;  lvl->wall_colors[WALL_STONE][2] = 115;
    lvl->wall_colors[WALL_TECH][0] = 50;    lvl->wall_colors[WALL_TECH][1] = 60;    lvl->wall_colors[WALL_TECH][2] = 180;
    lvl->wall_colors[WALL_WOOD][0] = 130;   lvl->wall_colors[WALL_WOOD][1] = 85;    lvl->wall_colors[WALL_WOOD][2] = 45;
    lvl->wall_colors[WALL_RED][0] = 170;    lvl->wall_colors[WALL_RED][1] = 50;     lvl->wall_colors[WALL_RED][2] = 50;
    lvl->wall_colors[WALL_GREEN][0] = 40;   lvl->wall_colors[WALL_GREEN][1] = 160;  lvl->wall_colors[WALL_GREEN][2] = 40;
    lvl->wall_colors[WALL_BLUE][0] = 40;    lvl->wall_colors[WALL_BLUE][1] = 40;    lvl->wall_colors[WALL_BLUE][2] = 180;
    lvl->wall_colors[WALL_DOOR][0] = 180;   lvl->wall_colors[WALL_DOOR][1] = 160;   lvl->wall_colors[WALL_DOOR][2] = 60;
    lvl->wall_colors[WALL_EXIT][0] = 200;   lvl->wall_colors[WALL_EXIT][1] = 200;   lvl->wall_colors[WALL_EXIT][2] = 50;
    lvl->wall_colors[WALL_HELL][0] = 160;   lvl->wall_colors[WALL_HELL][1] = 55;    lvl->wall_colors[WALL_HELL][2] = 35;
}

static void fill_rect(uint8_t grid[MAP_MAX_W][MAP_MAX_H], int w, int h,
                       int x1, int y1, int x2, int y2, uint8_t wall) {
    for (int y = y1; y <= y2 && y < h; y++)
        for (int x = x1; x <= x2 && x < w; x++)
            if (x >= 0 && y >= 0)
                grid[x][y] = wall;
}

static void clear_rect(uint8_t grid[MAP_MAX_W][MAP_MAX_H], int w, int h,
                        int x1, int y1, int x2, int y2) {
    for (int y = y1; y <= y2 && y < h; y++)
        for (int x = x1; x <= x2 && x < w; x++)
            if (x >= 0 && y >= 0 && x < w && y < h)
                grid[x][y] = WALL_NONE;
}

/* ===== LEVEL 1: Hangar ===== */
static void level_hangar(Level *lvl) {
    memset(lvl, 0, sizeof(Level));
    lvl->w = 40; lvl->h = 40;
    snprintf(lvl->name, sizeof(lvl->name), "MAP01: Hangar");

    fill_rect(lvl->grid, lvl->w, lvl->h, 1, 1, 14, 14, WALL_METAL);
    clear_rect(lvl->grid, lvl->w, lvl->h, 2, 2, 13, 13);
    fill_rect(lvl->grid, lvl->w, lvl->h, 14, 7, 22, 13, WALL_BRICK);
    clear_rect(lvl->grid, lvl->w, lvl->h, 15, 8, 21, 12);
    fill_rect(lvl->grid, lvl->w, lvl->h, 22, 1, 32, 13, WALL_STONE);
    clear_rect(lvl->grid, lvl->w, lvl->h, 23, 2, 31, 12);
    fill_rect(lvl->grid, lvl->w, lvl->h, 14, 18, 26, 28, WALL_TECH);
    clear_rect(lvl->grid, lvl->w, lvl->h, 15, 19, 25, 27);
    fill_rect(lvl->grid, lvl->w, lvl->h, 22, 28, 38, 38, WALL_HELL);
    clear_rect(lvl->grid, lvl->w, lvl->h, 23, 29, 37, 37);
    fill_rect(lvl->grid, lvl->w, lvl->h, 1, 18, 12, 28, WALL_RED);
    clear_rect(lvl->grid, lvl->w, lvl->h, 2, 19, 11, 27);
    fill_rect(lvl->grid, lvl->w, lvl->h, 28, 14, 38, 22, WALL_EXIT);
    clear_rect(lvl->grid, lvl->w, lvl->h, 29, 15, 37, 21);
    lvl->grid[33][17] = WALL_EXIT;

    lvl->grid[14][9] = WALL_DOOR;
    lvl->grid[22][7] = WALL_DOOR;
    lvl->grid[22][20] = WALL_DOOR;
    lvl->grid[14][22] = WALL_DOOR;
    lvl->grid[26][22] = WALL_DOOR;
    lvl->grid[28][18] = WALL_DOOR;

    lvl->player_start.x = 8 * 64 + 32;
    lvl->player_start.y = 8 * 64 + 32;
    lvl->player_angle = 0.0f;
    lvl->player_spawn_x = 8;
    lvl->player_spawn_y = 8;

    level_add_enemy(lvl, 18*64+32, 10*64+32, ENEMY_GRUNT);
    level_add_enemy(lvl, 27*64+32,  6*64+32, ENEMY_GRUNT);
    level_add_enemy(lvl, 27*64+32, 10*64+32, ENEMY_IMP);
    level_add_enemy(lvl, 27*64+32, 20*64+32, ENEMY_HEAVY);
    level_add_enemy(lvl, 12*64+32, 23*64+32, ENEMY_IMP);
    level_add_enemy(lvl, 12*64+32, 25*64+32, ENEMY_IMP);
    level_add_enemy(lvl, 25*64+32, 33*64+32, ENEMY_DEMON);
    level_add_enemy(lvl, 30*64+32, 33*64+32, ENEMY_DEMON);
    level_add_enemy(lvl,  6*64+32, 33*64+32, ENEMY_CACODEMON);
    level_add_enemy(lvl, 33*64+32, 18*64+32, ENEMY_BARON);

    level_add_item(lvl,  8*64+32,  4*64+32, ITEM_CLIP);
    level_add_item(lvl, 10*64+32,  4*64+32, ITEM_SHELLS);
    level_add_item(lvl, 18*64+32,  4*64+32, ITEM_SHOTGUN);
    level_add_item(lvl, 27*64+32,  3*64+32, ITEM_HEALTH);
    level_add_item(lvl, 12*64+32, 22*64+32, ITEM_MEDKIT);
    level_add_item(lvl, 12*64+32, 26*64+32, ITEM_CHAINGUN);
    level_add_item(lvl, 27*64+32, 19*64+32, ITEM_SHELLS);
    level_add_item(lvl, 25*64+32, 34*64+32, ITEM_MEDKIT);
    level_add_item(lvl,  6*64+32, 34*64+32, ITEM_COMBAT_ARMOR);
    level_add_item(lvl, 30*64+32, 30*64+32, ITEM_ROCKET_LAUNCHER);
    level_add_item(lvl, 16*64+32, 25*64+32, ITEM_KEY_RED);
    level_add_item(lvl, 33*64+32, 17*64+32, ITEM_KEY_RED);
    level_add_item(lvl,  5*64+32,  5*64+32, ITEM_CLIP);

    lvl->exit_x = 33;
    lvl->exit_y = 17;
}

/* ===== LEVEL 2: Nuclear Plant ===== */
static void level_nuclear(Level *lvl) {
    memset(lvl, 0, sizeof(Level));
    lvl->w = 42; lvl->h = 42;
    snprintf(lvl->name, sizeof(lvl->name), "MAP02: Nuclear Plant");

    fill_rect(lvl->grid, lvl->w, lvl->h, 1, 1, 20, 14, WALL_METAL);
    clear_rect(lvl->grid, lvl->w, lvl->h, 2, 2, 19, 13);
    fill_rect(lvl->grid, lvl->w, lvl->h, 20, 1, 40, 18, WALL_TECH);
    clear_rect(lvl->grid, lvl->w, lvl->h, 21, 2, 39, 17);
    fill_rect(lvl->grid, lvl->w, lvl->h, 1, 14, 18, 30, WALL_BRICK);
    clear_rect(lvl->grid, lvl->w, lvl->h, 2, 15, 17, 29);
    fill_rect(lvl->grid, lvl->w, lvl->h, 5, 15, 5, 22, WALL_BRICK);
    fill_rect(lvl->grid, lvl->w, lvl->h, 10, 20, 10, 29, WALL_BRICK);
    fill_rect(lvl->grid, lvl->w, lvl->h, 14, 15, 14, 24, WALL_BRICK);
    fill_rect(lvl->grid, lvl->w, lvl->h, 20, 18, 40, 40, WALL_HELL);
    clear_rect(lvl->grid, lvl->w, lvl->h, 21, 19, 39, 39);
    lvl->grid[25][25] = WALL_RED;
    lvl->grid[30][25] = WALL_RED;
    lvl->grid[25][32] = WALL_RED;
    lvl->grid[30][32] = WALL_RED;
    fill_rect(lvl->grid, lvl->w, lvl->h, 1, 31, 10, 40, WALL_STONE);
    clear_rect(lvl->grid, lvl->w, lvl->h, 2, 32, 9, 39);

    lvl->grid[20][8] = WALL_DOOR;
    lvl->grid[10][14] = WALL_DOOR;
    lvl->grid[20][24] = WALL_DOOR;
    lvl->grid[10][31] = WALL_DOOR;

    lvl->player_start.x = 10 * 64 + 32;
    lvl->player_start.y = 8 * 64 + 32;
    lvl->player_angle = 0.0f;
    lvl->player_spawn_x = 10;
    lvl->player_spawn_y = 8;

    level_add_enemy(lvl, 30*64+32, 10*64+32, ENEMY_GRUNT);
    level_add_enemy(lvl, 30*64+32, 14*64+32, ENEMY_GRUNT);
    level_add_enemy(lvl, 25*64+32,  5*64+32, ENEMY_IMP);
    level_add_enemy(lvl, 35*64+32,  5*64+32, ENEMY_IMP);
    level_add_enemy(lvl, 25*64+32, 14*64+32, ENEMY_HEAVY);
    level_add_enemy(lvl,  7*64+32, 25*64+32, ENEMY_DEMON);
    level_add_enemy(lvl, 15*64+32, 25*64+32, ENEMY_IMP);
    level_add_enemy(lvl, 30*64+32, 25*64+32, ENEMY_DEMON);
    level_add_enemy(lvl, 35*64+32, 25*64+32, ENEMY_DEMON);
    level_add_enemy(lvl, 30*64+32, 35*64+32, ENEMY_CACODEMON);
    level_add_enemy(lvl, 35*64+32, 35*64+32, ENEMY_CACODEMON);
    level_add_enemy(lvl,  5*64+32, 36*64+32, ENEMY_BARON);

    level_add_item(lvl, 15*64+32,  5*64+32, ITEM_SHOTGUN);
    level_add_item(lvl, 36*64+32, 10*64+32, ITEM_CLIP);
    level_add_item(lvl, 36*64+32, 14*64+32, ITEM_SHELLS);
    level_add_item(lvl,  7*64+32, 18*64+32, ITEM_HEALTH);
    level_add_item(lvl, 15*64+32, 22*64+32, ITEM_MEDKIT);
    level_add_item(lvl, 30*64+32, 30*64+32, ITEM_PLASMA);
    level_add_item(lvl, 32*64+32, 30*64+32, ITEM_CELLS);
    level_add_item(lvl, 32*64+32, 32*64+32, ITEM_CELLS);
    level_add_item(lvl,  5*64+32, 35*64+32, ITEM_BFG);
    level_add_item(lvl,  3*64+32, 35*64+32, ITEM_CELLS);
    level_add_item(lvl,  3*64+32, 37*64+32, ITEM_CELLS);
    level_add_item(lvl,  5*64+32,  5*64+32, ITEM_KEY_BLUE);
    level_add_item(lvl, 22*64+32, 30*64+32, ITEM_KEY_YELLOW);

    lvl->exit_x = 30;
    lvl->exit_y = 35;
}

/* ===== LEVEL 3: Toxic Refinery ===== */
static void level_toxic(Level *lvl) {
    memset(lvl, 0, sizeof(Level));
    lvl->w = 48; lvl->h = 48;
    snprintf(lvl->name, sizeof(lvl->name), "MAP03: Toxic Refinery");

    fill_rect(lvl->grid, lvl->w, lvl->h, 1, 1, 22, 12, WALL_METAL);
    clear_rect(lvl->grid, lvl->w, lvl->h, 2, 2, 21, 11);
    fill_rect(lvl->grid, lvl->w, lvl->h, 1, 12, 12, 28, WALL_GREEN);
    clear_rect(lvl->grid, lvl->w, lvl->h, 2, 13, 11, 27);
    fill_rect(lvl->grid, lvl->w, lvl->h, 1, 28, 12, 40, WALL_STONE);
    clear_rect(lvl->grid, lvl->w, lvl->h, 2, 29, 11, 39);
    fill_rect(lvl->grid, lvl->w, lvl->h, 22, 12, 36, 26, WALL_TECH);
    clear_rect(lvl->grid, lvl->w, lvl->h, 23, 13, 35, 25);
    fill_rect(lvl->grid, lvl->w, lvl->h, 36, 12, 46, 26, WALL_BLUE);
    clear_rect(lvl->grid, lvl->w, lvl->h, 37, 13, 45, 25);
    fill_rect(lvl->grid, lvl->w, lvl->h, 16, 20, 30, 34, WALL_RED);
    clear_rect(lvl->grid, lvl->w, lvl->h, 17, 21, 29, 33);
    lvl->grid[23][20] = WALL_RED;
    lvl->grid[24][20] = WALL_RED;
    lvl->grid[25][20] = WALL_RED;
    fill_rect(lvl->grid, lvl->w, lvl->h, 26, 34, 46, 46, WALL_HELL);
    clear_rect(lvl->grid, lvl->w, lvl->h, 27, 35, 45, 45);
    lvl->grid[32][38] = WALL_HELL;
    lvl->grid[38][38] = WALL_HELL;
    lvl->grid[32][42] = WALL_HELL;
    lvl->grid[38][42] = WALL_HELL;
    fill_rect(lvl->grid, lvl->w, lvl->h, 36, 34, 46, 40, WALL_EXIT);
    clear_rect(lvl->grid, lvl->w, lvl->h, 37, 35, 45, 39);
    lvl->grid[41][37] = WALL_EXIT;

    lvl->grid[22][6] = WALL_DOOR;
    lvl->grid[12][18] = WALL_DOOR;
    lvl->grid[22][18] = WALL_DOOR;
    lvl->grid[36][18] = WALL_DOOR;
    lvl->grid[16][28] = WALL_DOOR;
    lvl->grid[30][28] = WALL_DOOR;
    lvl->grid[26][34] = WALL_DOOR;

    lvl->player_start.x = 10 * 64 + 32;
    lvl->player_start.y = 6 * 64 + 32;
    lvl->player_angle = 0.0f;
    lvl->player_spawn_x = 10;
    lvl->player_spawn_y = 6;

    level_add_enemy(lvl, 28*64+32,  6*64+32, ENEMY_GRUNT);
    level_add_enemy(lvl, 28*64+32, 10*64+32, ENEMY_GRUNT);
    level_add_enemy(lvl, 32*64+32,  6*64+32, ENEMY_HEAVY);
    level_add_enemy(lvl, 40*64+32,  6*64+32, ENEMY_IMP);
    level_add_enemy(lvl, 40*64+32, 18*64+32, ENEMY_IMP);
    level_add_enemy(lvl, 40*64+32, 22*64+32, ENEMY_IMP);
    level_add_enemy(lvl, 32*64+32, 18*64+32, ENEMY_HEAVY);
    level_add_enemy(lvl,  6*64+32, 20*64+32, ENEMY_DEMON);
    level_add_enemy(lvl,  6*64+32, 24*64+32, ENEMY_DEMON);
    level_add_enemy(lvl,  6*64+32, 34*64+32, ENEMY_IMP);
    level_add_enemy(lvl,  6*64+32, 36*64+32, ENEMY_IMP);
    level_add_enemy(lvl, 23*64+32, 27*64+32, ENEMY_DEMON);
    level_add_enemy(lvl, 25*64+32, 27*64+32, ENEMY_DEMON);
    level_add_enemy(lvl, 35*64+32, 40*64+32, ENEMY_CACODEMON);
    level_add_enemy(lvl, 40*64+32, 40*64+32, ENEMY_CACODEMON);
    level_add_enemy(lvl, 35*64+32, 36*64+32, ENEMY_BARON);
    level_add_enemy(lvl, 42*64+32, 36*64+32, ENEMY_BARON);

    level_add_item(lvl, 15*64+32,  5*64+32, ITEM_SHOTGUN);
    level_add_item(lvl, 15*64+32,  8*64+32, ITEM_CHAINGUN);
    level_add_item(lvl, 18*64+32,  5*64+32, ITEM_CLIP);
    level_add_item(lvl, 18*64+32,  8*64+32, ITEM_SHELLS);
    level_add_item(lvl,  5*64+32, 18*64+32, ITEM_HEALTH);
    level_add_item(lvl,  8*64+32, 22*64+32, ITEM_MEDKIT);
    level_add_item(lvl,  5*64+32, 32*64+32, ITEM_ROCKET_LAUNCHER);
    level_add_item(lvl,  5*64+32, 37*64+32, ITEM_ROCKETS);
    level_add_item(lvl,  3*64+32, 37*64+32, ITEM_ROCKETS);
    level_add_item(lvl, 30*64+32, 22*64+32, ITEM_PLASMA);
    level_add_item(lvl, 32*64+32, 22*64+32, ITEM_CELLS);
    level_add_item(lvl, 32*64+32, 24*64+32, ITEM_CELLS);
    level_add_item(lvl, 40*64+32, 38*64+32, ITEM_COMBAT_ARMOR);
    level_add_item(lvl, 42*64+32, 42*64+32, ITEM_BFG);
    level_add_item(lvl, 40*64+32, 42*64+32, ITEM_CELLS);
    level_add_item(lvl, 42*64+32, 40*64+32, ITEM_CELLS);
    level_add_item(lvl, 20*64+32, 15*64+32, ITEM_KEY_RED);
    level_add_item(lvl,  8*64+32, 35*64+32, ITEM_KEY_BLUE);
    level_add_item(lvl, 42*64+32, 15*64+32, ITEM_KEY_YELLOW);

    lvl->exit_x = 41;
    lvl->exit_y = 37;
}

void level_init_default(Level *lvl, int level_index) {
    level_init_colors(lvl);
    switch (level_index) {
        case 0:  level_hangar(lvl); break;
        case 1:  level_nuclear(lvl); break;
        case 2:  level_toxic(lvl); break;
        default: level_hangar(lvl); break;
    }
}

int level_get_wall(Level *lvl, int x, int y) {
    if (x < 0 || y < 0 || x >= lvl->w || y >= lvl->h) return WALL_METAL;
    return lvl->grid[x][y];
}

void level_set_wall(Level *lvl, int x, int y, uint8_t wall) {
    if (x >= 0 && y >= 0 && x < lvl->w && y < lvl->h)
        lvl->grid[x][y] = wall;
}

int level_in_bounds(Level *lvl, int x, int y) {
    return x >= 0 && y >= 0 && x < lvl->w && y < lvl->h;
}

int level_is_wall(Level *lvl, float x, float y) {
    int gx = (int)(x / 64.0f);
    int gy = (int)(y / 64.0f);
    return level_get_wall(lvl, gx, gy) != WALL_NONE;
}

static void level_add_enemy(Level *lvl, float x, float y, int type) {
    if (lvl->num_enemies >= MAX_ENEMIES) return;
    Enemy *e = &lvl->enemies[lvl->num_enemies];
    memset(e, 0, sizeof(Enemy));
    e->x = x; e->y = y;
    e->type = type;
    e->alive = 1;
    e->state = 0;
    e->attack_cooldown = 0;
    e->pain_timer = 0;
    switch (type) {
        case ENEMY_GRUNT:
            e->health = 20; e->max_health = 20; e->damage = 3; e->speed = 1.5f;
            e->radius = 20; e->color_r = 0; e->color_g = 200; e->color_b = 0;
            snprintf(e->name, sizeof(e->name), "Grunt"); break;
        case ENEMY_HEAVY:
            e->health = 30; e->max_health = 30; e->damage = 5; e->speed = 1.5f;
            e->radius = 20; e->color_r = 100; e->color_g = 200; e->color_b = 0;
            snprintf(e->name, sizeof(e->name), "Heavy"); break;
        case ENEMY_IMP:
            e->health = 60; e->max_health = 60; e->damage = 4; e->speed = 1.2f;
            e->radius = 20; e->color_r = 180; e->color_g = 80; e->color_b = 30;
            snprintf(e->name, sizeof(e->name), "Imp"); break;
        case ENEMY_DEMON:
            e->health = 150; e->max_health = 150; e->damage = 10; e->speed = 1.8f;
            e->radius = 30; e->color_r = 200; e->color_g = 50; e->color_b = 50;
            snprintf(e->name, sizeof(e->name), "Demon"); break;
        case ENEMY_CACODEMON:
            e->health = 400; e->max_health = 400; e->damage = 8; e->speed = 1.0f;
            e->radius = 31; e->color_r = 200; e->color_g = 0; e->color_b = 0;
            snprintf(e->name, sizeof(e->name), "Cacodemon"); break;
        case ENEMY_BARON:
            e->health = 1000; e->max_health = 1000; e->damage = 15; e->speed = 1.2f;
            e->radius = 24; e->color_r = 200; e->color_g = 0; e->color_b = 80;
            snprintf(e->name, sizeof(e->name), "Baron"); break;
    }
    lvl->num_enemies++;
}

static void level_add_item(Level *lvl, float x, float y, int type) {
    if (lvl->num_items >= MAX_ITEMS) return;
    Item *it = &lvl->items[lvl->num_items];
    memset(it, 0, sizeof(Item));
    it->x = x; it->y = y;
    it->type = type;
    it->alive = 1;
    it->amount = 0;
    it->ammo_type = -1;
    it->weapon_type = -1;
    it->key_color = -1;
    it->color_r = 200; it->color_g = 200; it->color_b = 50;
    switch (type) {
        case ITEM_HEALTH: it->amount = 10; it->value = 10; snprintf(it->name, sizeof(it->name), "Health Bonus"); break;
        case ITEM_MEDKIT: it->amount = 25; it->value = 25; snprintf(it->name, sizeof(it->name), "Medikit"); break;
        case ITEM_ARMOR: it->amount = 100; it->value = 100; snprintf(it->name, sizeof(it->name), "Security Armor"); break;
        case ITEM_COMBAT_ARMOR: it->amount = 200; it->value = 200; snprintf(it->name, sizeof(it->name), "Combat Armor"); break;
        case ITEM_CLIP: it->ammo_type = 0; it->amount = 10; it->value = 10; snprintf(it->name, sizeof(it->name), "Clip"); break;
        case ITEM_SHELLS: it->ammo_type = 1; it->amount = 4; it->value = 4; snprintf(it->name, sizeof(it->name), "Shells"); break;
        case ITEM_ROCKETS: it->ammo_type = 2; it->amount = 2; it->value = 2; snprintf(it->name, sizeof(it->name), "Rockets"); break;
        case ITEM_CELLS: it->ammo_type = 3; it->amount = 20; it->value = 20; snprintf(it->name, sizeof(it->name), "Cell Pack"); break;
        case ITEM_SHOTGUN: it->weapon_type = WPN_SHOTGUN; it->color_r = 0; it->color_g = 200; it->color_b = 200; snprintf(it->name, sizeof(it->name), "Shotgun"); break;
        case ITEM_CHAINGUN: it->weapon_type = WPN_CHAINGUN; it->color_r = 0; it->color_g = 200; it->color_b = 200; snprintf(it->name, sizeof(it->name), "Chaingun"); break;
        case ITEM_ROCKET_LAUNCHER: it->weapon_type = WPN_ROCKET; it->color_r = 0; it->color_g = 200; it->color_b = 200; snprintf(it->name, sizeof(it->name), "Rocket Launcher"); break;
        case ITEM_PLASMA: it->weapon_type = WPN_PLASMA; it->color_r = 0; it->color_g = 200; it->color_b = 200; snprintf(it->name, sizeof(it->name), "Plasma Rifle"); break;
        case ITEM_BFG: it->weapon_type = WPN_BFG; it->color_r = 0; it->color_g = 255; it->color_b = 0; snprintf(it->name, sizeof(it->name), "BFG 9000"); break;
        case ITEM_KEY_RED: it->key_color = 0; it->color_r = 255; it->color_g = 0; it->color_b = 0; snprintf(it->name, sizeof(it->name), "Red Key"); break;
        case ITEM_KEY_BLUE: it->key_color = 1; it->color_r = 0; it->color_g = 0; it->color_b = 255; snprintf(it->name, sizeof(it->name), "Blue Key"); break;
        case ITEM_KEY_YELLOW: it->key_color = 2; it->color_r = 255; it->color_g = 255; it->color_b = 0; snprintf(it->name, sizeof(it->name), "Yellow Key"); break;
    }
    lvl->num_items++;
}

void get_level(int idx, Level *lvl) {
    level_init_colors(lvl);
    switch (idx) {
        case 0: level_hangar(lvl); break;
        case 1: level_nuclear(lvl); break;
        case 2: level_toxic(lvl); break;
        default: level_hangar(lvl); break;
    }
}

/* ================================================================
 * .boomwad FILE FORMAT
 *
 * Header (16 bytes):
 *   char[4]   magic     = "BOOM"
 *   uint32_t  version   = 1
 *   uint32_t  num_lumps
 *   uint32_t  dir_offset
 *
 * Directory entries (16 bytes each):
 *   char[8]   name      (null-padded)
 *   uint32_t  offset    (from start of file)
 *   uint32_t  size      (in bytes)
 *   uint8_t   type      (LUMP_MAPDATA, LUMP_ENEMIES, etc.)
 *   uint8_t   pad[3]
 *
 * Lump data: raw bytes for each lump
 * ================================================================ */

static int write_le32(FILE *f, uint32_t v) {
    uint8_t b[4] = {(uint8_t)(v & 0xFF), (uint8_t)((v >> 8) & 0xFF),
                     (uint8_t)((v >> 16) & 0xFF), (uint8_t)((v >> 24) & 0xFF)};
    return (int)fwrite(b, 1, 4, f);
}

static uint32_t read_le32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

int wad_file_exists(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
}

int wad_save_level(const char *path, Level *lvl) {
    FILE *f = fopen(path, "wb");
    if (!f) return -1;

    /* Reserve space for header */
    fseek(f, sizeof(WadHeader), SEEK_SET);

    int num_lumps = 4; /* MAP, ENMS, ITEMS, DOORS */
    uint32_t dir_offset = 0;

    /* Calculate lump sizes */
    /* LUMP 1: MAPDATA - grid w,h + grid data */
    uint32_t mapdata_size = 8 + (uint32_t)(lvl->w * lvl->h);
    uint32_t mapdata_offset = (uint32_t)ftell(f);
    write_le32(f, (uint32_t)lvl->w);
    write_le32(f, (uint32_t)lvl->h);
    fwrite(lvl->grid, 1, lvl->w * lvl->h, f);

    /* LUMP 2: ENEMIES - num_enemies + enemy data */
    uint32_t enemies_size = 4 + (uint32_t)lvl->num_enemies * (uint32_t)sizeof(Enemy);
    uint32_t enemies_offset = (uint32_t)ftell(f);
    write_le32(f, (uint32_t)lvl->num_enemies);
    fwrite(lvl->enemies, sizeof(Enemy), lvl->num_enemies, f);

    /* LUMP 3: ITEMS - num_items + item data */
    uint32_t items_size = 4 + (uint32_t)lvl->num_items * (uint32_t)sizeof(Item);
    uint32_t items_offset = (uint32_t)ftell(f);
    write_le32(f, (uint32_t)lvl->num_items);
    fwrite(lvl->items, sizeof(Item), lvl->num_items, f);

    /* LUMP 4: INFO - level metadata (name, spawn, exit, etc.) */
    typedef struct {
        char name[64];
        int32_t player_spawn_x, player_spawn_y;
        float player_angle;
        int32_t exit_x, exit_y;
        int32_t num_doors;
        float padding[8]; /* reserved */
    } InfoLump;
    InfoLump info;
    memset(&info, 0, sizeof(info));
    memset(info.name, 0, sizeof(info.name));
    memcpy(info.name, lvl->name, sizeof(lvl->name) < 63 ? sizeof(lvl->name) : 63);
    info.player_spawn_x = lvl->player_spawn_x;
    info.player_spawn_y = lvl->player_spawn_y;
    info.player_angle = lvl->player_angle;
    info.exit_x = lvl->exit_x;
    info.exit_y = lvl->exit_y;
    info.num_doors = lvl->num_doors;
    uint32_t info_size = sizeof(InfoLump);
    uint32_t info_offset = (uint32_t)ftell(f);
    fwrite(&info, 1, sizeof(info), f);

    /* Write directory */
    dir_offset = (uint32_t)ftell(f);

    /* MAP lump entry */
    char name_buf[8];
    memset(name_buf, 0, 8);
    strncpy(name_buf, "MAP", 7);
    fwrite(name_buf, 1, 8, f);
    write_le32(f, mapdata_offset);
    write_le32(f, mapdata_size);
    fputc(LUMP_MAPDATA, f);
    fwrite("\0\0\0", 1, 3, f);

    /* ENMS lump entry */
    memset(name_buf, 0, 8);
    strncpy(name_buf, "ENMS", 7);
    fwrite(name_buf, 1, 8, f);
    write_le32(f, enemies_offset);
    write_le32(f, enemies_size);
    fputc(LUMP_ENEMIES, f);
    fwrite("\0\0\0", 1, 3, f);

    /* ITEMS lump entry */
    memset(name_buf, 0, 8);
    strncpy(name_buf, "ITEMS", 7);
    fwrite(name_buf, 1, 8, f);
    write_le32(f, items_offset);
    write_le32(f, items_size);
    fputc(LUMP_ITEMS, f);
    fwrite("\0\0\0", 1, 3, f);

    /* INFO lump entry */
    memset(name_buf, 0, 8);
    strncpy(name_buf, "INFO", 7);
    fwrite(name_buf, 1, 8, f);
    write_le32(f, info_offset);
    write_le32(f, info_size);
    fputc(LUMP_INFO, f);
    fwrite("\0\0\0", 1, 3, f);

    /* Write header (go back to start) */
    fseek(f, 0, SEEK_SET);
    fwrite(BOOMWAD_MAGIC, 1, 4, f);
    write_le32(f, BOOMWAD_VERSION);
    write_le32(f, (uint32_t)num_lumps);
    write_le32(f, dir_offset);

    fclose(f);
    return 0;
}

int wad_load_level(const char *path, Level *lvl) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    /* Read header */
    uint8_t header_buf[16];
    if (fread(header_buf, 1, 16, f) != 16) { fclose(f); return -1; }
    if (memcmp(header_buf, BOOMWAD_MAGIC, 4) != 0) { fclose(f); return -1; }

    uint32_t version = read_le32(header_buf + 4);
    uint32_t num_lumps = read_le32(header_buf + 8);
    uint32_t dir_offset = read_le32(header_buf + 12);

    if (version > BOOMWAD_VERSION) { fclose(f); return -1; }

    /* Read directory */
    fseek(f, dir_offset, SEEK_SET);
    for (uint32_t i = 0; i < num_lumps; i++) {
        uint8_t entry[17];
        if (fread(entry, 1, 16, f) != 16) break;

        char lump_name[9] = {0};
        memcpy(lump_name, entry, 8);
        uint32_t lump_offset = read_le32(entry + 8);
        uint32_t lump_size = read_le32(entry + 12);
        /* lump_type is a 5th field after the 16-byte entry - read separately */
        long pos_before = ftell(f);
        uint8_t lump_type = 0;
        if (fread(&lump_type, 1, 1, f) == 1) { /* read the type byte */ }
        fseek(f, pos_before, SEEK_SET); /* seek back to before the next entry read */

        long saved_pos = ftell(f);
        fseek(f, lump_offset, SEEK_SET);

        if (lump_type == LUMP_MAPDATA && strcmp(lump_name, "MAP") == 0) {
            uint8_t map_hdr[8];
            if (fread(map_hdr, 1, 8, f) == 8) {
                lvl->w = (int)read_le32(map_hdr);
                lvl->h = (int)read_le32(map_hdr + 4);
                if (lvl->w > MAP_MAX_W) lvl->w = MAP_MAX_W;
                if (lvl->h > MAP_MAX_H) lvl->h = MAP_MAX_H;
                int to_read = lvl->w * lvl->h;
                if (to_read > (int)(lump_size - 8)) to_read = (int)(lump_size - 8);
                if (to_read > 0)
                    fread(lvl->grid, 1, to_read, f);
            }
        } else if (lump_type == LUMP_ENEMIES && strcmp(lump_name, "ENMS") == 0) {
            uint32_t count;
            if (fread(&count, 1, 4, f) == 4) {
                lvl->num_enemies = (int)count;
                if (lvl->num_enemies > MAX_ENEMIES) lvl->num_enemies = MAX_ENEMIES;
                int to_read = lvl->num_enemies * (int)sizeof(Enemy);
                if (to_read > (int)(lump_size - 4)) to_read = (int)(lump_size - 4);
                if (to_read > 0)
                    fread(lvl->enemies, 1, to_read, f);
            }
        } else if (lump_type == LUMP_ITEMS && strcmp(lump_name, "ITEMS") == 0) {
            uint32_t count;
            if (fread(&count, 1, 4, f) == 4) {
                lvl->num_items = (int)count;
                if (lvl->num_items > MAX_ITEMS) lvl->num_items = MAX_ITEMS;
                int to_read = lvl->num_items * (int)sizeof(Item);
                if (to_read > (int)(lump_size - 4)) to_read = (int)(lump_size - 4);
                if (to_read > 0)
                    fread(lvl->items, 1, to_read, f);
            }
        } else if (lump_type == LUMP_INFO && strcmp(lump_name, "INFO") == 0) {
            typedef struct {
                char name[64];
                int32_t player_spawn_x, player_spawn_y;
                float player_angle;
                int32_t exit_x, exit_y;
                int32_t num_doors;
                float padding[8];
            } InfoLump;
            InfoLump info;
            int to_read = (lump_size < sizeof(InfoLump)) ? (int)lump_size : (int)sizeof(InfoLump);
            memset(&info, 0, sizeof(info));
            fread(&info, 1, to_read, f);
            memset(lvl->name, 0, sizeof(lvl->name));
            memcpy(lvl->name, info.name, sizeof(info.name) < 63 ? sizeof(info.name) : 63);
            lvl->player_spawn_x = info.player_spawn_x;
            lvl->player_spawn_y = info.player_spawn_y;
            lvl->player_start.x = info.player_spawn_x * 64.0f + 32.0f;
            lvl->player_start.y = info.player_spawn_y * 64.0f + 32.0f;
            lvl->player_angle = info.player_angle;
            lvl->exit_x = info.exit_x;
            lvl->exit_y = info.exit_y;
        }

        fseek(f, saved_pos, SEEK_SET);
    }

    fclose(f);
    level_init_colors(lvl);
    return 0;
}
