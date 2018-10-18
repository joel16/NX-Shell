#ifndef NX_SHELL_CONFIG_H
#define NX_SHELL_CONFIG_H

#include <switch.h>

typedef struct {
	bool dark_theme;
	int sort;
} nxshell_config_t;

extern nxshell_config_t config;

Result Config_Save(nxshell_config_t config);
Result Config_Load(void);
Result Config_GetLastDirectory(void);

#endif
