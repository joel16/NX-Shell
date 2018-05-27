#ifndef NX_SHELL_CONFIG_H
#define NX_SHELL_CONFIG_H

bool dark_theme;

int Config_Save(bool dark_theme);
int Config_Load(void);

#endif