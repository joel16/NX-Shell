#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <switch.h>

#include "fs.h"

Result FS_MakeDir(const char *path) {
	Result ret = 0;

	if (R_FAILED(ret = fsFsCreateDirectory(&fs, path)))
		return ret;

	return 0;
}

bool FS_FileExists(const char *path) {
	FsFile file;

	if (R_SUCCEEDED(fsFsOpenFile(&fs, path, FS_OPEN_READ, &file))) {
		fsFileClose(&file);
		return true;
	}

	return false;
}

bool FS_DirExists(const char *path) {
	FsDir dir;

	if (R_SUCCEEDED(fsFsOpenDirectory(&fs, path, FS_DIROPEN_DIRECTORY | FS_DIROPEN_FILE, &dir))) {
		fsDirClose(&dir);
		return true;
	}

	return false;
}

const char *FS_GetFileExt(const char *filename) {
	const char *dot = strrchr(filename, '.');
	
	if (!dot || dot == filename)
		return "";
	
	return dot + 1;
}

Result FS_GetFileSize(const char *path, u64 *size) {
	FsFile file;
	Result ret = 0;

	if (R_FAILED(ret = fsFsOpenFile(&fs, path, FS_OPEN_READ, &file)))
		return ret;

	if (R_FAILED(ret = fsFileGetSize(&file, size)))
		return ret;

	fsFileClose(&file);
	return 0;
}
