#ifndef NX_SHELL_UTILS_H
#define NX_SHELL_UTILS_H

char *Utils_Basename(const char *filename);
void Utils_GetSizeString(char *string, u64 size);
void Utils_SetMax(int set, int value, int max);
void Utils_SetMin(int set, int value, int min);

#endif