#pragma once

#include <glad/glad.h>
#include <switch.h>
#include <vector>

typedef struct {
    GLuint id = 0;
    int width = 0;
    int height = 0;
    int delay = 0;
} Tex;

extern std::vector<Tex> file_icons;
extern Tex folder_icon, check_icon, uncheck_icon;

namespace Textures {
    bool LoadImageFile(const char path[FS_MAX_PATH], std::vector<Tex> &textures);
    void Free(Tex &texture);
    void Init(void);
    void Exit(void);
}
