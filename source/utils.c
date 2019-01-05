#include <stdio.h>
#include <string.h>
#include <switch.h>

#include "utils.h"

char *Utils_Basename(const char *filename) {
	char *p = strrchr (filename, '/');
	return p ? p + 1 : (char *) filename;
}

void Utils_GetSizeString(char *string, u64 size) {
	double double_size = (double)size;

	int i = 0;
	static char *units[] = {"B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};

	while (double_size >= 1024.0f) {
		double_size /= 1024.0f;
		i++;
	}

	sprintf(string, "%.*f %s", (i == 0) ? 0 : 2, double_size, units[i]);
}

void Utils_SetMax(int *set, int value, int max) {
	if (*set > max)
		*set = value;
}

void Utils_SetMin(int *set, int value, int min) {
	if (*set < min)
		*set = value;
}

int Utils_Alphasort(const void *p1, const void *p2) {
	FsDirectoryEntry* entryA = (FsDirectoryEntry*) p1;
	FsDirectoryEntry* entryB = (FsDirectoryEntry*) p2;
	
	if ((entryA->type == ENTRYTYPE_DIR) && !(entryB->type == ENTRYTYPE_DIR))
		return -1;
	else if (!(entryA->type == ENTRYTYPE_DIR) && (entryB->type == ENTRYTYPE_DIR))
		return 1;
		
	return strcasecmp(entryA->name, entryB->name);
}

void Utils_AppendArr(char subject[], const char insert[], int pos) {
	char buf[512] = {};

	strncpy(buf, subject, pos);
	int len = strlen(buf);
	strcpy(buf + len, insert);
	len += strlen(insert);
	strcpy(buf+len, subject + pos);
	
	strcpy(subject, buf);
}

u64 Utils_GetTotalStorage(FsFileSystem *fs) {
	Result ret = 0;
	u64 total = 0;

	if (R_FAILED(ret = fsFsGetTotalSpace(fs, "/", &total)))
		printf("fsFsGetTotalSpace() failed: 0x%x.\n\n", ret);

	return total;
}

static u64 Utils_GetFreeStorage(FsFileSystem *fs) {
	Result ret = 0;
	u64 free = 0;

	if (R_FAILED(ret = fsFsGetFreeSpace(fs, "/", &free)))
		printf("fsFsGetFreeSpace() failed: 0x%x.\n\n", ret);

	return free;
}

u64 Utils_GetUsedStorage(FsFileSystem *fs) {
	return (Utils_GetTotalStorage(fs) - Utils_GetFreeStorage(fs));
}
