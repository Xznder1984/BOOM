/*
 * BOOM - Main entry point, input handling, game loop
 */
#include "boom.h"
#include <ncurses.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

static GameState gs;
static int last_time = 0;
static int anim_frame = 0;

static int screen_w, screen_h;
static Level levels[NUM_LEVELS];
static int current_level = 0;
static int level_loaded = 0;

void boom_error(const char *msg) {
    endwin();
    printf("BOOM ERROR: %s\n", msg);
    exit(1);
}

static void load_level(int idx) {
    if (idx < 0 || idx >= NUM_LEVELS) return;
    get_level(idx, &levels[idx]);
    current_level = idx;
    level_loaded = 1;
    gs.level = levels[idx];
    game_init(&gs);
}

static void init_ncurses(void) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    curs_set(0);
    start_color();

    for (int r = 0; r <= 5; r++)
        for (int g = 0; g <= 5; g++)
            for (int b = 0; b <= 5; b++) {
                int idx = 1 + r * 36 + g * 6 + b;
                init_color(idx, r * 200, g * 200, b * 200);
                init_pair(idx, idx, COLOR_BLACK);
            }

    getmaxyx(stdscr, screen_h, screen_w);

    memset(&gs, 0, sizeof(gs));
    gs.screen_w = screen_w;
    gs.screen_h = screen_h;
    gs.zbuffer = (float *)calloc(screen_w, sizeof(float));
    render_init(&gs);
}

static void cleanup(void) {
    render_cleanup(&gs);
    free(gs.zbuffer);
    endwin();
}

static int check_deps(void) {
    return 1;
}

static void print_banner(void) {
    printf("=================================\n");
    printf("  ____   ___  _  _  _____ \n");
    printf(" |  _ \\ / _ \\| \\| || ____|\n");
    printf(" | |_) | | | |  \\| ||  _|  \n");
    printf(" |  __/| |_| | |\\  || |___ \n");
    printf(" |_|    \\___/|_| \\_||_____|\n");
    printf("\n");
    printf(" Terminal FPS | Raycasting 3D Engine\n");
    printf(" DOOM Clone   | Version 1.0.0\n");
    printf("=================================\n");
    printf("\n");
}

static void show_help(void) {
    print_banner();
    printf("Usage: boom [OPTIONS]\n\n");
    printf("Options:\n");
    printf("  -l, --level <N>   Start at level N (0-%d)\n", NUM_LEVELS - 1);
    printf("  -d, --difficulty <N>  Difficulty: 0=easy, 1=normal, 2=hard\n");
    printf("  -f, --fps <N>     Target FPS (default: 30)\n");
    printf("  -w, --width <N>   Screen width (default: terminal)\n");
    printf("  -t, --height <N>  Screen height (default: terminal)\n");
    printf("  -h, --help        Show this help\n");
    printf("  -v, --version     Show version\n");
    printf("\n");
    printf("Controls:\n");
    printf("  WASD    - Move / Strafe\n");
    printf("  Arrows  - Turn left/right\n");
    printf("  F/Space - Fire weapon\n");
    printf("  E       - Use/Open door\n");
    printf("  1-7     - Select weapon\n");
    printf("  Tab     - Toggle automap\n");
    printf("  +/-     - Adjust FPS\n");
    printf("  Esc     - Pause / Quit\n");
    printf("  Q       - Quit\n");
    printf("\n");
}

static void show_version(void) {
    print_banner();
    printf("Version 1.0.0\n");
    printf("Build: %s %s\n", __DATE__, __TIME__);
    printf("Platform: %s\n",
#ifdef _WIN32
        "Windows"
#elif defined(__APPLE__)
        "macOS"
#else
        "Linux"
#endif
    );
    printf("Compiler: %s\n", __VERSION__);
}

static int parse_args(int argc, char **argv) {
    gs.start_level = 0;
    gs.difficulty = 1;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            show_help();
            exit(0);
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            show_version();
            exit(0);
        } else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--level") == 0) {
            if (i + 1 < argc) gs.start_level = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--difficulty") == 0) {
            if (i + 1 < argc) gs.difficulty = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--fps") == 0) {
            if (i + 1 < argc) gs.target_fps = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--width") == 0) {
            if (i + 1 < argc) gs.screen_w = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--height") == 0) {
            if (i + 1 < argc) gs.screen_h = atoi(argv[++i]);
        }
    }
    return 0;
}

static void print_welcome_screen(void) {
    clear();
    int w = gs.screen_w;
    int y = 1;

    attron(COLOR_PAIR(make_color(200, 0, 0)) | A_BOLD);
    mvprintw(y++, (w - 30) / 2, "  ____   ___  _  _  _____");
    mvprintw(y++, (w - 30) / 2, " |  _ \\ / _ \\| \\| || ____|");
    mvprintw(y++, (w - 30) / 2, " | |_) | | | |  \\| ||  _|");
    mvprintw(y++, (w - 30) / 2, " |  __/| |_| | |\\  || |___");
    mvprintw(y++, (w - 30) / 2, " |_|    \\___/|_| \\_||_____|");
    attroff(COLOR_PAIR(make_color(200, 0, 0)) | A_BOLD);

    y++;
    attron(COLOR_PAIR(make_color(200, 200, 0)));
    mvprintw(y++, (w - 35) / 2, " Terminal FPS | Raycasting 3D Engine");
    mvprintw(y++, (w - 35) / 2, " DOOM Clone   | Version 1.0.0");
    attroff(COLOR_PAIR(make_color(200, 200, 0)));

    y += 2;
    attron(COLOR_PAIR(make_color(0, 200, 0)));
    mvprintw(y++, (w - 40) / 2, "         Controls");
    attroff(COLOR_PAIR(make_color(0, 200, 0)));

    attron(COLOR_PAIR(make_color(200, 200, 200)));
    mvprintw(y++, (w - 40) / 2, " WASD    - Move / Strafe");
    mvprintw(y++, (w - 40) / 2, " Arrows  - Turn left/right");
    mvprintw(y++, (w - 40) / 2, " F/Space - Fire weapon");
    mvprintw(y++, (w - 40) / 2, " E       - Use/Open door");
    mvprintw(y++, (w - 40) / 2, " 1-7     - Select weapon");
    mvprintw(y++, (w - 40) / 2, " Tab     - Toggle automap");
    mvprintw(y++, (w - 40) / 2, " +/-     - Adjust FPS");
    mvprintw(y++, (w - 40) / 2, " Esc     - Pause / Quit");
    attroff(COLOR_PAIR(make_color(200, 200, 200)));

    y += 2;
    attron(COLOR_PAIR(make_color(100, 100, 100)));
    mvprintw(y++, (w - 40) / 2, " Press any key to start...");
    attroff(COLOR_PAIR(make_color(100, 100, 100)));

    y += 2;
    attron(COLOR_PAIR(make_color(80, 80, 80)));
    mvprintw(y++, (w - 40) / 2, " Built-in levels: 3 (Hangar, Nuclear, Toxic)");
    mvprintw(y++, (w - 40) / 2, " No WAD files required!");
    attroff(COLOR_PAIR(make_color(80, 80, 80)));

    refresh();
}

static void handle_input(void) {
    int ch;
    while ((ch = getch()) != ERR) {
        switch (ch) {
            case 'q': case 'Q': case 27:
                gs.running = 0;
                break;
            case 'w': case 'W':
                player_move(&gs, cosf(gs.player.angle) * MOVE_SPEED, sinf(gs.player.angle) * MOVE_SPEED);
                break;
            case 's': case 'S':
                player_move(&gs, -cosf(gs.player.angle) * MOVE_SPEED, -sinf(gs.player.angle) * MOVE_SPEED);
                break;
            case 'a': case 'A':
                player_move(&gs, cosf(gs.player.angle - PI / 2) * MOVE_SPEED, sinf(gs.player.angle - PI / 2) * MOVE_SPEED);
                break;
            case 'd': case 'D':
                player_move(&gs, cosf(gs.player.angle + PI / 2) * MOVE_SPEED, sinf(gs.player.angle + PI / 2) * MOVE_SPEED);
                break;
            case KEY_LEFT:
                gs.player.angle -= TURN_SPEED;
                break;
            case KEY_RIGHT:
                gs.player.angle += TURN_SPEED;
                break;
            case 'f': case 'F': case ' ':
                gs.attack_held = 1;
                if (gs.player.attack_timer <= 0)
                    weapon_fire(&gs);
                break;
            case 'e': case 'E':
                check_doors(&gs);
                break;
            case '1': gs.player.weapon = WPN_FIST; break;
            case '2': if (gs.player.has_weapon[WPN_PISTOL]) gs.player.weapon = WPN_PISTOL; break;
            case '3': if (gs.player.has_weapon[WPN_SHOTGUN]) gs.player.weapon = WPN_SHOTGUN; break;
            case '4': if (gs.player.has_weapon[WPN_CHAINGUN]) gs.player.weapon = WPN_CHAINGUN; break;
            case '5': if (gs.player.has_weapon[WPN_ROCKET]) gs.player.weapon = WPN_ROCKET; break;
            case '6': if (gs.player.has_weapon[WPN_PLASMA]) gs.player.weapon = WPN_PLASMA; break;
            case '7': if (gs.player.has_weapon[WPN_BFG]) gs.player.weapon = WPN_BFG; break;
            case '\t':
                if (gs.map_timer <= 0) {
                    gs.show_map = !gs.show_map;
                    gs.map_timer = 15;
                }
                break;
            case '=': case '+':
                gs.target_fps += 5;
                if (gs.target_fps > 120) gs.target_fps = 120;
                break;
            case '-': case '_':
                gs.target_fps -= 5;
                if (gs.target_fps < 10) gs.target_fps = 10;
                break;
            case KEY_RESIZE:
                getmaxyx(stdscr, screen_h, screen_w);
                gs.screen_w = screen_w;
                gs.screen_h = screen_h;
                free(gs.zbuffer);
                gs.zbuffer = (float *)calloc(screen_w, sizeof(float));
                render_cleanup(&gs);
                render_init(&gs);
                break;
        }
    }
}

static void check_death(GameState *gs) {
    if (gs->player.health <= 0 && !gs->game_over) {
        gs->game_over = 1;
        gs->game_over_timer = 0;
    }
    if (gs->game_over) {
        gs->game_over_timer++;
        if (gs->game_over_timer > 90) {
            gs->running = 0;
        }
    }
}

static void draw_death_screen(void) {
    int w = gs.screen_w, h = gs.screen_h;
    int cy = h / 2 - 2;

    attron(COLOR_PAIR(make_color(200, 0, 0)) | A_BOLD);
    mvprintw(cy++, (w - 20) / 2, "   ____  _____  ____");
    mvprintw(cy++, (w - 20) / 2, "  / ___|| ____|/ ___|");
    mvprintw(cy++, (w - 20) / 2, " | |    |  _|  \\___ \\");
    mvprintw(cy++, (w - 20) / 2, " | |___ | |___  ___) |");
    mvprintw(cy++, (w - 20) / 2, "  \\____||_____|____/");
    attroff(COLOR_PAIR(make_color(200, 0, 0)) | A_BOLD);

    cy++;
    attron(COLOR_PAIR(make_color(200, 200, 0)));
    mvprintw(cy++, (w - 20) / 2, " Kills: %d", gs.player.kills);
    mvprintw(cy++, (w - 20) / 2, " Score: %d", gs.score);
    attroff(COLOR_PAIR(make_color(200, 200, 0)));

    cy++;
    attron(COLOR_PAIR(make_color(100, 100, 100)));
    mvprintw(cy++, (w - 40) / 2, " Press Q to quit");
    attroff(COLOR_PAIR(make_color(100, 100, 100)));
    refresh();
}

static void draw_pause_screen(void) {
    int w = gs.screen_w, h = gs.screen_h;
    int cy = h / 2 - 2;
    attron(COLOR_PAIR(make_color(200, 200, 0)) | A_BOLD);
    mvprintw(cy, (w - 6) / 2, "PAUSED");
    attroff(COLOR_PAIR(make_color(200, 200, 0)) | A_BOLD);
    cy += 2;
    attron(COLOR_PAIR(make_color(100, 100, 100)));
    mvprintw(cy, (w - 30) / 2, " Press ESC or P to resume");
    attroff(COLOR_PAIR(make_color(100, 100, 100)));
    refresh();
}

static int game_loop(void) {
    gs.running = 1;
    gs.game_over = 0;
    gs.game_over_timer = 0;
    gs.paused = 0;
    gs.attack_held = 0;
    gs.map_timer = 0;
    last_time = (int)time(NULL);

    while (gs.running) {
        int now = (int)time(NULL);
        float dt = (now - last_time) > 0 ? 1.0f : 1.0f / gs.target_fps;
        last_time = now;

        handle_input();

        if (gs.paused) {
            draw_pause_screen();
            napms(16);
            continue;
        }

        if (gs.player.attack_timer > 0) gs.player.attack_timer--;
        if (gs.player.pain_timer > 0) gs.player.pain_timer--;
        if (gs.map_timer > 0) gs.map_timer--;

        if (gs.attack_held && gs.player.attack_timer <= 0 && !gs.game_over) {
            weapon_fire(&gs);
        }

        game_update(&gs, dt);
        check_death(&gs);

        if (!gs.game_over) {
            render_frame(&gs);
        } else {
            draw_death_screen();
        }

        anim_frame++;
        napms(1000 / gs.target_fps);
    }

    return gs.exit_reached ? 1 : 0;
}

int main(int argc, char **argv) {
    print_banner();

    if (!check_deps()) {
        printf("Missing dependencies!\n");
        printf("Run: boom_setup to install\n");
        return 1;
    }

    init_ncurses();

    memset(&gs, 0, sizeof(gs));
    gs.screen_w = screen_w;
    gs.screen_h = screen_h;
    gs.zbuffer = (float *)calloc(screen_w, sizeof(float));
    render_init(&gs);

    parse_args(argc, argv);

    load_level(gs.start_level);

    print_welcome_screen();
    nodelay(stdscr, FALSE);
    getch();
    nodelay(stdscr, TRUE);

    int result = game_loop();

    cleanup();
    free(gs.zbuffer);
    return result;
}
