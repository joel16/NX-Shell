#include "common.h"

jmp_buf exitJmp;

int MENU_DEFAULT_STATE;
int BROWSE_STATE;

char cwd[512];
