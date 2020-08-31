
#ifndef NX_SHELL_CONFIG_H
#define NX_SHELL_CONFIG_H

#include <switch.h>

typedef struct {
	int sort = 0;
	bool dark_theme = false;
	bool image_filename = false;
	char cwd[FS_MAX_PATH + 1];
} config_t;

extern config_t config;

namespace Config {
	int Save(config_t config);
	int Load(void);
}

#endif
