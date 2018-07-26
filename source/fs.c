#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <switch.h>

#include "fs.h"

int FS_MakeDir(const char *path)
{
	if (!path) 
		return -1;

	return mkdir(path, 0777);
}

int FS_RecursiveMakeDir(const char * dir)
{
	int ret = 0;
	char buf[256];
	char *p = NULL;
	size_t len;

	snprintf(buf, sizeof(buf), "%s",dir);
	len = strlen(buf);

	if (buf[len - 1] == '/')
		buf[len - 1] = 0;

	for (p = buf + 1; *p; p++)
	{
		if (*p == '/') 
		{
			*p = 0;

			ret = FS_MakeDir(buf);
			
			*p = '/';
		}
		
		ret = FS_MakeDir(buf);
	}
	
	return ret;
}

bool FS_FileExists(const char *path)
{
	FILE *temp = fopen(path, "r");
	
	if (temp == NULL)
		return false;
	
	fclose(temp);
	return true;
}

bool FS_DirExists(const char *path)
{
	struct stat info;

	if (stat(path, &info) != 0)
		return false;
	else if (info.st_mode & S_IFDIR)
		return true;
	else
		return false;
}

const char *FS_GetFileExt(const char *filename) 
{
	const char *dot = strrchr(filename, '.');
	
	if (!dot || dot == filename)
		return "";
	
	return dot + 1;
}

char *FS_GetFileModifiedTime(const char *filename) 
{
	struct stat attr;
	stat(filename, &attr);
	
	return ctime(&attr.st_mtime);
}

u64 FS_GetFileSize(const char *filename)
{
	struct stat st;
	stat(filename, &st);
	
	return st.st_size;
}