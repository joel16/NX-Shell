#ifndef NX_SHELL_TEXTURES_H
#define NX_SHELL_TEXTURES_H

#include <switch.h>
#include <vector>

#define NUM_FILE_ICONS 4

typedef struct {
    u32 image_id = 1;
    u32 sampler_id = 1;
    int width = 0;
    int height = 0;
    int delay = 0;
} Tex;

extern Tex folder_icon, file_icons[NUM_FILE_ICONS], check_icon, uncheck_icon;

namespace Textures {
    bool LoadImageFile(const char path[FS_MAX_PATH], std::vector<Tex> &textures);
    void Free(Tex *texture);
    void Init(void);
    void Exit(void);
}

#endif
