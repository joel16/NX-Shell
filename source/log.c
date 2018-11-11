#include <stdio.h>
#include <stdarg.h>

void log_func(const char *s, ...) {
	char buf[256];
	va_list argptr;
	va_start(argptr, s);
	vsnprintf(buf, sizeof(buf), s, argptr);
	va_end(argptr);

	FILE *fp;
	fp = fopen("/switch/NX-Shell/debug.log", "a");
	fprintf(fp, buf);
	fclose(fp);
}
