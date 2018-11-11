#ifndef NX_SHELL_FS_H
#define NX_SHELL_FS_H

#include <switch.h>

bool FS_FileExists(const char *path);
bool FS_DirExists(const char *path);
Result FS_MakeDir(const char *path);
Result FS_CreateFile(const char *path, size_t size, int flags);
Result FS_OpenDirectory(const char *path, int flags, FsDir *dir);
Result FS_GetDirEntryCount(FsDir *dir, u64 *count);
Result FS_ReadDir(FsDir *dir, u64 inval, size_t *total_entries, size_t max_entries, FsDirectoryEntry *entry);
Result FS_GetFileSize(const char *path, u64 *size);
Result FS_RemoveFile(const char *path);
Result FS_RemoveDir(const char *path);
Result FS_RemoveDirRecursive(const char *path);
Result FS_RenameFile(const char *old_filename, const char *new_filename);
Result FS_RenameDir(const char *old_dirname, const char *new_dirname);
Result FS_Read(const char *path, size_t size, void *buf);
Result FS_Write(const char *path, const void *buf);
Result FS_SetArchiveBit(const char *path);
Result FS_OpenBisFileSystem(FsFileSystem *fs, u32 partition_ID, const char *string);
const char *FS_GetFileExt(const char *filename);
char *FS_GetFilePermission(const char *filename);

#endif
