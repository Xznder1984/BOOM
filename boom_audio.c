/*
 * BOOM - Terminal audio (beeps)
 */
#include "boom.h"
#include <ncurses.h>

static int audio_enabled = 1;
static int audio_volume = 50;

void audio_init(void) {
    audio_enabled = 1;
}

void audio_cleanup(void) {
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
    switch (sound_id) {
        case SND_PISTOL: audio_beep(800, 50); break;
        case SND_SHOTGUN: audio_beep(400, 100); break;
        case SND_CHAINGUN: audio_beep(600, 30); break;
        case SND_ROCKET: audio_beep(200, 150); break;
        case SND_PLASMA: audio_beep(1000, 80); break;
        case SND_BFG: audio_beep(1500, 200); break;
        case SND_PUNCH: audio_beep(300, 20); break;
        case SND_DOOR: audio_beep(500, 100); break;
        case SND_ITEM: audio_beep(1200, 50); break;
        case SND_HURT: audio_beep(250, 80); break;
        case SND_DEATH: audio_beep(150, 200); break;
        case SND_PICKUP: audio_beep(1500, 40); break;
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
