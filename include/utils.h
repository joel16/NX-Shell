#ifndef NX_SHELL_UTILS_H
#define NX_SHELL_UTILS_H

#include <switch.h>

extern char __application_path[FS_MAX_PATH];

namespace Utils {
    void GetSizeString(char *string, double size);
}

#endif
