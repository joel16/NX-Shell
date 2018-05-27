#include <stdio.h>
#include <stdlib.h>
#include <switch.h>

#include "config.h"
#include "fs.h"

const char * configFile =
	"theme = %d";

int Config_Save(bool dark_theme)
{
	char *buf = (char *)malloc(64);
	snprintf(buf, 64, configFile, dark_theme);

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
		dark_theme = false;
		return Config_Save(false);
	}

	u64 size = FS_GetFileSize("/switch/NX-Shell/config.cfg");
	char *buf = (char *)malloc(size + 1);

	FILE *file = fopen("/switch/NX-Shell/config.cfg", "r");
	fgets(buf, size + 1, file);
	fclose(file);

	buf[size] = '\0';
	sscanf(buf, configFile, &dark_theme);
	free(buf);

	return 0;
}