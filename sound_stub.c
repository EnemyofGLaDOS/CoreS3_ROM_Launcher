/*
 * sound_stub.c — Silent audio stubs
 * Replaces sound.c and sound_bridge.cpp entirely.
 */

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <noftypes.h>
#include "nofrendo_compat.h"

int osd_init_sound(void)   { return 0; }
void osd_stopsound(void)   {}
void do_audio_frame(void)  {}
void osd_setsound(void (*playfunc)(void *buffer, int length)) { (void)playfunc; }

void osd_getsoundinfo(sndinfo_t *info)
{
    info->sample_rate = 22050;
    info->bps = 16;
}
