#include "common.h"

jmp_buf exitJmp;

int MENU_DEFAULT_STATE;
int BROWSE_STATE;

char cwd[512];

FsFileSystem *fs;
FsFileSystem user_fs;
u64 total_storage = 0, used_storage = 0;
