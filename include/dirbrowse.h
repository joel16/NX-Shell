#ifndef NX_SHELL_DIRBROWSE_H
#define NX_SHELL_DIRBROWSE_H

#include <switch.h>

typedef struct File {
	struct File *next; // Next item
	int isDir;          // Folder flag
	int isReadOnly;     // Read-only flag
	int isHidden;       // Hidden file flag
	char name[256];       // File name
	char ext[4];        // File extension
	u64 size;           // File size
} File;

extern File *files;

extern int initialPosition;
extern int position;
extern int fileCount;

int multi_select_index;           // Multi-select index.
bool multi_select[256];           // Array of indices selected.
int multi_select_indices[256];    // Array to hold the indices.
char multi_select_dir[FS_MAX_PATH];       // Holds the current dir where multi-select happens.
char multi_select_paths[512][FS_MAX_PATH]; // Holds the file paths of those in the clipboard.

void Dirbrowse_RecursiveFree(File *node);
Result Dirbrowse_PopulateFiles(bool clear);
void Dirbrowse_DisplayFiles(void);
File *Dirbrowse_GetFileIndex(int index);
void Dirbrowse_OpenFile(void);
int Dirbrowse_Navigate(bool parent);

#endif
