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
	char buf[100] = {}; // 100 so that it's big enough. fill with 0
	// or you could use malloc() to allocate sufficient space
	
	strncpy(buf, subject, pos); // copy at most first pos characters
	int len = strlen(buf);
	strcpy(buf+len, insert); // copy all of insert[] at the end
	len += strlen(insert);  // increase the length by length of insert[]
	strcpy(buf+len, subject+pos); // copy the rest
	
	strcpy(subject, buf);   // copy it back to subject
	// deallocate buf[] here, if used malloc()
}
