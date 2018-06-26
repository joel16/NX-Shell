#ifndef NX_SHELL_DIRBROWSE_H
#define NX_SHELL_DIRBROWSE_H

#include <switch.h>

typedef struct File
{
	struct File *next; // Next item
	int isDir;          // Folder flag
	int isReadOnly;     // Read-only flag
	int isHidden;       // Hidden file flag
	u8 name[256];       // File name
	char ext[4];        // File extension
	u64 size;           // File size
} File;

extern File *files;

extern int initialPosition;
extern int position;
extern int fileCount;

void Dirbrowse_RecursiveFree(File *node);
Result Dirbrowse_PopulateFiles(bool clear);
void Dirbrowse_DisplayFiles(void);
File *Dirbrowse_GetFileIndex(int index);
void Dirbrowse_OpenFile(void);
int Dirbrowse_Navigate(int dir);

#endif