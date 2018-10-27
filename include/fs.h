#ifndef NX_SHELL_FS_H
#define NX_SHELL_FS_H

#include <switch.h>

extern FsFileSystem fs;

Result FS_MakeDir(const char *path);
bool FS_FileExists(const char *path);
bool FS_DirExists(const char *path);
const char *FS_GetFileExt(const char *filename);
Result FS_GetFileSize(const char *path, u64 *size);
char *FS_GetFilePermission(const char *filename);
Result FS_Read(const char *path, size_t size, void *buf);
Result FS_Write(const char *path, const void *buf);

#endif
