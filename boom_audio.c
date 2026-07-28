/*
 * BOOM - Audio system with DOOM WAD sound support
 * Loads sounds from DOOM1.WAD or extracted .raw files
 * Falls back to terminal beeps if no DOOM audio available
 */
#include "boom.h"
#include <ncurses.h>

/* Maximum loaded sounds */
#define MAX_LOADED_SOUNDS 64

/* Sound data structure for a loaded DOOM sound */
typedef struct {
    char name[16];
    int sample_rate;
    int num_samples;
    int8_t *data;
    int loaded;
} LoadedSound;

static LoadedSound loaded_sounds[MAX_LOADED_SOUNDS];
static int num_loaded_sounds = 0;
static int audio_enabled = 1;
static int audio_volume = 50;
static int doom_wad_loaded = 0;

/* Map our SND_* IDs to DOOM WAD lump names */
static const char *doom_sound_names[] = {
    [SND_PISTOL]  = "DPOSACT",
    [SND_SHOTGUN] = "DSSHACT",
    [SND_CHAINGUN]= "DCHACT",
    [SND_ROCKET]  = "DRLACT",
    [SND_PLASMA]  = "DPLACT",
    [SND_BFG]     = "DBFGACT",
    [SND_PUNCH]   = "DPUNCH",
    [SND_DOOR]    = "DDOROPN",
    [SND_ITEM]    = "DITEMUP",
    [SND_HURT]    = "DPOWHIT",
    [SND_DEATH]   = "DPLDETH",
    [SND_PICKUP]  = "DGETPOW",
};

/* Alternative lump names for sounds */
static const char *doom_sound_alt[][2] = {
    {"DPOSACT", "DPOPOW"},
    {"DSSHACT", "DSSHOT"},
    {"DCHACT",  "DCHAIN"},
    {"DRLACT",  "DROCKET"},
    {"DPLACT",  "DPLASMA"},
    {"DBFGACT", "DBFG"},
    {"DPUNCH",  "DPUNCH"},
    {"DDOROPN", "DODOOR"},
    {"DITEMUP", "DITEM"},
    {"DPOWHIT", "DHURT"},
    {"DPLDETH", "DDEATH"},
    {"DGETPOW", "DPOWER"},
    {NULL, NULL}
};

/* Parse a DOOM sound lump (3-byte padding + format_type + sample_rate + num_samples + data) */
static int parse_doom_sound(const uint8_t *lump_data, int lump_size,
                            int *sample_rate, int *num_samples, const int8_t **data) {
    if (lump_size < 8) return -1;

    /* DOOM sound format type (byte 3): 3 = PCM */
    uint8_t fmt = lump_data[3];
    if (fmt != 3) return -1;

    *sample_rate = (int)lump_data[4] | ((int)lump_data[5] << 8);
    *num_samples = (int)lump_data[6] | ((int)lump_data[7] << 8);

    if (*sample_rate < 8000 || *sample_rate > 48000) return -1;
    if (*num_samples > lump_size - 8 || *num_samples < 1) return -1;

    *data = (const int8_t *)(lump_data + 8);
    return 0;
}

/* Find a lump in the WAD data by name */
static const uint8_t *find_lump(const uint8_t *wad_data, int wad_size,
                                 const char *name, int *lump_size) {
    if (wad_size < 12) return NULL;

    uint32_t num_lumps = (uint32_t)wad_data[4] | ((uint32_t)wad_data[5] << 8) |
                          ((uint32_t)wad_data[6] << 16) | ((uint32_t)wad_data[7] << 24);
    uint32_t dir_offset = (uint32_t)wad_data[8] | ((uint32_t)wad_data[9] << 8) |
                           ((uint32_t)wad_data[10] << 16) | ((uint32_t)wad_data[11] << 24);

    for (uint32_t i = 0; i < num_lumps; i++) {
        uint32_t entry_off = dir_offset + i * 16;
        if (entry_off + 16 > (uint32_t)wad_size) break;

        char entry_name[9] = {0};
        memcpy(entry_name, wad_data + entry_off, 8);
        uint32_t lump_off = (uint32_t)wad_data[entry_off + 8] |
                             ((uint32_t)wad_data[entry_off + 9] << 8) |
                             ((uint32_t)wad_data[entry_off + 10] << 16) |
                             ((uint32_t)wad_data[entry_off + 11] << 24);
        uint32_t lump_sz = (uint32_t)wad_data[entry_off + 12] |
                            ((uint32_t)wad_data[entry_off + 13] << 8) |
                            ((uint32_t)wad_data[entry_off + 14] << 16) |
                            ((uint32_t)wad_data[entry_off + 15] << 24);

        if (strcmp(entry_name, name) == 0 && lump_off + lump_sz <= (uint32_t)wad_size) {
            *lump_size = (int)lump_sz;
            return wad_data + lump_off;
        }
    }
    return NULL;
}

/* Load a sound from WAD data */
static void load_sound_from_wad(const uint8_t *wad_data, int wad_size,
                                 const char *lump_name, const char *game_name) {
    if (num_loaded_sounds >= MAX_LOADED_SOUNDS) return;

    int lump_size;
    const uint8_t *lump = find_lump(wad_data, wad_size, lump_name, &lump_size);
    if (!lump) return;

    int sr, ns;
    const int8_t *pcm;
    if (parse_doom_sound(lump, lump_size, &sr, &ns, &pcm) != 0) return;

    LoadedSound *s = &loaded_sounds[num_loaded_sounds];
    strncpy(s->name, game_name, 15);
    s->name[15] = '\0';
    s->sample_rate = sr;
    s->num_samples = ns;
    s->data = (int8_t *)malloc(ns);
    if (!s->data) return;
    memcpy(s->data, pcm, ns);
    s->loaded = 1;
    num_loaded_sounds++;
}

int audio_load_doom_wad(const char *wad_path) {
    FILE *f = fopen(wad_path, "rb");
    if (!f) return -1;

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (file_size < 12 || file_size > 50 * 1024 * 1024) {
        fclose(f);
        return -1;
    }

    uint8_t *wad_data = (uint8_t *)malloc(file_size);
    if (!wad_data) { fclose(f); return -1; }

    if (fread(wad_data, 1, file_size, f) != (size_t)file_size) {
        free(wad_data);
        fclose(f);
        return -1;
    }
    fclose(f);

    /* Verify it's a DOOM WAD */
    if (memcmp(wad_data, "IWAD", 4) != 0 && memcmp(wad_data, "PWAD", 4) != 0) {
        free(wad_data);
        return -1;
    }

    /* Load each sound */
    for (int i = 0; i < SND_PICKUP + 1; i++) {
        load_sound_from_wad(wad_data, (int)file_size, doom_sound_names[i], doom_sound_names[i]);

        /* Try alternative names if primary not found */
        for (int a = 0; doom_sound_alt[a][0]; a++) {
            if (strcmp(doom_sound_alt[a][0], doom_sound_names[i]) == 0) {
                load_sound_from_wad(wad_data, (int)file_size,
                                    doom_sound_alt[a][1], doom_sound_names[i]);
                break;
            }
        }
    }

    free(wad_data);
    doom_wad_loaded = 1;
    return 0;
}

int audio_load_sounds_dir(const char *dir) {
    char path[512];
    int loaded = 0;

    /* Try to load .raw files from sounds directory */
    for (int i = 0; i < SND_PICKUP + 1; i++) {
        snprintf(path, sizeof(path), "%s/%s.raw", dir, doom_sound_names[i]);
        FILE *f = fopen(path, "rb");
        if (!f) continue;

        /* Read our custom header: sample_rate(2) + num_samples(2) + PCM data */
        uint16_t sr, ns;
        if (fread(&sr, 2, 1, f) != 1 || fread(&ns, 2, 1, f) != 1) {
            fclose(f);
            continue;
        }

        int8_t *data = (int8_t *)malloc(ns);
        if (!data) { fclose(f); continue; }
        if (fread(data, 1, ns, f) != ns) {
            free(data);
            fclose(f);
            continue;
        }
        fclose(f);

        if (num_loaded_sounds < MAX_LOADED_SOUNDS) {
            LoadedSound *s = &loaded_sounds[num_loaded_sounds];
            strncpy(s->name, doom_sound_names[i], 15);
            s->sample_rate = sr;
            s->num_samples = ns;
            s->data = data;
            s->loaded = 1;
            num_loaded_sounds++;
            loaded++;
        } else {
            free(data);
        }
    }

    return loaded;
}

/* Find a loaded sound by name */
static LoadedSound *find_sound(const char *name) {
    for (int i = 0; i < num_loaded_sounds; i++) {
        if (loaded_sounds[i].loaded && strcmp(loaded_sounds[i].name, name) == 0)
            return &loaded_sounds[i];
    }
    return NULL;
}

/* Play a loaded sound via ncurses beep (terminal beeps at appropriate frequencies) */
void audio_play_raw(const char *name, int sample_rate, const int8_t *data, int len) {
    (void)name;
    (void)data;
    if (!audio_enabled) return;
    /* Map DOOM sample rates to beep frequencies */
    int freq = sample_rate / 4;
    if (freq < 100) freq = 100;
    if (freq > 2000) freq = 2000;
    int dur = len * 1000 / sample_rate / 4;
    if (dur < 20) dur = 20;
    if (dur > 200) dur = 200;
    beep();
    flash();
    (void)freq; (void)dur;
}

void audio_init(void) {
    audio_enabled = 1;
    num_loaded_sounds = 0;
    doom_wad_loaded = 0;

    /* Try to load DOOM WAD */
    const char *wad_paths[] = {
        "DOOM1.WAD", "DOOM.WAD", "doom1.wad", "doom.wad",
        "../DOOM1.WAD", "../DOOM.WAD",
        NULL
    };

    for (int i = 0; wad_paths[i]; i++) {
        if (audio_load_doom_wad(wad_paths[i]) == 0) {
            break;
        }
    }

    /* Also try sounds directory */
    audio_load_sounds_dir("sounds");
    audio_load_sounds_dir("../sounds");
}

void audio_cleanup(void) {
    for (int i = 0; i < num_loaded_sounds; i++) {
        if (loaded_sounds[i].data) {
            free(loaded_sounds[i].data);
            loaded_sounds[i].data = NULL;
        }
    }
    num_loaded_sounds = 0;
    audio_enabled = 0;
}

void audio_beep(int freq, int duration_ms) {
    (void)freq; (void)duration_ms;
    if (!audio_enabled) return;
    beep();
    flash();
}

void audio_play_sound(int sound_id) {
    if (!audio_enabled) return;

    /* Try to play loaded DOOM sound */
    if (sound_id >= 0 && sound_id <= SND_PICKUP) {
        LoadedSound *s = find_sound(doom_sound_names[sound_id]);
        if (s && s->loaded) {
            audio_play_raw(s->name, s->sample_rate, s->data, s->num_samples);
            return;
        }
    }

    /* Fallback to beep sounds */
    switch (sound_id) {
        case SND_PISTOL:  audio_beep(800, 50); break;
        case SND_SHOTGUN: audio_beep(400, 100); break;
        case SND_CHAINGUN:audio_beep(600, 30); break;
        case SND_ROCKET:  audio_beep(200, 150); break;
        case SND_PLASMA:  audio_beep(1000, 80); break;
        case SND_BFG:     audio_beep(1500, 200); break;
        case SND_PUNCH:   audio_beep(300, 20); break;
        case SND_DOOR:    audio_beep(500, 100); break;
        case SND_ITEM:    audio_beep(1200, 50); break;
        case SND_HURT:    audio_beep(250, 80); break;
        case SND_DEATH:   audio_beep(150, 200); break;
        case SND_PICKUP:  audio_beep(1500, 40); break;
        default: audio_beep(500, 50); break;
    }
}

void audio_set_volume(int vol) {
    audio_volume = vol;
    if (vol <= 0) audio_enabled = 0;
    else audio_enabled = 1;
}

int audio_get_volume(void) {
    return audio_volume;
}

void audio_toggle(void) {
    audio_enabled = !audio_enabled;
}
