#include <dirent.h>
#include <sys/stat.h>

#include "archive.h"
#include "common.h"
#include "config.h"
#include "dirbrowse.h"
#include "fs.h"
#include "menu_gallery.h"
#include "menu_music.h"
#include "menu_book_reader.h"
#include "SDL_helper.h"
#include "textures.h"
#include "utils.h"

#define TYPE_DIR(n) (n == DT_DIR ? 1 : 0)

int initialPosition = 0;
int position = 0; // menu position
int fileCount = 0; // file count
File *files = NULL; // file list

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
	if (compar != NULL)
		qsort((void *)(*namelist), (size_t)i, sizeof(struct dirent *), (qsort_compar)compar);

	return(i);
}

static int cmpstringp(const struct dirent **dent1, const struct dirent **dent2) 
{
	int ret = 0;
	char isDir[2], path1[512], path2[512];
	struct stat sbuf1, sbuf2;
	
	// push '..' to beginning
	if (strcmp("..", (*dent1)->d_name) == 0)
		return -1;
	else if (strcmp("..", (*dent2)->d_name) == 0)
		return 1;

	isDir[0] = TYPE_DIR((*dent1)->d_type);
	isDir[1] = TYPE_DIR((*dent2)->d_type);

	strcpy(path1, cwd);
	strcpy(path1 + strlen(path1), (*dent1)->d_name);
	ret = stat(path1, &sbuf1);
	if (ret)
		return 0;

	strcpy(path2, cwd);
	strcpy(path2 + strlen(path2), (*dent2)->d_name);
	ret = stat(path2, &sbuf2);
	if (ret)
		return 0;

	u64 sizeA = FS_GetFileSize(path1);
	u64 sizeB = FS_GetFileSize(path2);

	switch(config_sort_by)
	{
		case 0:
			if (isDir[0] == isDir[1]) // Alphabetical ascending
				return strcasecmp((*dent1)->d_name, (*dent2)->d_name);
			else
				return isDir[1] - isDir[0];
			break;

		case 1:
			if (isDir[0] == isDir[1]) // Alphabetical descending
				return strcasecmp((*dent2)->d_name, (*dent1)->d_name);
			else
				return isDir[1] - isDir[0];
			break;
		
		case 2:
			if (isDir[0] == isDir[1]) // Size newest
   	    		return sizeA > sizeB ? -1 : sizeA < sizeB ? 1 : 0;
   	    	else
				return isDir[1] - isDir[0];
			break;
		
		case 3:
			if (isDir[0] == isDir[1]) // Size oldest
   	    		return sizeB > sizeA ? -1 : sizeB < sizeA ? 1 : 0;
   	    	else
				return isDir[1] - isDir[0]; // put directories first
			break;
	}
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

		if (fileCount >= 0)
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

				// Set Folder Flag
				item->isDir = entries[i]->d_type == DT_DIR;

				// Copy file size
				if (!item->isDir)
				{
					strcpy(path, cwd);
					strcpy(path + strlen(path), item->name);
					item->size = FS_GetFileSize(path);
				}

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

				free(entries[i]);
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
	int title_height = 0;
	SDL_DrawImage(RENDERER, icon_nav_drawer, 20, 58);
	SDL_DrawImage(RENDERER, icon_actions, (1260 - 64), 58);
	TTF_SizeText(Roboto_large, cwd, NULL, &title_height);
	SDL_DrawText(RENDERER, Roboto_large, 170, 40 + ((100 - title_height)/2), WHITE, cwd);

	int i = 0, printed = 0;
	File *file = files; // Draw file list

	for(; file != NULL; file = file->next)
	{
		if (printed == FILES_PER_PAGE) // Limit the files per page
			break;

		if (position < FILES_PER_PAGE || i > (position - FILES_PER_PAGE))
		{
			if (i == position)
				SDL_DrawRect(RENDERER, 0, 140 + (73 * printed), 1280, 73, config_dark_theme? SELECTOR_COLOUR_DARK : SELECTOR_COLOUR_LIGHT);

			if (strcmp(multi_select_dir, cwd) == 0)
			{
				multi_select[i] == true? SDL_DrawImage(RENDERER, config_dark_theme? icon_check_dark : icon_check, 20, 156 + (73 * printed)) : 
					SDL_DrawImage(RENDERER, config_dark_theme? icon_uncheck_dark : icon_uncheck, 20, 156 + (73 * printed));
			}
			else
				SDL_DrawImage(RENDERER, config_dark_theme? icon_uncheck_dark : icon_uncheck, 20, 156 + (73 * printed));

			char path[512];
			strcpy(path, cwd);
			strcpy(path + strlen(path), file->name);

			if (file->isDir)
				SDL_DrawImageScale(RENDERER, config_dark_theme? icon_dir_dark : icon_dir, 80, 141 + (73 * printed), 72, 72);
			else if ((strncasecmp(FS_GetFileExt(file->name), "nro", 3) == 0) || (strncasecmp(FS_GetFileExt(file->name), "elf", 3) == 0)
					|| (strncasecmp(FS_GetFileExt(file->name), "bin", 3) == 0))
				SDL_DrawImageScale(RENDERER, icon_app, 80, 141 + (73 * printed), 72, 72);
			else if ((strncasecmp(FS_GetFileExt(file->name), "zip", 3) == 0) || (strncasecmp(FS_GetFileExt(file->name), "tar", 3) == 0)
					|| (strncasecmp(FS_GetFileExt(file->name), "lz4", 3) == 0))
				SDL_DrawImageScale(RENDERER, icon_archive, 80, 141 + (73 * printed), 72, 72);
			else if ((strncasecmp(FS_GetFileExt(file->name), "mp3", 3) == 0) || (strncasecmp(FS_GetFileExt(file->name), "ogg", 3) == 0)
					|| (strncasecmp(FS_GetFileExt(file->name), "wav", 3) == 0) || (strncasecmp(FS_GetFileExt(file->name), "mod", 3) == 0)
					|| (strncasecmp(FS_GetFileExt(file->name), "flac", 4) == 0) || (strncasecmp(FS_GetFileExt(file->name), "midi", 4) == 0)
					|| (strncasecmp(FS_GetFileExt(file->name), "mid", 3) == 0))
				SDL_DrawImageScale(RENDERER, icon_audio, 80, 141 + (73 * printed), 72, 72);
			else if ((strncasecmp(FS_GetFileExt(file->name), "png", 3) == 0) || (strncasecmp(FS_GetFileExt(file->name), "jpg", 3) == 0) 
					|| (strncasecmp(FS_GetFileExt(file->name), "bmp", 3) == 0) || (strncasecmp(FS_GetFileExt(file->name), "gif", 3) == 0))
				SDL_DrawImageScale(RENDERER, icon_image, 80, 141 + (73 * printed), 72, 72);
			else if ((strncasecmp(FS_GetFileExt(file->name), "txt", 3) == 0) || (strncasecmp(FS_GetFileExt(file->name), "lua", 3) == 0) 
            		|| (strncasecmp(FS_GetFileExt(file->name), "cfg", 3) == 0))
				SDL_DrawImageScale(RENDERER, icon_text, 80, 141 + (73 * printed), 72, 72);
			else if ((strncasecmp(FS_GetFileExt(file->name), "pdf", 3) == 0) || (strncasecmp(FS_GetFileExt(file->name), "cbz", 3) == 0)
					|| (strncasecmp(FS_GetFileExt(file->name), "fb2", 3) == 0) || (strncasecmp(FS_GetFileExt(file->name), "epub", 4) == 0))
				SDL_DrawImageScale(RENDERER, icon_doc, 80, 141 + (73 * printed), 72, 72);
			else
				SDL_DrawImageScale(RENDERER, icon_file, 80, 141 + (73 * printed), 72, 72);

			char buf[64], size[16];
			strncpy(buf, file->name, sizeof(buf));
			buf[sizeof(buf) - 1] = '\0';

			if (!file->isDir)
			{
				Utils_GetSizeString(size, file->size);
				int width = 0;
				TTF_SizeText(Roboto_small, size, &width, NULL);
				SDL_DrawText(RENDERER, Roboto_small, 1260 - width, 180 + (73 * printed), config_dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, size);
			}

			int height = 0;
			TTF_SizeText(Roboto, buf, NULL, &height);
			
			if (strncmp(file->name, "..", 2) == 0)
				SDL_DrawText(RENDERER, Roboto, 170, 140 + ((73 - height)/2) + (73 * printed), config_dark_theme? WHITE : BLACK, "Parent folder");
			else 
				SDL_DrawText(RENDERER, Roboto, 170, 140 + ((73 - height)/2) + (73 * printed), config_dark_theme? WHITE : BLACK, buf);

			printed++; // Increase printed counter
		}

		i++; // Increase counter
	}
}

static void Dirbrowse_SaveLastDirectory(void)
{
	char *buf = (char *)malloc(512);
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
	
	strcpy(path, cwd);
	strcpy(path + strlen(path), file->name);

	if (file->isDir)
	{
		// Attempt to navigate to target
		if (R_SUCCEEDED(Dirbrowse_Navigate(false)))
		{
			Dirbrowse_SaveLastDirectory();
			Dirbrowse_PopulateFiles(true);
		}
	}
	else if ((strncasecmp(FS_GetFileExt(file->name), "png", 3) == 0) || (strncasecmp(FS_GetFileExt(file->name), "jpg", 3) == 0)
			|| (strncasecmp(FS_GetFileExt(file->name), "bmp", 3) == 0) || (strncasecmp(FS_GetFileExt(file->name), "gif", 3) == 0))
		Gallery_DisplayImage(path);
	else if (strncasecmp(FS_GetFileExt(file->name), "zip", 3) == 0)
	{
		Archive_ExtractZip(path, cwd);
		Dirbrowse_PopulateFiles(true);
	}
	else if ((strncasecmp(FS_GetFileExt(file->name), "mp3", 3) == 0) || (strncasecmp(FS_GetFileExt(file->name), "ogg", 3) == 0)
			|| (strncasecmp(FS_GetFileExt(file->name), "wav", 3) == 0) || (strncasecmp(FS_GetFileExt(file->name), "mod", 3) == 0)
			|| (strncasecmp(FS_GetFileExt(file->name), "flac", 4) == 0) || (strncasecmp(FS_GetFileExt(file->name), "midi", 4) == 0)
			|| (strncasecmp(FS_GetFileExt(file->name), "mid", 3) == 0))
		Menu_PlayMusic(path);
	else if ((strncasecmp(FS_GetFileExt(file->name), "pdf", 3) == 0) || (strncasecmp(FS_GetFileExt(file->name), "cbz", 3) == 0)
			|| (strncasecmp(FS_GetFileExt(file->name), "fb2", 3) == 0) || (strncasecmp(FS_GetFileExt(file->name), "epub", 4) == 0))
		Menu_OpenBook(path);

	/*else if (strncasecmp(file->ext, "txt", 3) == 0)
		TextViewer_DisplayText(path);*/
}

// Navigate to Folder
int Dirbrowse_Navigate(bool parent)
{
	File *file = Dirbrowse_GetFileIndex(position); // Get index
	
	if (file == NULL)
		return -1;

	// Special case ".."
	if ((parent) || (strncmp(file->name, "..", 2) == 0))
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
		if (file->isDir)
		{
			// Append folder to working directory
			strcpy(cwd + strlen(cwd), file->name);
			cwd[strlen(cwd) + 1] = 0;
			cwd[strlen(cwd)] = '/';
		}
	}

	Dirbrowse_SaveLastDirectory();

	return 0; // Return success
}