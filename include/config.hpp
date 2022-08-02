#pragma once

#include <string>
#include <switch.h>

typedef struct {
    int lang = 1;
    bool dev_options = false;
    bool image_filename = false;
} config_t;

extern config_t cfg;
extern char cwd[FS_MAX_PATH];

namespace Config {
    int Save(config_t &config);
    int Load(void);
}
