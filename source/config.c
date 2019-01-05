#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "config.h"
#include "fs.h"

#define CONFIG_VERSION 0

nxshell_config_t config;
static int config_version_holder = 0;

const char *configFile =
	"config_ver = %d\n"
	"theme = %d\n"
	"dev_options = %d\n"
	"sort = %d\n";

Result Config_Save(nxshell_config_t config) {
	Result ret = 0;

	char *buf = (char *)malloc(64);
	snprintf(buf, 64, configFile, CONFIG_VERSION, config.dark_theme, config.dev_options, config.sort);

	if (R_FAILED(ret = FS_Write(fs, "/switch/NX-Shell/config.cfg", buf))) {
		free(buf);
		return ret;
	}

	free(buf);
	return 0;
}

Result Config_Load(void) {
	Result ret = 0;

	if (!FS_DirExists(fs, "/switch/"))
		FS_MakeDir(fs, "/switch");
	if (!FS_DirExists(fs, "/switch/NX-Shell/"))
		FS_MakeDir(fs, "/switch/NX-Shell");

	if (!FS_FileExists(fs, "/switch/NX-Shell/config.cfg")) {
		config.dark_theme = false;
		config.dev_options = false;
		config.sort = 0;
		return Config_Save(config);
	}

	u64 size = 0;
	FS_GetFileSize(fs, "/switch/NX-Shell/config.cfg", &size);
	char *buf = (char *)malloc(size + 1);

	if (R_FAILED(ret = FS_Read(fs, "/switch/NX-Shell/config.cfg", size, buf))) {
		free(buf);
		return ret;
	}

	buf[size] = '\0';
	sscanf(buf, configFile, &config_version_holder, &config.dark_theme, &config.dev_options, &config.sort);
	free(buf);

	// Delete config file if config file is updated. This will rarely happen.
	if (config_version_holder  < CONFIG_VERSION) {
		FS_RemoveFile(fs, "/switch/NX-Shell/config.cfg");
		config.dark_theme = false;
		config.dev_options = false;
		config.sort = 0;
		return Config_Save(config);
	}

	return 0;
}

Result Config_GetLastDirectory(void) {
	Result ret = 0;

	if (!FS_FileExists(fs, "/switch/NX-Shell/lastdir.txt")) {
		if (R_FAILED(ret = FS_Write(fs, "/switch/NX-Shell/lastdir.txt", START_PATH))) {
			strcpy(cwd, START_PATH); // Set Start Path to "sdmc:/" if lastdir.txt hasn't been created.
			return ret;
		}

		strcpy(cwd, START_PATH);
	}
	else {
		u64 size = 0;
		FS_GetFileSize(fs, "/switch/NX-Shell/lastdir.txt", &size);
		char *buf = (char *)malloc(size + 1);

		if (R_FAILED(ret = FS_Read(fs, "/switch/NX-Shell/lastdir.txt", size, buf))) {
			free(buf);
			return ret;
		}

		buf[size] = '\0';
		char temp_path[FS_MAX_PATH];
		sscanf(buf, "%[^\n]s", temp_path);

		if (FS_DirExists(fs, temp_path)) // Incase a directory previously visited had been deleted, set start path to sdmc:/ to avoid errors.
			strcpy(cwd, temp_path);
		else
			strcpy(cwd, START_PATH);
		
		free(buf);
	}

	return 0;
}
