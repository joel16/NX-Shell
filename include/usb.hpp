#pragma once

#include <switch.h>
#include <vector>

namespace USB {
    Result Init(void);
    void Exit(void);
    void Unmount(void);
    bool Connected(void);
}
