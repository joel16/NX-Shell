#ifndef NX_SHELL_FS_H
#define NX_SHELL_FS_H

#include <switch.h>

FsFileSystem fs;

int FS_MakeDir(const char *path);
int FS_RecursiveMakeDir(const char * dir);
bool FS_FileExists(const char *path);
bool FS_DirExists(const char *path);
const char *FS_GetFileExt(const char *filename);
char *FS_GetFileModifiedTime(const char *filename);
Result FS_GetFileSize(const char *path, u64 *size);

#endif