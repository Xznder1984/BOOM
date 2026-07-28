/*
 * BOOM - Main entry point, input handling, mouse support, game loop
 */
#include "boom.h"
#include <ncurses.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

static GameState gs;
static int last_time = 0;
static int anim_frame = 0;

static int screen_w, screen_h;
static Level levels[NUM_LEVELS];
static int current_level = 0;
static int level_loaded = 0;
static char custom_wad_path[256] = {0};

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

static void load_custom_wad(const char *path) {
    Level lvl;
    if (wad_load_level(path, &lvl) == 0) {
        levels[0] = lvl;
        current_level = 0;
        level_loaded = 1;
        gs.level = levels[0];
        game_init(&gs);
    }
}

static void init_ncurses(void) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    curs_set(0);
    start_color();

    /* Enable mouse support */
    mousemask(BUTTON1_CLICKED | BUTTON1_PRESSED | REPORT_MOUSE_POSITION, NULL);
    mouseinterval(0);

    /* Try to enable xterm mouse tracking for motion reports */
    printf("\033[?1003h"); /* enable all mouse motion tracking */
    fflush(stdout);

    getmaxyx(stdscr, screen_h, screen_w);

    memset(&gs, 0, sizeof(gs));
    gs.screen_w = screen_w;
    gs.screen_h = screen_h;
    gs.mouse_enabled = 1;
    gs.mouse_dx = 0;
    gs.mouse_dy = 0;
    gs.mouse_fire = 0;
    gs.mouse_prev_x = -1;
    render_init(&gs);
}

static void cleanup(void) {
    /* Disable mouse tracking */
    printf("\033[?1003l");
    fflush(stdout);
    render_cleanup(&gs);
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
    printf(" DOOM Clone   | Version %s\n", BOOM_VERSION);
    printf("=================================\n");
    printf("\n");
}

static void show_help(void) {
    print_banner();
    printf("Usage: boom [OPTIONS]\n\n");
    printf("Options:\n");
    printf("  -l, --level <N>   Start at level N (0-%d)\n", NUM_LEVELS - 1);
    printf("  -wad <file>       Load custom .boomwad level file\n");
    printf("  -save <file>      Save current level as .boomwad after playing\n");
    printf("  -d, --difficulty <N>  Difficulty: 0=easy, 1=normal, 2=hard\n");
    printf("  -f, --fps <N>     Target FPS (default: 30)\n");
    printf("  -m, --no-mouse    Disable mouse support\n");
    printf("  -h, --help        Show this help\n");
    printf("  -v, --version     Show version\n");
    printf("\n");
    printf("Controls:\n");
    printf("  WASD      - Move / Strafe\n");
    printf("  Arrows    - Turn left/right\n");
    printf("  F/Space   - Fire weapon\n");
    printf("  E         - Use/Open door\n");
    printf("  1-7       - Select weapon\n");
    printf("  Tab       - Toggle automap\n");
    printf("  +/-       - Adjust FPS\n");
    printf("  M         - Toggle sound\n");
    printf("  Esc       - Pause / Quit\n");
    printf("  Q         - Quit\n");
    printf("\n");
    printf("Mouse:\n");
    printf("  Move mouse - Look left/right\n");
    printf("  Left click - Fire weapon\n");
    printf("  (Auto-detected, use -m to disable)\n");
    printf("\n");
}

static void show_version(void) {
    print_banner();
    printf("Version %s\n", BOOM_VERSION);
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
    custom_wad_path[0] = '\0';

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            show_help();
            exit(0);
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            show_version();
            exit(0);
        } else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--level") == 0) {
            if (i + 1 < argc) gs.start_level = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--wad") == 0) {
            if (i + 1 < argc) strncpy(custom_wad_path, argv[++i], 255);
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--difficulty") == 0) {
            if (i + 1 < argc) gs.difficulty = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--fps") == 0) {
            if (i + 1 < argc) gs.target_fps = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--no-mouse") == 0) {
            gs.mouse_enabled = 0;
        } else if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--width") == 0) {
            if (i + 1 < argc) gs.screen_w = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--height") == 0) {
            if (i + 1 < argc) gs.screen_h = atoi(argv[++i]);
        } else if (argv[i][0] != '-') {
            /* Bare argument = WAD file path */
            strncpy(custom_wad_path, argv[i], 255);
        }
    }
    return 0;
}

static void print_welcome_screen(void) {
    /* Use direct ANSI writes for the welcome screen */
    char buf[8192];
    int b = 0;

    b += snprintf(buf + b, sizeof(buf) - b, "\033[2J\033[H"); /* clear */
    b += snprintf(buf + b, sizeof(buf) - b, "\033[?25l"); /* hide cursor */

    int w = gs.screen_w;
    int y = 2;

    /* Title in red */
    b += snprintf(buf + b, sizeof(buf) - b, "\033[%d;%dH\033[38;2;220;30;30m\033[1m", y, (w - 30) / 2);
    b += snprintf(buf + b, sizeof(buf) - b, "  ____   ___  _  _  _____"); y++;
    b += snprintf(buf + b, sizeof(buf) - b, "\033[%d;%dH", y, (w - 30) / 2);
    b += snprintf(buf + b, sizeof(buf) - b, " |  _ \\ / _ \\| \\| || ____|"); y++;
    b += snprintf(buf + b, sizeof(buf) - b, "\033[%d;%dH", y, (w - 30) / 2);
    b += snprintf(buf + b, sizeof(buf) - b, " | |_) | | | |  \\| ||  _|"); y++;
    b += snprintf(buf + b, sizeof(buf) - b, "\033[%d;%dH", y, (w - 30) / 2);
    b += snprintf(buf + b, sizeof(buf) - b, " |  __/| |_| | |\\  || |___"); y++;
    b += snprintf(buf + b, sizeof(buf) - b, "\033[%d;%dH", y, (w - 30) / 2);
    b += snprintf(buf + b, sizeof(buf) - b, " |_|    \\___/|_| \\_||_____|\033[0m"); y += 2;

    /* Subtitle in yellow */
    b += snprintf(buf + b, sizeof(buf) - b, "\033[%d;%dH\033[38;2;255;220;0m", y, (w - 35) / 2);
    b += snprintf(buf + b, sizeof(buf) - b, " True Color ANSI | Raycasting 3D Engine"); y++;
    b += snprintf(buf + b, sizeof(buf) - b, "\033[%d;%dH", y, (w - 35) / 2);
    b += snprintf(buf + b, sizeof(buf) - b, " DOOM Clone      | Version %s\033[0m", BOOM_VERSION); y += 2;

    /* Controls header */
    b += snprintf(buf + b, sizeof(buf) - b, "\033[%d;%dH\033[38;2;0;200;0m", y, (w - 40) / 2);
    b += snprintf(buf + b, sizeof(buf) - b, "           Controls"); y++;

    /* Controls */
    b += snprintf(buf + b, sizeof(buf) - b, "\033[%d;%dH\033[38;2;200;200;200m", y, (w - 40) / 2);
    b += snprintf(buf + b, sizeof(buf) - b, " WASD      - Move / Strafe"); y++;
    b += snprintf(buf + b, sizeof(buf) - b, "\033[%d;%dH", y, (w - 40) / 2);
    b += snprintf(buf + b, sizeof(buf) - b, " Arrows    - Turn left/right"); y++;
    b += snprintf(buf + b, sizeof(buf) - b, "\033[%d;%dH", y, (w - 40) / 2);
    b += snprintf(buf + b, sizeof(buf) - b, " F/Space   - Fire weapon"); y++;
    b += snprintf(buf + b, sizeof(buf) - b, "\033[%d;%dH", y, (w - 40) / 2);
    b += snprintf(buf + b, sizeof(buf) - b, " E         - Use/Open door"); y++;
    b += snprintf(buf + b, sizeof(buf) - b, "\033[%d;%dH", y, (w - 40) / 2);
    b += snprintf(buf + b, sizeof(buf) - b, " 1-7       - Select weapon"); y++;
    b += snprintf(buf + b, sizeof(buf) - b, "\033[%d;%dH", y, (w - 40) / 2);
    b += snprintf(buf + b, sizeof(buf) - b, " Tab       - Toggle automap"); y++;
    b += snprintf(buf + b, sizeof(buf) - b, "\033[%d;%dH", y, (w - 40) / 2);
    b += snprintf(buf + b, sizeof(buf) - b, " +/-       - Adjust FPS"); y++;
    b += snprintf(buf + b, sizeof(buf) - b, "\033[%d;%dH", y, (w - 40) / 2);
    b += snprintf(buf + b, sizeof(buf) - b, " M         - Toggle sound"); y++;
    b += snprintf(buf + b, sizeof(buf) - b, "\033[%d;%dH", y, (w - 40) / 2);
    b += snprintf(buf + b, sizeof(buf) - b, " Esc       - Pause / Quit\033[0m"); y += 2;

    /* Mouse controls */
    b += snprintf(buf + b, sizeof(buf) - b, "\033[%d;%dH\033[38;2;0;150;255m", y, (w - 40) / 2);
    b += snprintf(buf + b, sizeof(buf) - b, "         Mouse Controls"); y++;
    b += snprintf(buf + b, sizeof(buf) - b, "\033[%d;%dH\033[38;2;200;200;200m", y, (w - 40) / 2);
    b += snprintf(buf + b, sizeof(buf) - b, " Move mouse  - Look left/right"); y++;
    b += snprintf(buf + b, sizeof(buf) - b, "\033[%d;%dH", y, (w - 40) / 2);
    b += snprintf(buf + b, sizeof(buf) - b, " Left click  - Fire weapon\033[0m"); y += 2;

    /* Footer */
    b += snprintf(buf + b, sizeof(buf) - b, "\033[%d;%dH\033[38;2;100;100;100m", y, (w - 40) / 2);
    b += snprintf(buf + b, sizeof(buf) - b, " Press any key to start..."); y += 2;
    b += snprintf(buf + b, sizeof(buf) - b, "\033[%d;%dH\033[38;2;80;80;80m", y, (w - 40) / 2);
    b += snprintf(buf + b, sizeof(buf) - b, " Built-in levels: 3 (Hangar, Nuclear, Toxic)");
    b += snprintf(buf + b, sizeof(buf) - b, "\033[0m\033[%d;%dH", gs.screen_h, 1);

    write(STDOUT_FILENO, buf, b);
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
            case 'm': case 'M':
                audio_toggle();
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
                render_cleanup(&gs);
                render_init(&gs);
                break;
            default:
                /* Mouse events via ncurses */
                if (ch == KEY_MOUSE && gs.mouse_enabled) {
                    MEVENT event;
                    if (getmouse(&event) == OK) {
                        int mx = event.x;
                        int bstate = event.bstate;

                        /* Mouse click = fire */
                        if (bstate & BUTTON1_CLICKED) {
                            gs.attack_held = 1;
                            if (gs.player.attack_timer <= 0)
                                weapon_fire(&gs);
                        }
                        if (bstate & BUTTON1_RELEASED) {
                            gs.attack_held = 0;
                        }

                        /* Mouse X movement = turning (relative) */
                        if (gs.mouse_prev_x >= 0) {
                            int dx = mx - gs.mouse_prev_x;
                            gs.player.angle += dx * MOUSE_SENSITIVITY;
                        }
                        gs.mouse_prev_x = mx;

                        /* Mouse Y for look up/down could go here
                         * (not implemented - DOOM doesn't have free look) */
                    }
                }
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
    char buf[4096];
    int b = 0;
    int w = gs.screen_w, h = gs.screen_h;
    int cy = h / 2 - 3;

    /* Dark overlay */
    b += snprintf(buf + b, sizeof(buf) - b, "\033[2J\033[H");

    /* Title */
    b += snprintf(buf + b, sizeof(buf) - b, "\033[%d;%dH\033[38;2;220;20;20m\033[1m",
                  cy++, (w - 20) / 2);
    b += snprintf(buf + b, sizeof(buf) - b, "   ____  _____  ____");
    b += snprintf(buf + b, sizeof(buf) - b, "\033[%d;%dH", cy++, (w - 20) / 2);
    b += snprintf(buf + b, sizeof(buf) - b, "  / ___|| ____|/ ___|");
    b += snprintf(buf + b, sizeof(buf) - b, "\033[%d;%dH", cy++, (w - 20) / 2);
    b += snprintf(buf + b, sizeof(buf) - b, " | |    |  _|  \\___ \\");
    b += snprintf(buf + b, sizeof(buf) - b, "\033[%d;%dH", cy++, (w - 20) / 2);
    b += snprintf(buf + b, sizeof(buf) - b, " | |___ | |___  ___) |");
    b += snprintf(buf + b, sizeof(buf) - b, "\033[%d;%dH", cy++, (w - 20) / 2);
    b += snprintf(buf + b, sizeof(buf) - b, "  \\____||_____|____/");

    cy += 2;
    b += snprintf(buf + b, sizeof(buf) - b, "\033[%d;%dH\033[38;2;255;220;0m", cy++, (w - 20) / 2);
    b += snprintf(buf + b, sizeof(buf) - b, " Kills: %d", gs.player.kills);
    b += snprintf(buf + b, sizeof(buf) - b, "\033[%d;%dH", cy++, (w - 20) / 2);
    b += snprintf(buf + b, sizeof(buf) - b, " Score: %d", gs.score);

    cy += 2;
    b += snprintf(buf + b, sizeof(buf) - b, "\033[%d;%dH\033[38;2;150;150;150m",
                  cy, (w - 30) / 2);
    b += snprintf(buf + b, sizeof(buf) - b, " Press Q to quit\033[0m");

    write(STDOUT_FILENO, buf, b);
}

static void draw_pause_screen(void) {
    char buf[1024];
    int b = 0;
    int w = gs.screen_w, h = gs.screen_h;

    b += snprintf(buf + b, sizeof(buf) - b, "\033[%d;%dH\033[38;2;255;255;0m\033[1m",
                  h / 2, (w - 6) / 2);
    b += snprintf(buf + b, sizeof(buf) - b, " PAUSED");
    b += snprintf(buf + b, sizeof(buf) - b, "\033[%d;%dH\033[38;2;150;150;150m\033[0m",
                  h / 2 + 2, (w - 30) / 2);
    b += snprintf(buf + b, sizeof(buf) - b, " Press ESC or P to resume\033[0m");

    write(STDOUT_FILENO, buf, b);
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

    parse_args(argc, argv);

    init_ncurses();

    /* Re-init after parse_args may have changed mouse_enabled */
    if (!gs.mouse_enabled) {
        mousemask(0, NULL);
        printf("\033[?1003l");
        fflush(stdout);
    }

    /* Load level */
    if (custom_wad_path[0]) {
        load_custom_wad(custom_wad_path);
    } else {
        load_level(gs.start_level);
    }

    print_welcome_screen();
    nodelay(stdscr, FALSE);
    getch();
    nodelay(stdscr, TRUE);

    int result = game_loop();

    cleanup();
    return result;
}
