#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "common.h"
#include "config.h"
#include "fs.h"
#include "log.h"

bool FS_FileExists(const char *path) {
	FsFile file;

	if (R_SUCCEEDED(fsFsOpenFile(BROWSE_STATE == STATE_SD? fs : &user_fs, path, FS_OPEN_READ, &file))) {
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

Result FS_MakeDir(const char *path) {
	Result ret = 0;

	if (R_FAILED(ret = fsFsCreateDirectory(BROWSE_STATE == STATE_SD? fs : &user_fs, path))) {
		if (config.dev_options)
			DEBUG_LOG("fsFsCreateDirectory(%s) failed: 0x%lx\n", path, ret);

		return ret;
	}

	return 0;
}

Result FS_CreateFile(const char *path, size_t size, int flags) {
	Result ret = 0;

	if (R_FAILED(ret = fsFsCreateFile(BROWSE_STATE == STATE_SD? fs : &user_fs, path, size, flags))) {
		if (config.dev_options)
			DEBUG_LOG("FS_CreateFile(%s, %d, %d) failed: 0x%lx\n", path, size, flags, ret);

		return ret;
	}

	return 0;
}

Result FS_OpenDirectory(const char *path, int flags, FsDir *dir) {
	Result ret = 0;

	if (R_FAILED(ret = fsFsOpenDirectory(BROWSE_STATE == STATE_SD? fs : &user_fs, path, flags, dir))) {
		if (config.dev_options)
			DEBUG_LOG("fsFsOpenDirectory(%s, %d) failed: 0x%lx\n", path, flags, ret);

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

Result FS_GetFileSize(const char *path, u64 *size) {
	FsFile file;
	Result ret = 0;

	if (R_FAILED(ret = fsFsOpenFile(BROWSE_STATE == STATE_SD? fs : &user_fs, path, FS_OPEN_READ, &file))) {
		fsFileClose(&file);

		if (config.dev_options)
			DEBUG_LOG("fsFsOpenFile(%s, FS_OPEN_READ, %llu) failed: 0x%lx\n", path, &size, ret);

		return ret;
	}

	if (R_FAILED(ret = fsFileGetSize(&file, size))) {
		fsFileClose(&file);

		if (config.dev_options)
			DEBUG_LOG("fsFileGetSize(%s, %llu) failed: 0x%lx\n", path, &size, ret);

		return ret;
	}

	fsFileClose(&file);
	return 0;
}

Result FS_RemoveFile(const char *path) {
	Result ret = 0;

	if (R_FAILED(ret = fsFsDeleteFile(BROWSE_STATE == STATE_SD? fs : &user_fs, path))) {
		if (config.dev_options)
			DEBUG_LOG("fsFsDeleteFile(%s) failed: 0x%lx\n", path, ret);

		return ret;
	}

	return 0;
}

Result FS_RemoveDir(const char *path) {
	Result ret = 0;

	if (R_FAILED(ret = fsFsDeleteDirectory(BROWSE_STATE == STATE_SD? fs : &user_fs, path))) {
		if (config.dev_options)
			DEBUG_LOG("fsFsDeleteDirectory(%s) failed: 0x%lx\n", path, ret);

		return ret;
	}

	return 0;
}

Result FS_RemoveDirRecursive(const char *path) {
	Result ret = 0;

	if (R_FAILED(ret = fsFsDeleteDirectoryRecursively(BROWSE_STATE == STATE_SD? fs : &user_fs, path))) {
		if (config.dev_options)
			DEBUG_LOG("fsFsDeleteDirectoryRecursively(%s) failed: 0x%lx\n", path, ret);

		return ret;
	}

	return 0;
}

Result FS_RenameFile(const char *old_filename, const char *new_filename) {
	Result ret = 0;

	if (R_FAILED(ret = fsFsRenameFile(BROWSE_STATE == STATE_SD? fs : &user_fs, old_filename, new_filename))) {
		if (config.dev_options)
			DEBUG_LOG("fsFsRenameFile(%s, %s) failed: 0x%lx\n", old_filename, new_filename, ret);

		return ret;
	}

	return 0;
}

Result FS_RenameDir(const char *old_dirname, const char *new_dirname) {
	Result ret = 0;

	if (R_FAILED(ret = fsFsRenameDirectory(BROWSE_STATE == STATE_SD? fs : &user_fs, old_dirname, new_dirname))) {
		if (config.dev_options)
			DEBUG_LOG("fsFsRenameDirectory(%s, %s) failed: 0x%lx\n", old_dirname, new_dirname, ret);

		return ret;
	}

	return 0;
}

Result FS_Read(const char *path, size_t size, void *buf) {
	FsFile file;
	Result ret = 0;

	size_t out = 0;

	if (R_FAILED(ret = fsFsOpenFile(BROWSE_STATE == STATE_SD? fs : &user_fs, path, FS_OPEN_READ, &file))) {
		fsFileClose(&file);

		if (config.dev_options)
			DEBUG_LOG("fsFsOpenFile(%s, FS_OPEN_READ, %llu) failed: 0x%lx\n", path, &size, ret);

		return ret;
	}
	
	if (R_FAILED(ret = fsFileRead(&file, 0, buf, size, &out))) {
		fsFileClose(&file);

		if (config.dev_options)
			DEBUG_LOG("fsFileRead(%s, %llu) failed: 0x%lx\n", path, &size, ret);

		return ret;
	}

	fsFileClose(&file);
	return 0;
}

Result FS_Write(const char *path, const void *buf) {
	FsFile file;
	Result ret = 0;
	
	size_t len = strlen(buf);
	u64 size = 0;

	appletLockExit();

	if (FS_FileExists(path)) {
		if (R_FAILED(ret = fsFsDeleteFile(BROWSE_STATE == STATE_SD? fs : &user_fs, path))) {
			if (config.dev_options)
				DEBUG_LOG("fsFsDeleteFile(%s) failed: 0x%lx\n", path, ret);

			return ret;
		}
	}

	if (R_FAILED(ret = fsFsCreateFile(BROWSE_STATE == STATE_SD? fs : &user_fs, path, 0, 0))) {
		if (config.dev_options)
			DEBUG_LOG("fsFsCreateFile(%s) failed: 0x%lx\n", path, ret);

		return ret;
	}

	if (R_FAILED(ret = fsFsOpenFile(BROWSE_STATE == STATE_SD? fs : &user_fs, path, FS_OPEN_WRITE, &file))) {
		fsFileClose(&file);

		if (config.dev_options)
			DEBUG_LOG("fsFsOpenFile(%s, FS_OPEN_WRITE) failed: 0x%lx\n", path, ret);

		return ret;
	}

	if (R_FAILED(ret = fsFileGetSize(&file, &size))) {
		fsFileClose(&file);

		if (config.dev_options)
			DEBUG_LOG("fsFileGetSize(%s, %llu) failed: 0x%lx\n", path, size, ret);

		return ret;
	}

	if (R_FAILED(ret = fsFileSetSize(&file, size + len))) {
		fsFileClose(&file);

		if (config.dev_options)
			DEBUG_LOG("fsFileSetSize(%s, %llu) failed: 0x%lx\n", path, size + len, ret);

		return ret;
	}

	if (R_FAILED(ret = fsFileWrite(&file, 0, buf, size + len))) {
		fsFileClose(&file);

		if (config.dev_options)
			DEBUG_LOG("fsFileWrite(%s, %llu) failed: 0x%lx\n", path, size + len, ret);

		return ret;
	}

	if (R_FAILED(ret = fsFileFlush(&file))) {
		fsFileClose(&file);

		if (config.dev_options)
			DEBUG_LOG("fsFileFlush(%s) failed: 0x%lx\n", path, ret);

		return ret;
	}

	fsFileClose(&file);
	appletUnlockExit();
	return 0;
}

Result FS_SetArchiveBit(const char *path) {
	Result ret = 0;

	if (R_FAILED(ret = fsFsSetArchiveBit(BROWSE_STATE == STATE_SD? fs : &user_fs, path))) {
		if (config.dev_options)
			DEBUG_LOG("fsFsSetArchiveBit(%s) failed: 0x%lx\n", path, ret);

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
