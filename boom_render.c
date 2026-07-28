/*
 * BOOM - True color ANSI raycasting renderer
 * Uses 24-bit RGB escape sequences and half-block characters (▀▄)
 * for DOOM-like visuals in the terminal
 */
#include "boom.h"
#include <ncurses.h>
#include <unistd.h>
#include <wchar.h>

/* ANSI escape helpers */
#define ESC "\033"
#define FG_RGB(r,g,b) ESC "[38;2;" #r ";" #g ";" #b "m"
#define BG_RGB(r,g,b) ESC "[48;2;" #r ";" #g ";" #b "m"
#define RESET ESC "[0m"

/* Wall type texture patterns - each type has a distinct look */
static const char *wall_patterns[WALL_NUM_TYPES] = {
    NULL,           /* NONE */
    "#%#%#%#%",     /* METAL - industrial panels */
    "::||||::",     /* BRICK - brick pattern */
    "::::::::",     /* STONE - rough stone */
    "[=][=][=]",    /* TECH - tech panels */
    "|||///|||",    /* WOOD - wood grain */
    "########",     /* RED - solid red walls */
    "########",     /* GREEN */
    "########",     /* BLUE */
    "[=[=[=[=",     /* DOOR - mechanical */
    ">>> EXIT <<<", /* EXIT */
    "\\/\\/\\/\\/",  /* HELL - organic */
};

/* Get texture character for wall based on hit position */
static char get_wall_char(int wall_type, float frac_y, int dist) {
    if (wall_type == WALL_NONE) return ' ';
    const char *pat = wall_patterns[wall_type];
    if (!pat) return '#';
    int idx = (int)(frac_y * 7.99f);
    if (idx < 0) idx = 0;
    if (idx > 7) idx = 7;
    char ch = pat[idx];
    /* Distance fog - use simpler char for far walls */
    if (dist > 400 && ch != ' ') ch = ':';
    if (dist > 600 && ch != ' ') ch = '.';
    return ch;
}

/* Half-block framebuffer */
static FramePixel *fb = NULL;
static int fb_w = 0, fb_h = 0;

/* Output buffer for batched terminal writes */
static char *out_buf = NULL;
static int out_buf_size = 0;
static int out_pos = 0;

static void out_init(int w, int h) {
    /* Max: per pixel ~40 bytes for escape codes + char */
    out_buf_size = w * h * 48 + 1024;
    out_buf = (char *)malloc(out_buf_size);
    out_pos = 0;
}

static void out_cleanup(void) {
    free(out_buf);
    out_buf = NULL;
    out_buf_size = 0;
    out_pos = 0;
}

static void out_str(const char *s) {
    int len = (int)strlen(s);
    if (out_pos + len < out_buf_size) {
        memcpy(out_buf + out_pos, s, len);
        out_pos += len;
    }
}

static void out_rgb_fg(int r, int g, int b) {
    char tmp[32];
    int n = snprintf(tmp, sizeof(tmp), "\033[38;2;%d;%d;%dm", r, g, b);
    if (out_pos + n < out_buf_size) {
        memcpy(out_buf + out_pos, tmp, n);
        out_pos += n;
    }
}

static void out_rgb_bg(int r, int g, int b) {
    char tmp[32];
    int n = snprintf(tmp, sizeof(tmp), "\033[48;2;%d;%d;%dm", r, g, b);
    if (out_pos + n < out_buf_size) {
        memcpy(out_buf + out_pos, tmp, n);
        out_pos += n;
    }
}

static void out_flush(void) {
    if (out_pos > 0) {
        write(STDOUT_FILENO, out_buf, out_pos);
        out_pos = 0;
    }
}

void render_init(GameState *gs) {
    int rw = gs->screen_w;
    int rh = gs->screen_h - 5; /* leave rows for HUD */
    if (rh < 4) rh = 4;

    fb_w = rw;
    fb_h = rh * 2; /* half-block = 2 vertical pixels per cell */
    gs->fb_w = fb_w;
    gs->fb_h = fb_h;
    fb = (FramePixel *)calloc(fb_w * fb_h, sizeof(FramePixel));

    gs->zbuffer = (float *)calloc(rw, sizeof(float));
    gs->render_h = rh;

    out_init(rw + 2, rh + 8);

    /* Hide cursor, switch to alternative buffer for clean rendering */
    write(STDOUT_FILENO, "\033[?25l" /* hide cursor */
                         "\033[?1049h" /* alt buffer */
                         "\033[2J\033[H", 16); /* clear */
}

void render_cleanup(GameState *gs) {
    /* Show cursor, restore main buffer */
    write(STDOUT_FILENO, "\033[?25h\033[?1049l", 12);
    free(fb); fb = NULL;
    free(gs->zbuffer); gs->zbuffer = NULL;
    out_cleanup();
}

/* === RAYCASTING ENGINE === */

static void cast_all_rays(GameState *gs) {
    Level *lvl = &gs->level;
    Player *p = &gs->player;
    int sw = gs->screen_w;
    int rh = gs->render_h;

    for (int col = 0; col < sw; col++) {
        float ray_a = p->angle - HALF_FOV + (float)col * FOV / (float)sw;
        float sin_a = sinf(ray_a);
        float cos_a = cosf(ray_a);
        if (cos_a == 0.0f) cos_a = 0.0001f;
        if (sin_a == 0.0f) sin_a = 0.0001f;

        /* DDA raycast */
        float dist = 0;
        int hit_wall = 0;
        int wall_type = WALL_METAL;
        float hit_frac = 0; /* fractional hit position on wall face */

        /* Step-based raycast */
        float step = 1.0f;
        for (float d = step; d < MAX_DEPTH; d += step) {
            float tx = p->x + cos_a * d;
            float ty = p->y + sin_a * d;
            int gx = (int)(tx / 64.0f);
            int gy = (int)(ty / 64.0f);

            if (gx < 0 || gy < 0 || gx >= lvl->w || gy >= lvl->h) {
                dist = MAX_DEPTH;
                break;
            }

            int w = lvl->grid[gx][gy];
            if (w != WALL_NONE) {
                dist = d;
                hit_wall = 1;
                wall_type = w;
                /* Calculate exact hit fraction for texture mapping */
                float prev_x = tx - cos_a * step;
                /* Check which face was hit */
                int gxp = (int)(prev_x / 64.0f);
                float wall_x;
                if (gxp != gx) {
                    /* Hit vertical face */
                    wall_x = fmodf(ty / 64.0f, 1.0f);
                    if (wall_x < 0) wall_x += 1.0f;
                } else {
                    /* Hit horizontal face */
                    wall_x = fmodf(tx / 64.0f, 1.0f);
                    if (wall_x < 0) wall_x += 1.0f;
                }
                hit_frac = wall_x;
                break;
            }
        }

        if (!hit_wall) dist = MAX_DEPTH;

        /* Fix fisheye */
        float fixed = dist * cosf(ray_a - p->angle);
        if (fixed < 0.5f) fixed = 0.5f;
        gs->zbuffer[col] = fixed;

        /* Wall height */
        float wall_h_f = (float)rh / (fixed / 64.0f);
        int wall_h = (int)wall_h_f;
        if (wall_h > rh) wall_h = rh;
        if (wall_h < 1) wall_h = 1;

        int top = (rh - wall_h) / 2;
        int bot = top + wall_h;

        /* Wall color with distance fog */
        int *wc = lvl->wall_colors[wall_type];
        float fog = 1.0f - fixed / 700.0f;
        if (fog < 0.05f) fog = 0.05f;

        /* Add some variation based on texture hit position */
        float tex_var = 1.0f;
        if (hit_wall && wall_h > 4) {
            /* Panel lines: darken every 8th pixel */
            int tex_y = (int)(hit_frac * 32.0f) % 8;
            if (tex_y == 0 || tex_y == 7) tex_var = 0.7f;
            /* Edge highlight */
            if (hit_frac < 0.02f || hit_frac > 0.98f) tex_var = 1.3f;
        }

        int wr = (int)(wc[0] * fog * tex_var);
        int wg = (int)(wc[1] * fog * tex_var);
        int wb = (int)(wc[2] * fog * tex_var);
        if (wr > 255) wr = 255;
        if (wg > 255) wg = 255;
        if (wb > 255) wb = 255;
        if (wr < 0) wr = 0;
        if (wg < 0) wg = 0;
        if (wb < 0) wb = 0;

        /* Darken bottom of wall for 3D effect */
        int wr2 = (int)(wr * 0.65f);
        int wg2 = (int)(wg * 0.65f);
        int wb2 = (int)(wb * 0.65f);

        /* Draw wall column using half-blocks */
        for (int py = top; py < bot; py += 2) {
            int fb_row = py * 2;
            int fb_row2 = py * 2 + 1;
            if (fb_row >= fb_h) break;

            /* Top pixel */
            FramePixel *px = &fb[fb_row * fb_w + col];
            px->r = (uint8_t)wr;
            px->g = (uint8_t)wg;
            px->b = (uint8_t)wb;
            px->ch = get_wall_char(wall_type, hit_frac, (int)fixed);

            /* Bottom pixel (one row lower) */
            if (fb_row2 < fb_h) {
                FramePixel *px2 = &fb[fb_row2 * fb_w + col];
                px2->r = (uint8_t)wr2;
                px2->g = (uint8_t)wg2;
                px2->b = (uint8_t)wb2;
                px2->ch = get_wall_char(wall_type, hit_frac, (int)fixed);
            }
        }

        /* Ceiling */
        for (int py = 0; py < top; py += 2) {
            int fb_row = py * 2;
            int fb_row2 = py * 2 + 1;
            if (fb_row >= fb_h) break;

            /* Distance-based ceiling gradient */
            float cf = 1.0f - (float)(top - py) / (float)(top + 1);
            if (cf < 0.1f) cf = 0.1f;

            /* Dark blue/grey ceiling with slight gradient */
            int cr = (int)(15 * cf), cg = (int)(15 * cf), cb = (int)(30 * cf);
            int cr2 = (int)(12 * cf), cg2 = (int)(12 * cf), cb2 = (int)(25 * cf);

            FramePixel *px = &fb[fb_row * fb_w + col];
            px->r = (uint8_t)cr; px->g = (uint8_t)cg; px->b = (uint8_t)cb;
            px->ch = ' ';

            if (fb_row2 < fb_h) {
                FramePixel *px2 = &fb[fb_row2 * fb_w + col];
                px2->r = (uint8_t)cr2; px2->g = (uint8_t)cg2; px2->b = (uint8_t)cb2;
                px2->ch = ' ';
            }
        }

        /* Floor */
        for (int py = bot; py < rh; py += 2) {
            int fb_row = py * 2;
            int fb_row2 = py * 2 + 1;
            if (fb_row >= fb_h) break;

            float ff = 1.0f - (float)(py - bot) / (float)(rh - bot + 1);
            if (ff < 0.1f) ff = 0.1f;

            /* Brown/grey floor with depth-based brightness */
            int fr = (int)(60 * ff), fg = (int)(50 * ff), fb_ = (int)(40 * ff);
            int fr2 = (int)(45 * ff), fg2 = (int)(38 * ff), fb2 = (int)(30 * ff);

            FramePixel *px = &fb[fb_row * fb_w + col];
            px->r = (uint8_t)fr; px->g = (uint8_t)fg; px->b = (uint8_t)fb_;
            px->ch = '.';

            if (fb_row2 < fb_h) {
                FramePixel *px2 = &fb[fb_row2 * fb_w + col];
                px2->r = (uint8_t)fr2; px2->g = (uint8_t)fg2; px2->b = (uint8_t)fb2;
                px2->ch = '.';
            }
        }
    }
}

/* === SPRITE RENDERING === */

typedef struct { float dist; int idx; int is_enemy; float angle; } SpriteEntry;

static void render_sprites(GameState *gs) {
    Level *lvl = &gs->level;
    Player *p = &gs->player;
    float *zb = gs->zbuffer;
    int sw = gs->screen_w;
    int rh = gs->render_h;

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

    /* Sort front-to-back for proper z-buffer testing */
    for (int i = 0; i < count - 1; i++)
        for (int j = i + 1; j < count; j++)
            if (sorted[j].dist < sorted[i].dist) {
                SpriteEntry tmp = sorted[i];
                sorted[i] = sorted[j];
                sorted[j] = tmp;
            }

    for (int s = 0; s < count; s++) {
        float dist = sorted[s].dist;
        float angle = sorted[s].angle;
        int screen_x = (int)((angle / FOV + 0.5f) * (float)sw);
        int sprite_h = (int)((float)rh / (dist / 32.0f));
        if (sprite_h < 2) sprite_h = 2;
        if (sprite_h > rh) sprite_h = rh;
        int sprite_w = sprite_h / 2;
        if (sprite_w < 1) sprite_w = 1;
        int top = (rh - sprite_h) / 2;

        int r, g, b;
        const char *sprite_art[3] = {"@", "#", "x"};
        int art_idx = 0;

        if (sorted[s].is_enemy) {
            Enemy *e = &lvl->enemies[sorted[s].idx];
            r = e->color_r; g = e->color_g; b = e->color_b;
            if (e->state >= 4) { sprite_art[0] = "x"; sprite_art[1] = "%"; sprite_art[2] = "*"; }
            else if (e->state == 3) { sprite_art[0] = "!"; sprite_art[1] = "!"; sprite_art[2] = "!"; }
            else { sprite_art[0] = "@"; sprite_art[1] = "M"; sprite_art[2] = "W"; }
        } else {
            Item *it = &lvl->items[sorted[s].idx];
            r = it->color_r; g = it->color_g; b = it->color_b;
            sprite_art[0] = "*"; sprite_art[1] = "+"; sprite_art[2] = "*";
        }

        /* Distance attenuation */
        float fog = 1.0f - dist / 500.0f;
        if (fog < 0.12f) fog = 0.12f;
        r = (int)(r * fog); g = (int)(g * fog); b = (int)(b * fog);
        if (r < 0) r = 0;
        if (g < 0) g = 0;
        if (b < 0) b = 0;
        if (r > 255) r = 255;
        if (g > 255) g = 255;
        if (b > 255) b = 255;

        int sx_start = screen_x - sprite_w / 2;
        int sx_end = screen_x + sprite_w / 2;
        for (int sx = sx_start; sx < sx_end; sx++) {
            if (sx < 0 || sx >= sw) continue;
            if (dist >= zb[sx]) continue;
            for (int sy = top; sy < top + sprite_h && sy < rh; sy++) {
                if (sy < 0) continue;
                int fb_row = sy * 2;
                int fb_row2 = sy * 2 + 1;
                float local_y = (float)(sy - top) / (float)sprite_h;
                art_idx = (int)(local_y * 2.99f);
                if (art_idx > 2) art_idx = 2;

                if (fb_row < fb_h) {
                    FramePixel *px = &fb[fb_row * fb_w + sx];
                    px->r = (uint8_t)r; px->g = (uint8_t)g; px->b = (uint8_t)b;
                    px->ch = sprite_art[art_idx][0];
                }
                if (fb_row2 < fb_h) {
                    FramePixel *px2 = &fb[fb_row2 * fb_w + sx];
                    px2->r = (uint8_t)r; px2->g = (uint8_t)g; px2->b = (uint8_t)b;
                    px2->ch = sprite_art[art_idx][0];
                }
            }
        }
    }
}

/* === WEAPON OVERLAY === */

static const char *weapon_art[WPN_NUM_WEAPONS][7] = {
    /* FIST */
    {
        "        ",
        "        ",
        "        ",
        "     _  ",
        "    | | ",
        "   /| |\\",
        "  /_| |_\\"
    },
    /* PISTOL */
    {
        "        ",
        "        ",
        "        ",
        "    ____",
        "   |    ",
        "   |    ",
        "  /|    "
    },
    /* SHOTGUN */
    {
        "   _    ",
        "  | |   ",
        "  | |   ",
        "  | |___",
        "  |     ",
        "  |     ",
        " /|     "
    },
    /* CHAINGUN */
    {
        "  _  _  ",
        " | || | ",
        " | || | ",
        " | || | ",
        " |    | ",
        " |    | ",
        "||    |"
    },
    /* ROCKET */
    {
        "   __   ",
        "  /  \\  ",
        " |    | ",
        " |====| ",
        " |    | ",
        " |    | ",
        "/|    |\\"
    },
    /* PLASMA */
    {
        "   __   ",
        "  |  |  ",
        " |    | ",
        " |    | ",
        " |    | ",
        " |    | ",
        "/|    |\\"
    },
    /* BFG */
    {
        "  ____  ",
        " /    \\ ",
        "| O  O |",
        "| ==== |",
        " \\    / ",
        "  |  |  ",
        "  |  |  "
    }
};

static void render_weapon(GameState *gs) {
    Player *p = &gs->player;
    int sw = gs->screen_w;
    int rh = gs->render_h;

    int wpn = p->weapon;
    int art_w = 8;
    int art_h = 7;
    int start_x = sw / 2 - art_w / 2;
    int start_y = rh - art_h;

    /* Weapon bob */
    float bob_offset = 0;
    if (p->bob > 0.01f) {
        bob_offset = sinf(p->bob * 8.0f) * 3.0f;
    }

    /* Firing flash */
    int flash = 0;
    if (p->attack_timer > 0) {
        int max_timer = 15;
        if (wpn == WPN_SHOTGUN) max_timer = 25;
        if (wpn == WPN_BFG) max_timer = 40;
        flash = (int)(p->attack_timer / (float)max_timer * 3.0f);
    }

    int wr = 180, wg = 180, wb = 180; /* weapon base color */
    int flash_r = 0, flash_g = 0, flash_b = 0;

    switch (wpn) {
        case WPN_FIST: wr = 200; wg = 170; wb = 140; break;
        case WPN_PISTOL: wr = 100; wg = 100; wb = 110; break;
        case WPN_SHOTGUN: wr = 120; wg = 100; wb = 70; break;
        case WPN_CHAINGUN: wr = 80; wg = 80; wb = 90; break;
        case WPN_ROCKET: wr = 100; wg = 120; wb = 80; break;
        case WPN_PLASMA: wr = 80; wg = 120; wb = 200; break;
        case WPN_BFG: wr = 80; wg = 200; wb = 80; break;
    }

    if (flash > 0) {
        flash_r = 255; flash_g = 200; flash_b = 50;
        wr = 255; wg = 255; wb = 200;
    }

    for (int y = 0; y < art_h; y++) {
        for (int x = 0; x < art_w; x++) {
            char ch = weapon_art[wpn][y][x];
            if (ch == ' ' || ch == '\0') continue;
            int dx = start_x + x;
            int dy = start_y + y + (int)bob_offset;
            if (dx < 0 || dx >= sw || dy < 0 || dy >= rh) continue;

            int fb_row = dy * 2;
            int fb_row2 = dy * 2 + 1;
            int cr = (ch == '/' || ch == '\\') ? flash_r ? flash_r : wr + 30 : wr;
            int cg = (ch == '/' || ch == '\\') ? flash_g ? flash_g : wg + 30 : wg;
            int cb = (ch == '/' || ch == '\\') ? flash_b ? flash_b : wb + 30 : wb;

            if (fb_row < fb_h) {
                FramePixel *px = &fb[fb_row * fb_w + dx];
                px->r = (uint8_t)cr; px->g = (uint8_t)cg; px->b = (uint8_t)cb;
                px->ch = ch;
            }
            if (fb_row2 < fb_h) {
                FramePixel *px2 = &fb[fb_row2 * fb_w + dx];
                px2->r = (uint8_t)(cr / 2); px2->g = (uint8_t)(cg / 2); px2->b = (uint8_t)(cb / 2);
                px2->ch = ch;
            }
        }
    }

    /* Muzzle flash */
    if (flash > 1 && wpn != WPN_FIST) {
        int cx = sw / 2;
        int cy = start_y - 2;
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -2; dx <= 2; dx++) {
                int px_x = cx + dx;
                int px_y = cy + dy;
                if (px_x < 0 || px_x >= sw || px_y < 0 || px_y >= rh) continue;
                int fb_row = px_y * 2;
                if (fb_row < fb_h) {
                    FramePixel *px = &fb[fb_row * fb_w + px_x];
                    px->r = 255; px->g = 220; px->b = 80;
                    px->ch = '*';
                }
            }
        }
    }
}

/* === HUD - DOOM STATUS BAR === */

void render_hud(GameState *gs) {
    Player *p = &gs->player;
    int sw = gs->screen_w;
    int y = gs->render_h;

    /* Status bar background - dark grey */
    char *hp = out_buf + out_pos;
    int n = snprintf(hp, out_buf_size - out_pos,
                     "\033[%d;1H\033[48;2;40;40;40m", y + 1);
    out_pos += n;
    for (int x = 0; x < sw; x++) {
        if (out_pos + 1 < out_buf_size) {
            out_buf[out_pos++] = ' ';
        }
    }

    /* Separator line */
    n = snprintf(hp, out_buf_size - out_pos,
                 "\033[%d;1H\033[38;2;100;100;100m\033[48;2;30;30;30m", y);
    out_pos += snprintf(out_buf + out_pos, out_buf_size - out_pos,
                        "\033[%d;1H\033[38;2;120;120;120m\033[48;2;30;30;30m", y);
    for (int x = 0; x < sw; x++) {
        if (out_pos + 1 < out_buf_size)
            out_buf[out_pos++] = '=';
    }

    /* AMMO section (left) */
    const char *wpn_names[] = {"FIST", "PISTOL", "SHOTGUN", "CHAIN", "ROCKET", "PLASMA", "BFG"};
    int ammo = 0;
    int wpn = p->weapon;
    if (wpn == WPN_PISTOL) ammo = p->ammo[0];
    else if (wpn == WPN_SHOTGUN) ammo = p->ammo[1];
    else if (wpn == WPN_CHAINGUN) ammo = p->ammo[0];
    else if (wpn == WPN_ROCKET) ammo = p->ammo[2];
    else if (wpn == WPN_PLASMA || wpn == WPN_BFG) ammo = p->ammo[3];

    /* Health section */
    int hp_color_r = p->health < 25 ? 220 : (p->health < 50 ? 220 : 0);
    int hp_color_g = p->health < 25 ? 0 : (p->health < 50 ? 220 : 220);
    int hp_color_b = 0;

    /* Face display */
    const char *face;
    if (p->health <= 0) face = "X_x";
    else if (p->pain_timer > 0) face = ">_<";
    else if (p->attack_timer > 0) face = ">_O";
    else if (p->health < 25) face = ";_;";
    else if (p->health < 50) face = ":-|";
    else face = ":-)";

    /* Render status bar sections */
    char bar_line[512];
    int bw = 0;

    /* AMMO */
    bw += snprintf(bar_line + bw, sizeof(bar_line) - bw,
                   "\033[%d;1H\033[38;2;255;255;0m\033[48;2;40;40;40m AMMO:", y + 1);
    bw += snprintf(bar_line + bw, sizeof(bar_line) - bw,
                   "\033[38;2;255;255;255m%-4d", ammo);

    /* HEALTH */
    bw += snprintf(bar_line + bw, sizeof(bar_line) - bw,
                   " \033[38;2;%d;%d;%dmHP:", hp_color_r, hp_color_g, hp_color_b);
    bw += snprintf(bar_line + bw, sizeof(bar_line) - bw,
                   "\033[38;2;%d;%d;%dm%d%%", hp_color_r, hp_color_g, hp_color_b, p->health);

    /* FACE */
    bw += snprintf(bar_line + bw, sizeof(bar_line) - bw,
                   " \033[38;2;255;200;100m%s", face);

    /* ARMOR */
    bw += snprintf(bar_line + bw, sizeof(bar_line) - bw,
                   " \033[38;2;100;100;255mARM:%d%%", p->armor);

    /* WEAPON */
    bw += snprintf(bar_line + bw, sizeof(bar_line) - bw,
                   " \033[38;2;200;200;200m%s", wpn_names[wpn]);

    /* KEYS */
    if (p->keys[0]) bw += snprintf(bar_line + bw, sizeof(bar_line) - bw, " \033[38;2;255;0;0mR");
    if (p->keys[1]) bw += snprintf(bar_line + bw, sizeof(bar_line) - bw, " \033[38;2;0;0;255mB");
    if (p->keys[2]) bw += snprintf(bar_line + bw, sizeof(bar_line) - bw, " \033[38;2;255;255;0mY");

    /* SCORE */
    bw += snprintf(bar_line + bw, sizeof(bar_line) - bw,
                   " \033[38;2;200;200;0mSCR:%d", gs->score);

    /* LEVEL */
    bw += snprintf(bar_line + bw, sizeof(bar_line) - bw,
                   " \033[38;2;150;150;150mLVL:%d/%d",
                   gs->current_level + 1, gs->total_levels);

    out_str(bar_line);

    /* Health bar */
    int bar_y = y + 2;
    int bar_w = sw - 2;
    int hp_fill = bar_w * p->health / 100;
    if (hp_fill > bar_w) hp_fill = bar_w;

    char *bhp = out_buf + out_pos;
    n = snprintf(bhp, out_buf_size - out_pos, "\033[%d;1H\033[48;2;40;40;40m[", bar_y);
    out_pos += n;

    for (int x = 0; x < bar_w && out_pos + 20 < out_buf_size; x++) {
        if (x < hp_fill) {
            out_pos += snprintf(out_buf + out_pos, out_buf_size - out_pos,
                               "\033[38;2;%d;%d;%dm#", hp_color_r, hp_color_g, hp_color_b);
        } else {
            out_pos += snprintf(out_buf + out_pos, out_buf_size - out_pos,
                               "\033[38;2;60;60;60m.");
        }
    }
    out_pos += snprintf(out_buf + out_pos, out_buf_size - out_pos, "]");

    /* Help line */
    int help_y = y + 3;
    out_pos += snprintf(out_buf + out_pos, out_buf_size - out_pos,
                        "\033[%d;1H\033[38;2;80;80;80m\033[48;2;30;30;30m"
                        " WASD:Move Arrows:Turn Mouse:Aim/Fire F/E:Fire/Use 1-7:Wpn Tab:Map +/-:FPS M:Mute Q:Quit",
                        help_y);
}

/* === MINIMAP === */

void render_minimap(GameState *gs) {
    Level *lvl = &gs->level;
    Player *p = &gs->player;
    int mw = 18, mh = 14;
    int ox = 1, oy = 1;
    int px = (int)(p->x / 64.0f);
    int py = (int)(p->y / 64.0f);

    char line[256];

    /* Border top */
    {int n = snprintf(line, sizeof(line), "\033[%d;%dH\033[38;2;100;100;100m+", oy, ox); (void)n;}
    out_str(line);
    for (int x = 0; x < mw; x++) { out_str("-"); }
    out_str("+");

    for (int my = 0; my < mh; my++) {
        {int n = snprintf(line, sizeof(line), "\033[%d;%dH\033[38;2;100;100;100m|", oy + 1 + my, ox); (void)n;}
        out_str(line);
        for (int mx = 0; mx < mw; mx++) {
            int wx = px - mw / 2 + mx;
            int wy = py - mh / 2 + my;
            if (wx == px && wy == py) {
                out_str("\033[38;2;0;255;0m@");
            } else if (level_in_bounds(lvl, wx, wy) && lvl->grid[wx][wy] != WALL_NONE) {
                out_str("\033[38;2;150;150;150m#");
            } else {
                out_str("\033[38;2;40;40;40m.");
            }
        }
        out_str("\033[38;2;100;100;100m|");
    }

    /* Border bottom */
    {int n = snprintf(line, sizeof(line), "\033[%d;%dH\033[38;2;100;100;100m+", oy + mh + 1, ox); (void)n;}
    out_str(line);
    for (int x = 0; x < mw; x++) { out_str("-"); }
    out_str("+");
}

/* === MESSAGES === */

void render_messages(GameState *gs) {
    char line[512];
    for (int i = 0; i < MAX_MSGS; i++) {
        if (gs->messages[i][0] && gs->msg_timers[i] > 0) {
            int n = snprintf(line, sizeof(line),
                             "\033[%d;1H\033[38;2;255;255;100m%s",
                             i + 1, gs->messages[i]);
            (void)n;
            out_str(line);
        }
    }
}

/* === MAIN RENDER FUNCTION === */

void render_frame(GameState *gs) {
    /* Clear framebuffer */
    memset(fb, 0, fb_w * fb_h * sizeof(FramePixel));

    /* Cast rays - fills framebuffer */
    cast_all_rays(gs);

    /* Render sprites into framebuffer */
    render_sprites(gs);

    /* Render weapon overlay */
    render_weapon(gs);

    /* Build output buffer using ANSI true color */
    out_pos = 0;

    /* Hide cursor and move to home */
    out_str("\033[?25l\033[H");

    /* Convert framebuffer to ANSI output - half-block rendering */
    for (int row = 0; row < fb_h; row += 2) {
        int term_row = row / 2 + 1;
        /* Move to start of this row */
        char hdr[32];
        snprintf(hdr, sizeof(hdr), "\033[%d;1H", term_row);
        out_str(hdr);

        int prev_r = -1, prev_g = -1, prev_b = -1;
        int prev_br = -1, prev_bg = -1, prev_bb = -1;

        for (int col = 0; col < fb_w; col++) {
            FramePixel *top = &fb[row * fb_w + col];
            FramePixel *bot = (row + 1 < fb_h) ? &fb[(row + 1) * fb_w + col] : top;

            int fr = top->r, fg = top->g, fb_ = top->b;
            int br = bot->r, bg = bot->g, bb = bot->b;
            char ch = top->ch;
            if (ch == '\0') ch = ' ';

            /* Use ▀ (U+2580) - foreground = top pixel, background = bottom pixel */
            /* Only emit color codes when they change */
            if (fr != prev_r || fg != prev_g || fb_ != prev_b ||
                br != prev_br || bg != prev_bg || bb != prev_bb) {
                out_rgb_fg(fr, fg, fb_);
                out_rgb_bg(br, bg, bb);
                prev_r = fr; prev_g = fg; prev_b = fb_;
                prev_br = br; prev_bg = bg; prev_bb = bb;
            }
            /* Output UTF-8 for ▀ (U+2580 = 0xE2 0x96 0x80) */
            if (out_pos + 4 < out_buf_size) {
                out_buf[out_pos++] = 0xE2;
                out_buf[out_pos++] = 0x96;
                out_buf[out_pos++] = 0x80;
            }
        }
    }

    /* Render HUD below the 3D view */
    render_hud(gs);

    /* Automap overlay */
    if (gs->show_map) render_minimap(gs);

    /* Messages */
    render_messages(gs);

    /* Reset colors at end */
    out_str("\033[0m");
    out_str("\033[%d;1H"); /* Move cursor to bottom so it's hidden */

    /* Flush entire frame in one write */
    out_flush();
}
