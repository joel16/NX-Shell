#ifndef NX_SHELL_UTILS_H
#define NX_SHELL_UTILS_H

char *Utils_Basename(const char *filename);
void Utils_GetSizeString(char *string, u64 size);
void Utils_SetMax(int *set, int value, int max);
void Utils_SetMin(int *set, int value, int min);
int Utils_Alphasort(const void *p1, const void *p2);
void Utils_AppendArr(char subject[], const char insert[], int pos);
u64 Utils_GetTotalStorage(FsFileSystem *fs);
u64 Utils_GetUsedStorage(FsFileSystem *fs);

#endif
