#ifndef NX_SHELL_LOG_H
#define NX_SHELL_LOG_H

namespace Log {
    void Init(void);
    void Error(const char *data, ...);
    void Exit(void);
}

#endif
