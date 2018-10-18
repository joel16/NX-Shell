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

int Config_Save(nxshell_config_t config) {
	char *buf = (char *)malloc(64);
	snprintf(buf, 64, configFile, config.dark_theme, config.sort);

	FILE *file = fopen("/switch/NX-Shell/config.cfg", "w");
	fprintf(file, buf);
	fclose(file);

	free(buf);
	return 0;
}

int Config_Load(void) {
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

	FILE *file = fopen("/switch/NX-Shell/config.cfg", "r");
	fread(buf, size, 1, file);
	fclose(file);

	buf[size] = '\0';
	sscanf(buf, configFile, &config.dark_theme, &config.sort);
	free(buf);

	return 0;
}

int Config_GetLastDirectory(void) {
	char *buf = (char *)malloc(256);

	if (FS_FileExists("/switch/NX-Shell/lastdir.txt")) {
		FILE *read = fopen("/switch/NX-Shell/lastdir.txt", "r");
		fscanf(read, "%s", buf);
		fclose(read);
		
		if (FS_DirExists(buf)) // Incase a directory previously visited had been deleted, set start path to sdmc:/ to avoid errors.
			strcpy(cwd, buf);
		else 
			strcpy(cwd, START_PATH);
	}
	else {
		strcpy(buf, START_PATH);
			
		FILE *write = fopen("/switch/NX-Shell/lastdir.txt", "w");
		fprintf(write, "%s", buf);
		fclose(write);
		
		strcpy(cwd, buf); // Set Start Path to "sdmc:/" if lastDir.txt hasn't been created.
	}

	free(buf);
	return 0;
}
