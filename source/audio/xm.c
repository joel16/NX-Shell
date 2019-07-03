#include <string.h>

#include "audio.h"
#include "xmp.h"

static xmp_context xmp;
static struct xmp_frame_info frame_info;
static struct xmp_module_info module_info;
static u64 samples_read = 0, total_samples = 0;

int XM_Init(const char *path) {
    xmp = xmp_create_context();
    if (xmp_load_module(xmp, (char *)path) < 0)
        return -1;

    xmp_start_player(xmp, 44100, 0);
    xmp_get_frame_info(xmp, &frame_info);
    total_samples = (frame_info.total_time * 44.1);

    xmp_get_module_info(xmp, &module_info);
    if (module_info.mod->name[0] != '\0') {
        metadata.has_meta = true;
        strcpy(metadata.title, module_info.mod->name);
    }
    
    return 0;
}

u32 XM_GetSampleRate(void) {
    return 44100;
}

u8 XM_GetChannels(void) {
    return 2;
}

void XM_Decode(void *buf, unsigned int length, void *userdata) {
    xmp_play_buffer(xmp, buf, (int)length, 0);
    samples_read += length/(sizeof(s16) * 2);

    if (samples_read == total_samples)
        playing = false;
}

u64 XM_GetPosition(void) {
    return samples_read;
}

u64 XM_GetLength(void) {
    return total_samples;
}

u64 XM_Seek(u64 index) {
    int seek_sample = (total_samples * (index / 640.0));
    
    if (xmp_seek_time(xmp, seek_sample/44.1) >= 0) {
        samples_read = seek_sample;
        return samples_read;
    }
    
    return -1;
}

void XM_Term(void) {
    samples_read = 0;

    if (metadata.has_meta)
        metadata.has_meta = false;
    
    xmp_end_player(xmp);
    xmp_release_module(xmp);
    xmp_free_context(xmp);
}
