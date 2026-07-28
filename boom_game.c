/*
 * BOOM - Game logic (movement, collision, enemies, items, doors)
 */
#include "boom.h"
#include <math.h>

static float dist_to(float x1, float y1, float x2, float y2) {
    float dx = x2 - x1, dy = y2 - y1;
    return sqrtf(dx * dx + dy * dy);
}

static void add_msg(GameState *gs, const char *msg) {
    for (int i = 0; i < MAX_MSGS; i++) {
        if (gs->msg_timers[i] <= 0) {
            strncpy(gs->messages[i], msg, sizeof(gs->messages[0]) - 1);
            gs->messages[i][sizeof(gs->messages[0]) - 1] = '\0';
            gs->msg_timers[i] = MSG_LIFE;
            return;
        }
    }
    for (int i = MAX_MSGS - 1; i > 0; i--) {
        strcpy(gs->messages[i], gs->messages[i - 1]);
        gs->msg_timers[i] = gs->msg_timers[i - 1];
    }
    strncpy(gs->messages[0], msg, sizeof(gs->messages[0]) - 1);
    gs->messages[0][sizeof(gs->messages[0]) - 1] = '\0';
    gs->msg_timers[0] = MSG_LIFE;
}

void game_init(GameState *gs) {
    Level *lvl = &gs->level;
    Player *p = &gs->player;

    p->x = lvl->player_spawn_x * 64.0f + 32.0f;
    p->y = lvl->player_spawn_y * 64.0f + 32.0f;
    p->angle = 0;
    p->health = 100;
    p->armor = 0;
    p->weapon = WPN_PISTOL;
    p->ammo[0] = 50;
    p->ammo[1] = 0;
    p->ammo[2] = 0;
    p->ammo[3] = 0;
    memset(p->keys, 0, sizeof(p->keys));
    memset(p->has_weapon, 0, sizeof(p->has_weapon));
    p->has_weapon[WPN_FIST] = 1;
    p->has_weapon[WPN_PISTOL] = 1;
    p->attack_timer = 0;
    p->pain_timer = 0;
    p->bfg_charge = 0;
    p->bfg_charging = 0;
    p->kills = 0;
    p->secrets = 0;
    p->items = 0;

    gs->score = 0;
    gs->game_over = 0;
    gs->map_timer = 0;
    gs->game_time = 0;
    gs->show_map = 0;
    gs->paused = 0;
    gs->exit_reached = 0;
    gs->exit_code = 0;
    gs->target_fps = TARGET_FPS;

    memset(gs->zbuffer, 0, gs->screen_w * sizeof(float));
    memset(gs->messages, 0, sizeof(gs->messages));
    memset(gs->msg_timers, 0, sizeof(gs->msg_timers));

    add_msg(gs, "Welcome to BOOM! Kill all enemies and find the exit.");
    add_msg(gs, "WASD=Move Arrows=Turn F=Fire Space=Use Q=Quit");
}

static int wall_at(Level *lvl, float x, float y) {
    int gx = (int)(x / 64.0f);
    int gy = (int)(y / 64.0f);
    if (!level_in_bounds(lvl, gx, gy)) return 1;
    return lvl->grid[gx][gy] != WALL_NONE;
}

static int can_move(GameState *gs, float nx, float ny) {
    Level *lvl = &gs->level;
    float r = PLAYER_RADIUS;
    if (wall_at(lvl, nx - r, ny - r)) return 0;
    if (wall_at(lvl, nx + r, ny - r)) return 0;
    if (wall_at(lvl, nx - r, ny + r)) return 0;
    if (wall_at(lvl, nx + r, ny + r)) return 0;
    return 1;
}

void player_move(GameState *gs, float mx, float my) {
    Player *p = &gs->player;
    float nx = p->x + mx;
    float ny = p->y + my;
    if (can_move(gs, nx, ny)) { p->x = nx; p->y = ny; }
    else if (can_move(gs, nx, p->y)) p->x = nx;
    else if (can_move(gs, p->x, ny)) p->y = ny;
}

void weapon_fire(GameState *gs) {
    Player *p = &gs->player;
    Level *lvl = &gs->level;
    if (p->attack_timer > 0 || p->health <= 0) return;

    int wpn = p->weapon;
    float range = 600.0f;
    int damage = 0;
    int ammo_cost = 0;
    int hitscan = 1;
    float proj_speed = 0;
    float proj_x = 0, proj_y = 0, proj_dx = 0, proj_dy = 0;
    int is_bfg = 0;

    switch (wpn) {
        case WPN_FIST:
            range = 64.0f; damage = 20; break;
        case WPN_PISTOL:
            ammo_cost = 1; damage = 15; break;
        case WPN_SHOTGUN:
            ammo_cost = 1; damage = 7; break;
        case WPN_CHAINGUN:
            ammo_cost = 1; damage = 10; break;
        case WPN_ROCKET:
            ammo_cost = 1; hitscan = 0; proj_speed = 400.0f; damage = 100; break;
        case WPN_PLASMA:
            ammo_cost = 1; hitscan = 0; proj_speed = 500.0f; damage = 25; break;
        case WPN_BFG:
            ammo_cost = 40; hitscan = 0; proj_speed = 200.0f;
            damage = 10 + p->bfg_charge * 3;
            if (damage > 400) damage = 400;
            is_bfg = 1;
            break;
    }

    if (ammo_cost > 0) {
        int slot = p->ammo_type_for_weapon[wpn];
        if (p->ammo[slot] < ammo_cost) return;
        p->ammo[slot] -= ammo_cost;
    }

    if (wpn == WPN_SHOTGUN) {
        for (int s = -1; s <= 1; s++) {
            float sa = p->angle + s * 0.05f;
            float dx = cosf(sa), dy = sinf(sa);
            float cx = p->x, cy = p->y;
            for (float d = 0; d < range; d += 4) {
                cx += dx * 4; cy += dy * 4;
                int gx = (int)(cx / 64.0f), gy = (int)(cy / 64.0f);
                if (!level_in_bounds(&gs->level, gx, gy) || gs->level.grid[gx][gy] != WALL_NONE) break;
                for (int i = 0; i < lvl->num_enemies; i++) {
                    Enemy *e = &lvl->enemies[i];
                    if (!e->alive) continue;
                    if (dist_to(cx, cy, e->x, e->y) < 24) {
                        e->health -= damage;
                        e->alert = 1;
                        e->state = 1;
                        e->timer = 12;
                        if (e->health <= 0) { e->alive = 0; gs->score += e->score; add_msg(gs, "Enemy killed!"); }
                        goto shotgun_done;
                    }
                }
            }
        }
    } else if (hitscan) {
        float dx = cosf(p->angle), dy = sinf(p->angle);
        float cx = p->x, cy = p->y;
        for (float d = 0; d < range; d += 2) {
            cx += dx * 2; cy += dy * 2;
            int gx = (int)(cx / 64.0f), gy = (int)(cy / 64.0f);
            if (!level_in_bounds(&gs->level, gx, gy) || gs->level.grid[gx][gy] != WALL_NONE) break;
            for (int i = 0; i < lvl->num_enemies; i++) {
                Enemy *e = &lvl->enemies[i];
                if (!e->alive) continue;
                if (dist_to(cx, cy, e->x, e->y) < 24) {
                    e->health -= damage;
                    e->alert = 1;
                    e->state = 1;
                    e->timer = 12;
                    if (e->health <= 0) { e->alive = 0; gs->score += e->score; add_msg(gs, "Enemy killed!"); }
                    goto done;
                }
            }
        }
    } else {
        proj_x = p->x; proj_y = p->y;
        proj_dx = cosf(p->angle) * proj_speed;
        proj_dy = sinf(p->angle) * proj_speed;
        /* Projectile physics handled in enemy_ai / update loop - simple approach: */
        for (float t = 0; t < range / proj_speed; t += 0.016f) {
            proj_x += proj_dx * 0.016f * 60;
            proj_y += proj_dy * 0.016f * 60;
            int gx = (int)(proj_x / 64.0f), gy = (int)(proj_y / 64.0f);
            if (!level_in_bounds(&gs->level, gx, gy) || gs->level.grid[gx][gy] != WALL_NONE) break;
            for (int i = 0; i < lvl->num_enemies; i++) {
                Enemy *e = &lvl->enemies[i];
                if (!e->alive) continue;
                if (dist_to(proj_x, proj_y, e->x, e->y) < 24) {
                    e->health -= damage;
                    e->alert = 1;
                    e->state = 1;
                    e->timer = 12;
                    if (e->health <= 0) { e->alive = 0; gs->score += e->score; add_msg(gs, "Enemy killed!"); }
                    goto done;
                }
            }
        }
    }
done:
shotgun_done:
    switch (wpn) {
        case WPN_FIST: p->attack_timer = 12; break;
        case WPN_PISTOL: p->attack_timer = 15; break;
        case WPN_SHOTGUN: p->attack_timer = 25; break;
        case WPN_CHAINGUN: p->attack_timer = 5; break;
        case WPN_ROCKET: p->attack_timer = 20; break;
        case WPN_PLASMA: p->attack_timer = 8; break;
        case WPN_BFG: p->attack_timer = 40; break;
    }
}

static void enemy_attack_player(GameState *gs, Enemy *e) {
    Player *p = &gs->player;
    if (p->health <= 0) return;
    int dmg = e->damage;
    int absorbed = p->armor / 2;
    if (absorbed > dmg) absorbed = dmg;
    p->armor -= absorbed;
    p->health -= (dmg - absorbed);
    p->pain_timer = 10;
    if (p->health <= 0) { p->health = 0; gs->game_over = 1; }
}

static void enemy_ai(GameState *gs, Enemy *e, float dt) {
    Player *p = &gs->player;
    if (!e->alive) return;

    float dist = dist_to(e->x, e->y, p->x, p->y);
    float angle_to = atan2f(p->y - e->y, p->x - e->x);
    float diff = angle_to - e->angle;
    while (diff > PI) diff -= 2 * PI;
    while (diff < -PI) diff += 2 * PI;

    e->can_see = 0;
    {
        float dx = p->x - e->x, dy = p->y - e->y;
        float d = sqrtf(dx * dx + dy * dy);
        if (d > 0) { dx /= d; dy /= d; }
        float cx = e->x, cy = e->y;
        e->can_see = 1;
        for (float t = 0; t < d; t += 8) {
            cx += dx * 8; cy += dy * 8;
            int gx = (int)(cx / 64.0f), gy = (int)(cy / 64.0f);
            if (!level_in_bounds(&gs->level, gx, gy) || gs->level.grid[gx][gy] != WALL_NONE) {
                e->can_see = 0; break;
            }
        }
    }

    if (e->can_see || dist < 128) e->alert = 1;

    switch (e->type) {
        case ENEMY_ZOMBIE:
            if (e->alert) {
                e->angle += diff * 0.05f;
                if (dist > 160) {
                    float mx = cosf(e->angle) * ENEMY_SPEED * 0.6f * dt;
                    float my = sinf(e->angle) * ENEMY_SPEED * 0.6f * dt;
                    if (can_move(gs, e->x + mx, e->y + my)) { e->x += mx; e->y += my; }
                }
                if (e->can_see && dist < 400 && e->shoot_timer <= 0) {
                    enemy_attack_player(gs, e);
                    e->shoot_timer = 45;
                }
            }
            break;
        case ENEMY_IMP:
            if (e->alert) {
                e->angle += diff * 0.05f;
                if (dist > 128) {
                    float mx = cosf(e->angle) * ENEMY_SPEED * 0.8f * dt;
                    float my = sinf(e->angle) * ENEMY_SPEED * 0.8f * dt;
                    if (can_move(gs, e->x + mx, e->y + my)) { e->x += mx; e->y += my; }
                }
                if (e->can_see && dist < 320 && e->shoot_timer <= 0) {
                    enemy_attack_player(gs, e);
                    e->shoot_timer = 35;
                }
            }
            break;
        case ENEMY_DEMON:
            if (e->alert) {
                e->angle += diff * 0.05f;
                float mx = cosf(e->angle) * ENEMY_SPEED * 1.2f * dt;
                float my = sinf(e->angle) * ENEMY_SPEED * 1.2f * dt;
                if (can_move(gs, e->x + mx, e->y + my)) { e->x += mx; e->y += my; }
                if (dist < 80 && e->shoot_timer <= 0) {
                    enemy_attack_player(gs, e);
                    e->shoot_timer = 20;
                }
            }
            break;
        case ENEMY_CACODEMON:
            if (e->alert) {
                e->angle += diff * 0.03f;
                if (dist > 160) {
                    float mx = cosf(e->angle) * ENEMY_SPEED * 0.5f * dt;
                    float my = sinf(e->angle) * ENEMY_SPEED * 0.5f * dt;
                    if (can_move(gs, e->x + mx, e->y + my)) { e->x += mx; e->y += my; }
                }
                if (e->can_see && dist < 500 && e->shoot_timer <= 0) {
                    enemy_attack_player(gs, e);
                    e->shoot_timer = 50;
                }
            }
            break;
        case ENEMY_BARON:
            if (e->alert) {
                e->angle += diff * 0.04f;
                if (dist > 160) {
                    float mx = cosf(e->angle) * ENEMY_SPEED * 0.5f * dt;
                    float my = sinf(e->angle) * ENEMY_SPEED * 0.5f * dt;
                    if (can_move(gs, e->x + mx, e->y + my)) { e->x += mx; e->y += my; }
                }
                if (e->can_see && dist < 600 && e->shoot_timer <= 0) {
                    enemy_attack_player(gs, e);
                    e->shoot_timer = 60;
                }
            }
            break;
    }

    if (e->shoot_timer > 0) e->shoot_timer--;
    if (e->state > 0 && e->timer > 0) {
        e->timer--;
        if (e->timer <= 0) e->state = 0;
    }
}

static void item_collect(GameState *gs, Item *it) {
    Player *p = &gs->player;
    if (!it->alive) return;
    float dist = dist_to(p->x, p->y, it->x, it->y);
    if (dist > 32) return;

    switch (it->type) {
        case ITEM_HEALTH:
            if (p->health >= 100) return;
            p->health += it->value;
            if (p->health > 100) p->health = 100;
            add_msg(gs, "Health bonus!"); break;
        case ITEM_ARMOR:
            if (p->armor >= 200) return;
            p->armor += it->value;
            if (p->armor > 200) p->armor = 200;
            add_msg(gs, "Armor bonus!"); break;
        case ITEM_AMMO_CLIP:
            if (p->ammo[0] >= 200) return;
            p->ammo[0] += it->value; add_msg(gs, "Picked up clip!"); break;
        case ITEM_AMMO_SHELLS:
            if (p->ammo[1] >= 50) return;
            p->ammo[1] += it->value; add_msg(gs, "Picked up shells!"); break;
        case ITEM_AMMO_CELLS:
            if (p->ammo[3] >= 300) return;
            p->ammo[3] += it->value; add_msg(gs, "Picked up cells!"); break;
        case ITEM_AMMO_ROCKETS:
            if (p->ammo[2] >= 50) return;
            p->ammo[2] += it->value; add_msg(gs, "Picked up rockets!"); break;
        case ITEM_KEY_RED:
            p->keys[0] = 1; add_msg(gs, "Picked up red key!"); break;
        case ITEM_KEY_BLUE:
            p->keys[1] = 1; add_msg(gs, "Picked up blue key!"); break;
        case ITEM_KEY_YELLOW:
            p->keys[2] = 1; add_msg(gs, "Picked up yellow key!"); break;
        case ITEM_WPN_SHOTGUN:
            p->has_weapon[WPN_SHOTGUN] = 1; p->ammo[1] += 8;
            p->weapon = WPN_SHOTGUN;
            add_msg(gs, "Picked up shotgun!"); break;
        case ITEM_WPN_CHAINGUN:
            p->has_weapon[WPN_CHAINGUN] = 1; p->ammo[0] += 20;
            p->weapon = WPN_CHAINGUN;
            add_msg(gs, "Picked up chaingun!"); break;
        case ITEM_WPN_ROCKET:
            p->has_weapon[WPN_ROCKET] = 1; p->ammo[2] += 2;
            p->weapon = WPN_ROCKET;
            add_msg(gs, "Picked up rocket launcher!"); break;
        case ITEM_WPN_PLASMA:
            p->has_weapon[WPN_PLASMA] = 1; p->ammo[3] += 40;
            p->weapon = WPN_PLASMA;
            add_msg(gs, "Picked up plasma rifle!"); break;
        case ITEM_WPN_BFG:
            p->has_weapon[WPN_BFG] = 1; p->ammo[3] += 40;
            p->weapon = WPN_BFG;
            add_msg(gs, "Picked up BFG9000!"); break;
    }
    it->alive = 0;
    gs->score += it->score;
}

void check_doors(GameState *gs) {
    Player *p = &gs->player;
    Level *lvl = &gs->level;

    for (int i = 0; i < lvl->num_doors; i++) {
        Door *d = &lvl->doors[i];
        float dist = dist_to(p->x, p->y, d->x * 64 + 32, d->y * 64 + 32);
        if (dist > 128) continue;
        if (!d->open && d->state == 0) {
            if (d->need_key >= 0 && !p->keys[d->need_key]) {
                add_msg(gs, "You need a key!");
                continue;
            }
            d->state = 1;
            d->anim = 0;
        }
    }
}

static void update_doors(GameState *gs) {
    Level *lvl = &gs->level;
    for (int i = 0; i < lvl->num_doors; i++) {
        Door *d = &lvl->doors[i];
        if (d->state == 1) {
            d->anim += 0.03f;
            if (d->anim >= 1.0f) {
                d->anim = 1.0f;
                d->open = 1;
                d->state = 2;
                d->timer = 300;
            }
            (*d->grid_ref)[(int)(d->y)][(int)(d->x)] = (uint8_t)(WALL_NONE + d->anim * 100);
        } else if (d->state == 2) {
            d->timer--;
            if (d->timer <= 0) d->state = 3;
        } else if (d->state == 3) {
            d->anim -= 0.03f;
            if (d->anim <= 0.0f) {
                d->anim = 0.0f;
                d->open = 0;
                d->state = 0;
                (*d->grid_ref)[(int)(d->y)][(int)(d->x)] = WALL_DOOR;
            }
            (*d->grid_ref)[(int)(d->y)][(int)(d->x)] = WALL_DOOR;
        }
    }
}

static void check_exit(GameState *gs) {
    Player *p = &gs->player;
    Level *lvl = &gs->level;
    float dist = dist_to(p->x, p->y, lvl->exit_x * 64 + 32, lvl->exit_y * 64 + 32);
    if (dist < 48) {
        gs->exit_reached = 1;
        gs->exit_code = 1;
    }
}

void game_update(GameState *gs, float dt) {
    if (gs->game_over || gs->paused || gs->exit_reached) return;

    Player *p = &gs->player;

    if (gs->attack_held && p->attack_timer <= 0) {
        weapon_fire(gs);
    }

    if (p->attack_timer > 0) p->attack_timer--;
    if (p->pain_timer > 0) p->pain_timer--;
    if (gs->map_timer > 0) gs->map_timer--;

    gs->game_time++;

    for (int i = 0; i < MAX_MSGS; i++)
        if (gs->msg_timers[i] > 0) gs->msg_timers[i]--;

    Level *lvl = &gs->level;
    for (int i = 0; i < lvl->num_enemies; i++)
        enemy_ai(gs, &lvl->enemies[i], dt);

    for (int i = 0; i < lvl->num_items; i++)
        item_collect(gs, &lvl->items[i]);

    update_doors(gs);
    check_doors(gs);
    check_exit(gs);
}
