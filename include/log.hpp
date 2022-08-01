#pragma once

namespace Log {
    void Init(void);
    void Error(const char *data, ...);
    void Exit(void);
}
