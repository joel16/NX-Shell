#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "common.h"
#include "config.h"
#include "fs.h"
#include "log.h"

bool FS_FileExists(FsFileSystem *fs, const char *path) {
	FsFile file;

	char temp_path[FS_MAX_PATH];
	snprintf(temp_path, FS_MAX_PATH, path);

	if (R_SUCCEEDED(fsFsOpenFile(fs, temp_path, FS_OPEN_READ, &file))) {
		fsFileClose(&file);
		return true;
	}

	return false;
}

bool FS_DirExists(FsFileSystem *fs, const char *path) {
	FsDir dir;

	char temp_path[FS_MAX_PATH];
	snprintf(temp_path, FS_MAX_PATH, path);

	if (R_SUCCEEDED(fsFsOpenDirectory(fs, temp_path, FS_DIROPEN_DIRECTORY, &dir))) {
		fsDirClose(&dir);
		return true;
	}

	return false;
}

Result FS_MakeDir(FsFileSystem *fs, const char *path) {
	Result ret = 0;

	char temp_path[FS_MAX_PATH];
	snprintf(temp_path, FS_MAX_PATH, path);

	if (R_FAILED(ret = fsFsCreateDirectory(fs, temp_path))) {
		if (config.dev_options)
			DEBUG_LOG("fsFsCreateDirectory(%s) failed: 0x%lx\n", temp_path, ret);

		return ret;
	}

	return 0;
}

Result FS_CreateFile(FsFileSystem *fs, const char *path, size_t size, int flags) {
	Result ret = 0;

	char temp_path[FS_MAX_PATH];
	snprintf(temp_path, FS_MAX_PATH, path);

	if (R_FAILED(ret = fsFsCreateFile(fs, temp_path, size, flags))) {
		if (config.dev_options)
			DEBUG_LOG("FS_CreateFile(%s, %d, %d) failed: 0x%lx\n", temp_path, size, flags, ret);

		return ret;
	}

	return 0;
}

Result FS_OpenDirectory(FsFileSystem *fs, const char *path, int flags, FsDir *dir) {
	Result ret = 0;

	char temp_path[FS_MAX_PATH];
	snprintf(temp_path, FS_MAX_PATH, path);

	if (R_FAILED(ret = fsFsOpenDirectory(fs, temp_path, flags, dir))) {
		if (config.dev_options)
			DEBUG_LOG("fsFsOpenDirectory(%s, %d) failed: 0x%lx\n", temp_path, flags, ret);

		return ret;
	}

	return 0;
}

Result FS_GetDirEntryCount(FsDir *dir, u64 *count) {
	Result ret = 0;

	if (R_FAILED(ret = fsDirGetEntryCount(dir, count))) {
		if (config.dev_options)
			DEBUG_LOG("fsDirGetEntryCount(%s) failed: 0x%lx\n", cwd, ret);

		return ret;
	}

	return 0;
}

Result FS_ReadDir(FsDir *dir, u64 inval, size_t *total_entries, size_t max_entries, FsDirectoryEntry *entry) {
	Result ret = 0;

	if (R_FAILED(ret = fsDirRead(dir, inval, total_entries, max_entries, entry))) {
		if (config.dev_options)
			DEBUG_LOG("fsDirRead(%s, %d) failed: 0x%lx\n", cwd, max_entries, ret);

		return ret;
	}

	return 0;
}

Result FS_GetFileSize(FsFileSystem *fs, const char *path, u64 *size) {
	Result ret = 0;
	FsFile file;

	char temp_path[FS_MAX_PATH];
	snprintf(temp_path, FS_MAX_PATH, path);

	if (R_FAILED(ret = fsFsOpenFile(fs, temp_path, FS_OPEN_READ, &file))) {
		fsFileClose(&file);

		if (config.dev_options)
			DEBUG_LOG("fsFsOpenFile(%s, FS_OPEN_READ, %llu) failed: 0x%lx\n", temp_path, &size, ret);

		return ret;
	}

	if (R_FAILED(ret = fsFileGetSize(&file, size))) {
		fsFileClose(&file);

		if (config.dev_options)
			DEBUG_LOG("fsFileGetSize(%s, %llu) failed: 0x%lx\n", temp_path, &size, ret);

		return ret;
	}

	fsFileClose(&file);
	return 0;
}

Result FS_RemoveFile(FsFileSystem *fs, const char *path) {
	Result ret = 0;

	char temp_path[FS_MAX_PATH];
	snprintf(temp_path, FS_MAX_PATH, path);

	if (R_FAILED(ret = fsFsDeleteFile(fs, temp_path))) {
		if (config.dev_options)
			DEBUG_LOG("fsFsDeleteFile(%s) failed: 0x%lx\n", temp_path, ret);

		return ret;
	}

	return 0;
}

Result FS_RemoveDir(FsFileSystem *fs, const char *path) {
	Result ret = 0;

	char temp_path[FS_MAX_PATH];
	snprintf(temp_path, FS_MAX_PATH, path);

	if (R_FAILED(ret = fsFsDeleteDirectory(fs, temp_path))) {
		if (config.dev_options)
			DEBUG_LOG("fsFsDeleteDirectory(%s) failed: 0x%lx\n", temp_path, ret);

		return ret;
	}

	return 0;
}

Result FS_RemoveDirRecursive(FsFileSystem *fs, const char *path) {
	Result ret = 0;

	char temp_path[FS_MAX_PATH];
	snprintf(temp_path, FS_MAX_PATH, path);

	if (R_FAILED(ret = fsFsDeleteDirectoryRecursively(fs, temp_path))) {
		if (config.dev_options)
			DEBUG_LOG("fsFsDeleteDirectoryRecursively(%s) failed: 0x%lx\n", temp_path, ret);

		return ret;
	}

	return 0;
}

Result FS_RenameFile(FsFileSystem *fs, const char *old_filename, const char *new_filename) {
	Result ret = 0;

	char temp_path_old[FS_MAX_PATH], temp_path_new[FS_MAX_PATH];
	snprintf(temp_path_old, FS_MAX_PATH, old_filename);
	snprintf(temp_path_new, FS_MAX_PATH, new_filename);

	if (R_FAILED(ret = fsFsRenameFile(fs, temp_path_old, temp_path_new))) {
		if (config.dev_options)
			DEBUG_LOG("fsFsRenameFile(%s, %s) failed: 0x%lx\n", temp_path_old, temp_path_new, ret);

		return ret;
	}

	return 0;
}

Result FS_RenameDir(FsFileSystem *fs, const char *old_dirname, const char *new_dirname) {
	Result ret = 0;

	char temp_path_old[FS_MAX_PATH], temp_path_new[FS_MAX_PATH];
	snprintf(temp_path_old, FS_MAX_PATH, old_dirname);
	snprintf(temp_path_new, FS_MAX_PATH, new_dirname);

	if (R_FAILED(ret = fsFsRenameDirectory(fs, temp_path_old, temp_path_new))) {
		if (config.dev_options)
			DEBUG_LOG("fsFsRenameDirectory(%s, %s) failed: 0x%lx\n", temp_path_old, temp_path_new, ret);

		return ret;
	}

	return 0;
}

Result FS_Read(FsFileSystem *fs, const char *path, size_t size, void *buf) {
	FsFile file;
	Result ret = 0;
	size_t out = 0;

	char temp_path[FS_MAX_PATH];
	snprintf(temp_path, FS_MAX_PATH, path);

	if (R_FAILED(ret = fsFsOpenFile(fs, temp_path, FS_OPEN_READ, &file))) {
		fsFileClose(&file);

		if (config.dev_options)
			DEBUG_LOG("fsFsOpenFile(%s, FS_OPEN_READ, %llu) failed: 0x%lx\n", temp_path, &size, ret);

		return ret;
	}
	
	if (R_FAILED(ret = fsFileRead(&file, 0, buf, size, &out))) {
		fsFileClose(&file);

		if (config.dev_options)
			DEBUG_LOG("fsFileRead(%s, %llu) failed: 0x%lx\n", temp_path, &size, ret);

		return ret;
	}

	fsFileClose(&file);
	return 0;
}

Result FS_Write(FsFileSystem *fs, const char *path, const void *buf) {
	FsFile file;
	Result ret = 0;
	u64 size = 0;
	size_t len = strlen(buf);

	char temp_path[FS_MAX_PATH];
	snprintf(temp_path, FS_MAX_PATH, path);

	appletLockExit();

	if (FS_FileExists(fs, temp_path)) {
		if (R_FAILED(ret = fsFsDeleteFile(fs, temp_path))) {
			if (config.dev_options)
				DEBUG_LOG("fsFsDeleteFile(%s) failed: 0x%lx\n", temp_path, ret);

			return ret;
		}
	}

	if (R_FAILED(ret = fsFsCreateFile(fs, temp_path, 0, 0))) {
		if (config.dev_options)
			DEBUG_LOG("fsFsCreateFile(%s) failed: 0x%lx\n", temp_path, ret);

		return ret;
	}

	if (R_FAILED(ret = fsFsOpenFile(fs, temp_path, FS_OPEN_WRITE, &file))) {
		fsFileClose(&file);

		if (config.dev_options)
			DEBUG_LOG("fsFsOpenFile(%s, FS_OPEN_WRITE) failed: 0x%lx\n", temp_path, ret);

		return ret;
	}

	if (R_FAILED(ret = fsFileGetSize(&file, &size))) {
		fsFileClose(&file);

		if (config.dev_options)
			DEBUG_LOG("fsFileGetSize(%s, %llu) failed: 0x%lx\n", temp_path, size, ret);

		return ret;
	}

	if (R_FAILED(ret = fsFileSetSize(&file, size + len))) {
		fsFileClose(&file);

		if (config.dev_options)
			DEBUG_LOG("fsFileSetSize(%s, %llu) failed: 0x%lx\n", temp_path, size + len, ret);

		return ret;
	}

	if (R_FAILED(ret = fsFileWrite(&file, 0, buf, size + len))) {
		fsFileClose(&file);

		if (config.dev_options)
			DEBUG_LOG("fsFileWrite(%s, %llu) failed: 0x%lx\n", temp_path, size + len, ret);

		return ret;
	}

	if (R_FAILED(ret = fsFileFlush(&file))) {
		fsFileClose(&file);

		if (config.dev_options)
			DEBUG_LOG("fsFileFlush(%s) failed: 0x%lx\n", temp_path, ret);

		return ret;
	}

	fsFileClose(&file);
	appletUnlockExit();
	return 0;
}

Result FS_SetArchiveBit(FsFileSystem *fs, const char *path) {
	Result ret = 0;

	char temp_path[FS_MAX_PATH];
	snprintf(temp_path, FS_MAX_PATH, path);

	if (R_FAILED(ret = fsFsSetArchiveBit(fs, temp_path))) {
		if (config.dev_options)
			DEBUG_LOG("fsFsSetArchiveBit(%s) failed: 0x%lx\n", temp_path, ret);

		return ret;
	}

	return 0;
}

Result FS_OpenBisFileSystem(FsFileSystem *fs, u32 partition_ID, const char *string) {
	Result ret = 0;

	if (R_FAILED(ret = fsOpenBisFileSystem(fs, partition_ID, string))) {
		if (config.dev_options)
			DEBUG_LOG("fsOpenBisFileSystem(%d) failed: 0x%lx\n", partition_ID, ret);

		return ret;
	}

	return 0;
}

const char *FS_GetFileExt(const char *filename) {
	const char *dot = strrchr(filename, '.');
	
	if (!dot || dot == filename)
		return "";
	
	return dot + 1;
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
