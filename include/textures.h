#ifndef NX_SHELL_TEXTURES_H
#define NX_SHELL_TEXTURES_H

#include <glad/glad.h>
#include <string>

typedef struct {
    GLuint id = 0;
    int width = 0;
    int height = 0;
} Tex;

extern Tex folder_texture, file_texture, archive_texture, audio_texture, image_texture, check_texture, uncheck_texture;

namespace Textures {
    bool LoadImageFile(const std::string &path, Tex *texture);
    void Free(Tex *texture);
    void Init(void);
    void Exit(void);
}

#endif
