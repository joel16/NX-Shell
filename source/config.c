#include <stdio.h>
#include <stdlib.h>
#include <switch.h>

#include "config.h"
#include "fs.h"

bool config_dark_theme;
int config_sort_by;

const char *configFile =
	"theme = %d\n"
	"sortBy = %d\n";

int Config_Save(bool config_dark_theme, int config_sort_by)
{
	char *buf = (char *)malloc(64);
	snprintf(buf, 64, configFile, config_dark_theme, config_sort_by);

	FILE *file = fopen("/switch/NX-Shell/config.cfg", "w");
	fprintf(file, buf);
	fclose(file);

	free(buf);
	return 0;
}

int Config_Load(void)
{
	if (!FS_FileExists("/switch/NX-Shell/config.cfg"))
	{
		config_dark_theme = false;
		config_sort_by = 0;
		return Config_Save(config_dark_theme, config_sort_by);
	}

	u64 size = FS_GetFileSize("/switch/NX-Shell/config.cfg");
	char *buf = (char *)malloc(size + 1);

	FILE *file = fopen("/switch/NX-Shell/config.cfg", "r");
	fread(buf, size, 1, file);
	fclose(file);

	buf[size] = '\0';
	sscanf(buf, configFile, &config_dark_theme, &config_sort_by);
	free(buf);

	return 0;
}