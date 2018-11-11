#ifndef NX_SHELL_LOG_H
#define NX_SHELL_LOG_H

#define DEBUG_LOG(...) log_func(__VA_ARGS__)

void log_func(const char *s, ...);

#endif
