#ifndef NX_SHELL_TEXTURES_H
#define NX_SHELL_TEXTURES_H

#include <glad/glad.h>
#include <string>

#define NUM_FILE_ICONS 5

typedef enum ImageType {
    ImageTypeJPEG,
    ImageTypePNG,
    ImageTypeWEBP,
    ImageTypeOther
} ImageType;

typedef struct {
    GLuint id = 0;
    int width = 0;
    int height = 0;
} Tex;

extern Tex folder_icon, file_icons[NUM_FILE_ICONS], check_icon, uncheck_icon;

namespace Textures {
    ImageType GetImageType(const std::string &filename);
    bool LoadImageFile(const std::string &path, ImageType type, Tex *texture);
    void Free(Tex *texture);
    void Init(void);
    void Exit(void);
}

#endif
