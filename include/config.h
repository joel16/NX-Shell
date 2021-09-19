
#ifndef NX_SHELL_CONFIG_H
#define NX_SHELL_CONFIG_H

#include <switch.h>

typedef struct {
    int lang = 1;
    bool dev_options = false;
    bool image_filename = false;
    char cwd[FS_MAX_PATH + 1] = "/";
} config_t;

extern config_t cfg;

namespace Config {
    int Save(config_t &config);
    int Load(void);
}

#endif
