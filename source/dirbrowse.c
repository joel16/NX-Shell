#include <dirent.h>
#include <switch.h>

#include "archive.h"
#include "common.h"
#include "config.h"
#include "dirbrowse.h"
#include "fs.h"
#include "menu_gallery.h"
#include "SDL_helper.h"
#include "textures.h"
#include "utils.h"

#define TYPE_DIR(n) (n == DT_DIR ? 1 : 0)

int position = 0; // menu position
int fileCount = 0; // file count
File * files = NULL; // file list

void Dirbrowse_RecursiveFree(File *node)
{
	if (node == NULL) // End of list
		return;
	
	Dirbrowse_RecursiveFree(node->next); // Nest further
	free(node); // Free memory
}

/* function declaration */
typedef int(*qsort_compar)(const void *, const void *);

/* Basically scndir implementation */
static int Dirbrowse_ScanDir(const char *dir, struct dirent ***namelist, int (*select)(const struct dirent *),
	int (*compar)(const struct dirent **, const struct dirent **)) 
{
	DIR *d;
	struct dirent *entry;
	register int i = 0;
	size_t entrysize;

	if ((d=opendir(dir)) == NULL)
		return(-1);

	*namelist=NULL;
	
	while ((entry=readdir(d)) != NULL)
	{
		if (select == NULL || (select != NULL && (*select)(entry)))
		{
			*namelist = (struct dirent **)realloc((void *)(*namelist), (size_t)((i + 1) * sizeof(struct dirent *)));

			if (*namelist == NULL) 
				return(-1);
			
			entrysize = sizeof(struct dirent) - sizeof(entry->d_name) + strlen(entry->d_name) + 1;
			
			(*namelist)[i] = (struct dirent *)malloc(entrysize);
			
			if ((*namelist)[i] == NULL) 
				return(-1);

			memcpy((*namelist)[i], entry, entrysize);
			i++;
		}
	}

	if (closedir(d)) 
		return(-1);
	if (i == 0) 
		return(-1);
	if (compar != NULL)
		qsort((void *)(*namelist), (size_t)i, sizeof(struct dirent *), (qsort_compar)compar);

	return(i);
}

static int cmpstringp(const struct dirent **dent1, const struct dirent **dent2) 
{
	char isDir[2];
	
	// push '..' to beginning
	if (strcmp("..", (*dent1)->d_name) == 0)
		return -1;
	else if (strcmp("..", (*dent2)->d_name) == 0)
		return 1;

	isDir[0] = TYPE_DIR((*dent1)->d_type);
	isDir[1] = TYPE_DIR((*dent2)->d_type);

	if (isDir[0] == isDir[1]) // sort by name
		return strcasecmp((*dent1)->d_name, (*dent2)->d_name);
	else
		return isDir[1] - isDir[0]; // put directories first
}

void Dirbrowse_PopulateFiles(bool clear)
{
	char path[512];
	Dirbrowse_RecursiveFree(files);
	files = NULL;
	fileCount = 0;

	// Open Working Directory
	DIR *directory = opendir(cwd);

	if (directory)
	{
		/* Add fake ".." entry except on root */
		if (strcmp(cwd, ROOT_PATH)) 
		{
			// New List
			files = (File *)malloc(sizeof(File));
			memset(files, 0, sizeof(File));
			
			// Copy File Name
			strcpy(files->name, "..");
			files->isDir = 1;
			fileCount++;
		}

		struct dirent **entries;

		fileCount = Dirbrowse_ScanDir(cwd, &entries, NULL, cmpstringp);

		if (fileCount != 0)
		{
			for (int i = 0; i < (fileCount); i++)
			{
				// Ingore null filename
			if (entries[i]->d_name[0] == '\0') 
				continue;

			// Ignore "." in all Directories
			if (strcmp(entries[i]->d_name, ".") == 0) 
				continue;

			// Ignore ".." in Root Directory
			if (strcmp(cwd, ROOT_PATH) == 0 && strncmp(entries[i]->d_name, "..", 2) == 0) // Ignore ".." in Root Directory
				continue;

			char isDir[2];

			// Allocate Memory
			File *item = (File *)malloc(sizeof(File));

			// Clear Memory
			memset(item, 0, sizeof(File));

			// Copy File Name
			strcpy(item->name, entries[i]->d_name);

			strcpy(path, cwd);
			strcpy(path + strlen(path), item->name);
			item->size = FS_GetFileSize(path); // Copy file size

			// Set Folder Flag
			item->isDir = entries[i]->d_type == DT_DIR;

			// New List
			if (files == NULL) 
				files = item;

			// Existing List
			else
			{
				File *list = files;

				// Append to List
				while(list->next != NULL) 
					list = list->next;
	
				// Link Item
				list->next = item;
			}
			}
		}

		// Close Directory
		closedir(directory);
	}
	

	// Attempt to keep Index
	if (!clear)
	{
		if (position >= fileCount) 
			position = fileCount - 1;
	}
	else 
		position = 0;
}

void Dirbrowse_DisplayFiles(void)
{
	SDL_DrawText(Roboto, 170, 77, WHITE, cwd);

	int i = 0, printed = 0;
	File *file = files; // Draw file list

	for(; file != NULL; file = file->next)
	{
		if (printed == FILES_PER_PAGE) // Limit the files per page
			break;

		if (position < FILES_PER_PAGE || i > (position - FILES_PER_PAGE))
		{
			if (i == position)
				SDL_DrawRect(RENDERER, 0, 140 + (73 * printed), 1280, 73, dark_theme? SELECTOR_COLOUR_DARK : SELECTOR_COLOUR_LIGHT);

			SDL_DrawImage(RENDERER, dark_theme? icon_uncheck_dark : icon_uncheck, 20, 156 + (73 * printed), 40, 40);

			char path[500];
			strcpy(path, cwd);
			strcpy(path + strlen(path), file->name);

			if (file->isDir)
				SDL_DrawImage(RENDERER, dark_theme? icon_dir_dark : icon_dir, 80, 141 + (73 * printed), 72, 72);
			else if ((strncasecmp(FS_GetFileExt(file->name), "nro", 3) == 0) || (strncasecmp(FS_GetFileExt(file->name), "elf", 3) == 0)
					|| (strncasecmp(FS_GetFileExt(file->name), "bin", 3) == 0))
				SDL_DrawImage(RENDERER, icon_app, 80, 141 + (73 * printed), 72, 72);
			else if ((strncasecmp(FS_GetFileExt(file->name), "zip", 3) == 0) || (strncasecmp(FS_GetFileExt(file->name), "tar", 3) == 0)
					|| (strncasecmp(FS_GetFileExt(file->name), "lz4", 3) == 0))
				SDL_DrawImage(RENDERER, icon_archive, 80, 141 + (73 * printed), 72, 72);
			else if ((strncasecmp(FS_GetFileExt(file->name), "mp3", 3) == 0) || (strncasecmp(FS_GetFileExt(file->name), "wav", 3) == 0) 
					|| (strncasecmp(FS_GetFileExt(file->name), "ogg", 3) == 0))
				SDL_DrawImage(RENDERER, icon_audio, 80, 141 + (73 * printed), 72, 72);
			else if ((strncasecmp(FS_GetFileExt(file->name), "bmp", 3) == 0) || (strncasecmp(FS_GetFileExt(file->name), "gif", 3) == 0) 
					|| (strncasecmp(FS_GetFileExt(file->name), "jpg", 3) == 0) || (strncasecmp(FS_GetFileExt(file->name), "png", 3) == 0))
				SDL_DrawImage(RENDERER, icon_image, 80, 141 + (73 * printed), 72, 72);
			else if ((strncasecmp(FS_GetFileExt(file->name), "txt", 3) == 0) || (strncasecmp(FS_GetFileExt(file->name), "lua", 3) == 0) 
					|| (strncasecmp(FS_GetFileExt(file->name), "cfg", 3) == 0))
				SDL_DrawImage(RENDERER, icon_text, 80, 141 + (73 * printed), 72, 72);
			else
				SDL_DrawImage(RENDERER, icon_file, 80, 141 + (73 * printed), 72, 72);

			char buf[64], size[16];
			strncpy(buf, file->name, sizeof(buf));
			buf[sizeof(buf) - 1] = '\0';

			/*if (strncmp(file->name, "..", 2) == 0)
				SDL_DrawText(Roboto, 170, 160 + (73 * printed), BLACK, "Parent folder");*/
			if (!file->isDir)
			{
				Utils_GetSizeString(size, file->size);
				int width = 0;
				TTF_SizeText(Roboto_small, size, &width, NULL);
				SDL_DrawText(Roboto_small, 1260 - width, 180 + (73 * printed), dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, size);
			}

			int height = 0;
			TTF_SizeText(Roboto, buf, NULL, &height);
			SDL_DrawText(Roboto, 170, 140 + ((73 - height)/2) + (73 * printed), dark_theme? WHITE : BLACK, buf);

			printed++; // Increase printed counter
		}

		i++; // Increase counter
	}
}

static void Dirbrowse_SaveLastDirectory(void)
{
	char *buf = (char *)malloc(256);
	strcpy(buf, cwd);

	FILE *write = fopen("/switch/NX-Shell/lastdir.txt", "w");
	fprintf(write, "%s", buf);
	fclose(write);
	free(buf);
}

// Get file index
File *Dirbrowse_GetFileIndex(int index)
{
	int i = 0;
	File *file = files; // Find file Item
	
	for(; file != NULL && i != index; file = file->next)
		i++;

	return file; // Return file
}

/**
 * Executes an operation on the file depending on the filetype.
 */
void Dirbrowse_OpenFile(void)
{
	char path[512];
	File *file = Dirbrowse_GetFileIndex(position);

	if (file == NULL)
		return;

	strcpy(fileName, file->name);
	strcpy(path, cwd);
	strcpy(path + strlen(path), file->name);

	if (file->isDir)
	{
		// Attempt to navigate to target
		if (R_SUCCEEDED(Dirbrowse_Navigate(0)))
		{
			Dirbrowse_SaveLastDirectory();
			Dirbrowse_PopulateFiles(true);
		}
	}
	else if ((strncasecmp(FS_GetFileExt(file->name), "png", 3) == 0) || (strncasecmp(FS_GetFileExt(file->name), "jpg", 3) == 0))
		Gallery_DisplayImage(path);
	else if (strncasecmp(FS_GetFileExt(file->name), "zip", 3) == 0)
	{
		Archive_ExtractZip(path, cwd);
		Dirbrowse_PopulateFiles(true);
	}

	/*else if (Music_GetMusicFileType(path) != 0)
		Music_Player(path);
	else if (strncasecmp(file->ext, "txt", 3) == 0)
		TextViewer_DisplayText(path);
	else if (strncasecmp(file->ext, "zip", 3) == 0)
	{
		Archive_ExtractZip(path, cwd);
		Dirbrowse_PopulateFiles(true);
		Dirbrowse_DisplayFiles();
	}
	else if (strncasecmp(file->ext, "txt", 3) == 0)
		menu_displayText(path);*/
}

// Navigate to Folder
int Dirbrowse_Navigate(int dir)
{
	File *file = Dirbrowse_GetFileIndex(position); // Get index
	
	if ((file == NULL) || (!file->isDir)) // Not a folder
		return -1;

	// Special case ".."
	if ((dir == -1) || (strncmp(file->name, "..", 2) == 0))
	{
		char *slash = NULL;

		// Find last '/' in working directory
		int i = strlen(cwd) - 2; for(; i >= 0; i--)
		{
			// Slash discovered
			if (cwd[i] == '/')
			{
				slash = cwd + i + 1; // Save pointer
				break; // Stop search
			}
		}

		slash[0] = 0; // Terminate working directory
	}

	// Normal folder
	else
	{
		// Append folder to working directory
		strcpy(cwd + strlen(cwd), file->name);
		cwd[strlen(cwd) + 1] = 0;
		cwd[strlen(cwd)] = '/';
	}

	Dirbrowse_SaveLastDirectory();

	return 0; // Return success
}