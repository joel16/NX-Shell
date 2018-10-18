#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "config.h"
#include "fs.h"

nxshell_config_t config;

const char *configFile =
	"theme = %d\n"
	"sortBy = %d\n";

Result Config_Save(nxshell_config_t config) {
	Result ret = 0;

	char *buf = (char *)malloc(64);
	snprintf(buf, 64, configFile, config.dark_theme, config.sort);

	if (R_FAILED(ret = FS_Write("/switch/NX-Shell/config.cfg", buf))) {
		free(buf);
		return ret;
	}

	free(buf);
	return 0;
}

Result Config_Load(void) {
	Result ret = 0;

	if (!FS_DirExists("/switch/"))
		FS_MakeDir("/switch");
	if (!FS_DirExists("/switch/NX-Shell/"))
		FS_MakeDir("/switch/NX-Shell");

	if (!FS_FileExists("/switch/NX-Shell/config.cfg")) {
		config.dark_theme = false;
		config.sort = 0;
		return Config_Save(config);
	}

	u64 size = 0;
	FS_GetFileSize("/switch/NX-Shell/config.cfg", &size);
	char *buf = (char *)malloc(size + 1);

	if (R_FAILED(ret = FS_Read("/switch/NX-Shell/config.cfg", size, buf))) {
		free(buf);
		return ret;
	}

	buf[size] = '\0';
	sscanf(buf, configFile, &config.dark_theme, &config.sort);
	free(buf);

	return 0;
}

Result Config_GetLastDirectory(void) {
	Result ret = 0;

	if (!FS_FileExists("/switch/NX-Shell/lastdir.txt")) {
		if (R_FAILED(ret = FS_Write("/switch/NX-Shell/lastdir.txt", START_PATH))) {
			strcpy(cwd, START_PATH); // Set Start Path to "sdmc:/" if lastdir.txt hasn't been created.
			return ret;
		}

		strcpy(cwd, START_PATH);
	}
	else {
		u64 size = 0;
		FS_GetFileSize("/switch/NX-Shell/lastdir.txt", &size);
		char *buf = (char *)malloc(size + 1);

		if (R_FAILED(ret = FS_Read("/switch/NX-Shell/lastdir.txt", size, buf))) {
			free(buf);
			return ret;
		}

		buf[size] = '\0';
		char temp_path[513];
		sscanf(buf, "%[^\n]s", temp_path);

		if (FS_DirExists(temp_path)) // Incase a directory previously visited had been deleted, set start path to sdmc:/ to avoid errors.
			strcpy(cwd, temp_path);
		else
			strcpy(cwd, START_PATH);
		
		free(buf);
	}

	return 0;
}
