/*
 * BOOM - Raycasting renderer for terminal
 */
#include "boom.h"
#include <ncurses.h>

static float wall_hit_dist;
static int wall_hit_type;

int make_color(int r, int g, int b) {
    r = r * 1000 / 256;
    g = g * 1000 / 256;
    b = b * 1000 / 256;
    int ri = r * 5 / 1000;
    int gi = g * 5 / 1000;
    int bi = b * 5 / 1000;
    ri = ri < 0 ? 0 : (ri > 5 ? 5 : ri);
    gi = gi < 0 ? 0 : (gi > 5 ? 5 : gi);
    bi = bi < 0 ? 0 : (bi > 5 ? 5 : bi);
    int idx = 1 + ri * 36 + gi * 6 + bi;
    init_pair(idx, idx, COLOR_BLACK);
    return idx;
}

void render_init(GameState *gs) {
    gs->zbuffer = (float *)calloc(gs->screen_w, sizeof(float));
    gs->render_h = gs->screen_h - 4;
}

void render_cleanup(GameState *gs) {
    free(gs->zbuffer);
    gs->zbuffer = NULL;
}

static void cast_rays(GameState *gs) {
    Level *lvl = &gs->level;
    Player *p = &gs->player;

    for (int col = 0; col < gs->screen_w; col++) {
        float ray_a = p->angle - HALF_FOV + col * FOV / (float)gs->screen_w;
        float sin_a = sinf(ray_a);
        float cos_a = cosf(ray_a);
        if (cos_a == 0.0f) cos_a = 0.0001f;
        if (sin_a == 0.0f) sin_a = 0.0001f;

        float dist = 0;
        int hit_wall = 0;
        wall_hit_type = WALL_METAL;

        for (int d = 1; d < MAX_DEPTH; d += 2) {
            float test_x = p->x + cos_a * d;
            float test_y = p->y + sin_a * d;
            int gx = (int)(test_x / 64.0f);
            int gy = (int)(test_y / 64.0f);

            if (gx < 0 || gy < 0 || gx >= lvl->w || gy >= lvl->h) {
                dist = MAX_DEPTH;
                break;
            }

            int w = lvl->grid[gx][gy];
            if (w != WALL_NONE) {
                dist = d;
                hit_wall = 1;
                wall_hit_type = w;
                break;
            }
        }

        if (!hit_wall) dist = MAX_DEPTH;

        float fixed = dist * cosf(ray_a - p->angle);
        if (fixed < 1.0f) fixed = 1.0f;
        gs->zbuffer[col] = fixed;

        int wall_h = (int)((float)gs->render_h / (fixed / 64.0f));
        if (wall_h > gs->render_h) wall_h = gs->render_h;
        if (wall_h < 1) wall_h = 1;

        int top = (gs->render_h - wall_h) / 2;
        int bot = top + wall_h;

        int *wc = lvl->wall_colors[wall_hit_type];
        float dm = 1.0f - fixed / 600.0f;
        if (dm < 0.1f) dm = 0.1f;
        int wr = (int)(wc[0] * dm);
        int wg = (int)(wc[1] * dm);
        int wb = (int)(wc[2] * dm);

        /* Ceiling */
        int cci = make_color(25, 25, 35);
        mvaddstr(0, col, " ");
        attron(COLOR_PAIR(cci));
        for (int row = 0; row < top && row < gs->render_h; row++) {
            float cf = 1.0f - (float)(top - row) / (float)(top + 1);
            if (cf < 0.1f) cf = 0.1f;
            int cr = (int)(25 * cf), cg = (int)(25 * cf), cb = (int)(35 * cf);
            int ci = make_color(cr, cg, cb);
            attron(COLOR_PAIR(ci));
            mvaddch(row, col, ' ');
        }
        attroff(COLOR_PAIR(cci));

        /* Wall */
        int wi = make_color(wr, wg, wb);
        attron(COLOR_PAIR(wi));
        const char *wall_chars = " .:-=+*%#@";
        int ci_idx = (int)(fixed / 50.0f);
        if (ci_idx > 9) ci_idx = 9;
        char wch = wall_chars[ci_idx];
        for (int row = top; row <= bot && row < gs->render_h; row++) {
            mvaddch(row, col, wch);
        }
        attroff(COLOR_PAIR(wi));

        /* Floor */
        int fci = make_color(35, 30, 25);
        attron(COLOR_PAIR(fci));
        for (int row = bot + 1; row < gs->render_h; row++) {
            float ff = 1.0f - (float)(row - bot) / (float)(gs->render_h - bot + 1);
            if (ff < 0.1f) ff = 0.1f;
            int fr = (int)(50 * ff), fg = (int)(40 * ff), fb = (int)(30 * ff);
            int fi = make_color(fr, fg, fb);
            attron(COLOR_PAIR(fi));
            mvaddch(row, col, '.');
        }
        attroff(COLOR_PAIR(fci));
    }
}

static void render_sprites(GameState *gs) {
    Level *lvl = &gs->level;
    Player *p = &gs->player;
    float *zb = gs->zbuffer;

    typedef struct { float dist; int idx; int is_enemy; float angle; } SpriteEntry;
    SpriteEntry sorted[MAX_ENEMIES + MAX_ITEMS];
    int count = 0;

    for (int i = 0; i < lvl->num_enemies && count < MAX_ENEMIES + MAX_ITEMS; i++) {
        Enemy *e = &lvl->enemies[i];
        if (!e->alive) continue;
        float dx = e->x - p->x, dy = e->y - p->y;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist < 1 || dist > 600) continue;
        float angle = atan2f(dy, dx) - p->angle;
        while (angle > PI) angle -= 2 * PI;
        while (angle < -PI) angle += 2 * PI;
        if (fabsf(angle) > HALF_FOV + 0.3f) continue;
        sorted[count].dist = dist;
        sorted[count].idx = i;
        sorted[count].is_enemy = 1;
        sorted[count].angle = angle;
        count++;
    }
    for (int i = 0; i < lvl->num_items && count < MAX_ENEMIES + MAX_ITEMS; i++) {
        Item *it = &lvl->items[i];
        if (!it->alive) continue;
        float dx = it->x - p->x, dy = it->y - p->y;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist < 1 || dist > 600) continue;
        float angle = atan2f(dy, dx) - p->angle;
        while (angle > PI) angle -= 2 * PI;
        while (angle < -PI) angle += 2 * PI;
        if (fabsf(angle) > HALF_FOV + 0.3f) continue;
        sorted[count].dist = dist;
        sorted[count].idx = i;
        sorted[count].is_enemy = 0;
        sorted[count].angle = angle;
        count++;
    }

    /* Sort back-to-front */
    for (int i = 0; i < count - 1; i++)
        for (int j = i + 1; j < count; j++)
            if (sorted[j].dist > sorted[i].dist) {
                SpriteEntry tmp = sorted[i];
                sorted[i] = sorted[j];
                sorted[j] = tmp;
            }

    for (int s = 0; s < count; s++) {
        float dist = sorted[s].dist;
        float angle = sorted[s].angle;
        int screen_x = (int)((angle / FOV + 0.5f) * (float)gs->screen_w);
        int sprite_h = (int)((float)gs->render_h / (dist / 32.0f));
        if (sprite_h < 2) sprite_h = 2;
        if (sprite_h > gs->render_h) sprite_h = gs->render_h;
        int sprite_w = sprite_h / 2;
        if (sprite_w < 1) sprite_w = 1;
        int top = (gs->render_h - sprite_h) / 2;

        int r, g, b;
        char ch;
        if (sorted[s].is_enemy) {
            Enemy *e = &lvl->enemies[sorted[s].idx];
            r = e->color_r; g = e->color_g; b = e->color_b;
            ch = e->state >= 4 ? 'x' : (e->state == 3 ? '!' : '@');
        } else {
            Item *it = &lvl->items[sorted[s].idx];
            r = it->color_r; g = it->color_g; b = it->color_b;
            ch = '*';
        }

        float dm = 1.0f - dist / 500.0f;
        if (dm < 0.15f) dm = 0.15f;
        r = (int)(r * dm); g = (int)(g * dm); b = (int)(b * dm);
        int ci = make_color(r, g, b);
        attron(COLOR_PAIR(ci));

        int sx_start = screen_x - sprite_w / 2;
        int sx_end = screen_x + sprite_w / 2;
        for (int sx = sx_start; sx < sx_end; sx++) {
            if (sx < 0 || sx >= gs->screen_w) continue;
            if (dist >= zb[sx]) continue;
            for (int sy = top; sy < top + sprite_h && sy < gs->render_h; sy++) {
                if (sy < 0) continue;
                mvaddch(sy, sx, ch);
            }
        }
        attroff(COLOR_PAIR(ci));
    }
}

void render_hud(GameState *gs) {
    Player *p = &gs->player;
    int y = gs->render_h;
    int w = gs->screen_w;

    /* Separator line */
    attron(COLOR_PAIR(make_color(100, 100, 100)));
    for (int x = 0; x < w; x++) mvaddch(y, x, '=');
    attroff(COLOR_PAIR(make_color(100, 100, 100)));

    /* Status line */
    char line[256];
    const char *wpn_names[] = {"FIST", "PISTOL", "SHOTGUN", "CHAINGUN", "ROCKET", "PLASMA", "BFG"};
    const char *face = " :-)";
    if (p->health <= 0) face = " X_x";
    else if (p->pain_timer > 0) face = " >_<";
    else if (p->attack_timer > 0) face = " >_O";
    else if (p->health < 25) face = " ;_;";
    else if (p->health < 50) face = " :-|";

    int ammo = 0;
    int wpn = p->weapon;
    if (wpn == WPN_PISTOL) ammo = p->ammo[0];
    else if (wpn == WPN_SHOTGUN || wpn == WPN_CHAINGUN) ammo = wpn == WPN_SHOTGUN ? p->ammo[1] : p->ammo[0];
    else if (wpn == WPN_ROCKET) ammo = p->ammo[2];
    else if (wpn == WPN_PLASMA || wpn == WPN_BFG) ammo = p->ammo[3];

    char keys_str[16] = "";
    if (p->keys[0]) strcat(keys_str, "R ");
    if (p->keys[1]) strcat(keys_str, "B ");
    if (p->keys[2]) strcat(keys_str, "Y ");

    snprintf(line, sizeof(line), " HP:%3d%% %s ARM:%3d%% | %s | AMMO:%-4d | %s",
             p->health, face, p->armor, wpn_names[wpn], ammo, keys_str);

    int hp_color = p->health < 25 ? make_color(200, 0, 0) :
                   p->health < 50 ? make_color(200, 200, 0) : make_color(0, 200, 0);
    attron(COLOR_PAIR(hp_color) | A_BOLD);
    mvprintw(y + 1, 0, "%.*s", w, line);
    attroff(COLOR_PAIR(hp_color) | A_BOLD);

    /* Health bar */
    int bar_w = w - 2;
    int hp_fill = bar_w * p->health / 100;
    if (hp_fill > bar_w) hp_fill = bar_w;
    mvaddch(y + 2, 0, '[');
    attron(COLOR_PAIR(hp_color));
    for (int x = 1; x <= bar_w; x++) {
        mvaddch(y + 2, x, x <= hp_fill ? '#' : '.');
    }
    attroff(COLOR_PAIR(hp_color));
    mvaddch(y + 2, w - 1, ']');

    /* Help line */
    attron(COLOR_PAIR(make_color(80, 80, 80)));
    mvprintw(y + 3, 0, "%.*s", w, " WASD:Move Arrows:Turn F:Fire 1-7:Weapon Space:Use Tab:Map +/-:FPS Q:Quit");
    attroff(COLOR_PAIR(make_color(80, 80, 80)));
}

void render_minimap(GameState *gs) {
    Level *lvl = &gs->level;
    Player *p = &gs->player;
    int mw = 16, mh = 12;
    int ox = 1, oy = 1;
    int px = (int)(p->x / 64.0f);
    int py = (int)(p->y / 64.0f);

    int ci_border = make_color(100, 100, 100);
    int ci_wall = make_color(120, 120, 120);
    int ci_floor = make_color(40, 40, 40);
    int ci_player = make_color(0, 255, 0);

    /* Border */
    attron(COLOR_PAIR(ci_border));
    mvaddch(oy - 1, ox - 1, '+');
    for (int x = 0; x < mw; x++) mvaddch(oy - 1, ox + x, '-');
    mvaddch(oy - 1, ox + mw, '+');
    for (int y = 0; y < mh; y++) {
        mvaddch(oy + y, ox - 1, '|');
        mvaddch(oy + y, ox + mw, '|');
    }
    mvaddch(oy + mh, ox - 1, '+');
    for (int x = 0; x < mw; x++) mvaddch(oy + mh, ox + x, '-');
    mvaddch(oy + mh, ox + mw, '+');
    attroff(COLOR_PAIR(ci_border));

    for (int my = 0; my < mh; my++) {
        for (int mx = 0; mx < mw; mx++) {
            int wx = px - mw / 2 + mx;
            int wy = py - mh / 2 + my;
            char ch = ' ';
            int ci = ci_floor;
            if (wx == px && wy == py) {
                ch = '@'; ci = ci_player;
            } else if (level_in_bounds(lvl, wx, wy) && lvl->grid[wx][wy] != WALL_NONE) {
                ch = '#'; ci = ci_wall;
            } else {
                ch = '.'; ci = ci_floor;
            }
            attron(COLOR_PAIR(ci));
            mvaddch(oy + my, ox + mx, ch);
            attroff(COLOR_PAIR(ci));
        }
    }
}

void render_messages(GameState *gs) {
    for (int i = 0; i < MAX_MSGS; i++) {
        if (gs->messages[i][0] && gs->msg_timers[i] > 0) {
            int ci = make_color(255, 255, 100);
            attron(COLOR_PAIR(ci) | A_BOLD);
            mvprintw(i, 0, "%.*s", gs->screen_w - 1, gs->messages[i]);
            attroff(COLOR_PAIR(ci) | A_BOLD);
        }
    }
}

void render_frame(GameState *gs) {
    erase();
    cast_rays(gs);
    render_sprites(gs);
    render_hud(gs);
    if (gs->show_map) render_minimap(gs);
    render_messages(gs);
    refresh();
}
