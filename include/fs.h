#ifndef NX_SHELL_FS_H
#define NX_SHELL_FS_H

#include <switch.h>

FsFileSystem fs;

Result FS_MakeDir(const char *path);
bool FS_FileExists(const char *path);
bool FS_DirExists(const char *path);
const char *FS_GetFileExt(const char *filename);
Result FS_GetFileSize(const char *path, u64 *size);

#endif