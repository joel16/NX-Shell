#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <switch.h>
#include <time.h>

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
	struct stat info;

	if (stat(path, &info) != 0)
		return false;
	else if (info.st_mode & S_IFDIR)
		return true;
	else
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

	if (R_FAILED(ret = fsFsOpenFile(&fs, path, FS_OPEN_READ, &file))) {
		fsFileClose(&file);
		return ret;
	}

	if (R_FAILED(ret = fsFileGetSize(&file, size))) {
		fsFileClose(&file);
		return ret;
	}

	fsFileClose(&file);
	return 0;
}

Result FS_Read(const char *path, size_t size, void *buf) {
	FsFile file;
	Result ret = 0;

	size_t out = 0;

	if (R_FAILED(ret = fsFsOpenFile(&fs, path, FS_OPEN_READ, &file))) {
		fsFileClose(&file);
		return ret;
	}
	
	if (R_FAILED(ret = fsFileRead(&file, 0, buf, size, &out))) {
		fsFileClose(&file);
		return ret;
	}

	fsFileClose(&file);
	return 0;
}

char *FS_GetFilePermission(const char *filename) {
	static char perms[11];
	struct stat attr;
	
	if (R_FAILED(stat(filename, &attr)))
		return NULL;

	snprintf(perms, 11, "%s%s%s%s%s%s%s%s%s%s", (S_ISDIR(attr.st_mode)) ? "d" : "-", (attr.st_mode & S_IRUSR) ? "r" : "-",
		(attr.st_mode & S_IWUSR) ? "w" : "-", (attr.st_mode & S_IXUSR) ? "x" : "-", (attr.st_mode & S_IRGRP) ? "r" : "-",
		(attr.st_mode & S_IWGRP) ? "w" : "-", (attr.st_mode & S_IXGRP) ? "x" : "-", (attr.st_mode & S_IROTH) ? "r" : "-",
		(attr.st_mode & S_IWOTH) ? "w" : "-", (attr.st_mode & S_IXOTH) ? "x" : "-");

	return perms;
}

Result FS_Write(const char *path, const void *buf) {
	FsFile file;
	Result ret = 0;
	
	size_t len = strlen(buf);
	u64 size = 0;

	if (FS_FileExists(path))
		fsFsDeleteFile(&fs, path);

	if (R_FAILED(ret = fsFsCreateFile(&fs, path, 0, 0))) {
		fsFileClose(&file);
		return ret;
	}

	if (R_FAILED(ret = fsFsOpenFile(&fs, path, FS_OPEN_WRITE, &file))) {
		fsFileClose(&file);
		return ret;
	}

	if (R_FAILED(ret = fsFileGetSize(&file, &size))) {
		fsFileClose(&file);
		return ret;
	}

	if (R_FAILED(ret = fsFileSetSize(&file, size + len))) {
		fsFileClose(&file);
		return ret;
	}

	if (R_FAILED(ret = fsFileWrite(&file, 0, buf, size + len))) {
		fsFileClose(&file);
		return ret;
	}

	if (R_FAILED(ret = fsFileFlush(&file))) {
		fsFileClose(&file);
		return ret;
	}

	fsFileClose(&file);
	return 0;
}
