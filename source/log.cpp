#include <cstdarg>

#include "config.h"
#include "fs.h"

namespace Log {
    static FsFile log_file;
    static s64 offset = 0;
    
    void Init(void) {
        if (!config.dev_options)
            return;
        
        if (!FS::FileExists("/switch/NX-Shell/debug.log"))
            fsFsCreateFile(fs, "/switch/NX-Shell/debug.log", 0, 0);
            
        if (R_FAILED(fsFsOpenFile(fs, "/switch/NX-Shell/debug.log", (FsOpenMode_Read | FsOpenMode_Write | FsOpenMode_Append), &log_file)))
            return;
            
        s64 size = 0;
        if (R_FAILED(fsFileGetSize(&log_file, &size)))
            return;

        unsigned char *buffer = new unsigned char[size];

        u64 bytes_read = 0;
        if (R_FAILED(fsFileRead(&log_file, offset, buffer, size, FsReadOption_None, &bytes_read))) {
            delete[] buffer;
            return;
        }

        delete[] buffer;
        offset += bytes_read;
    }
    
    void Error(const char *data, ...) {
        if (!config.dev_options)
            return;
         
        char buf[256 + FS_MAX_PATH];
        va_list args;
        va_start(args, data);
        std::vsnprintf(buf, sizeof(buf), data, args);
        va_end(args);
        
        std::string error_string = "[ERROR] ";
        error_string.append(buf);

        std::printf("%s", error_string.c_str());
        
        if (R_FAILED(fsFileWrite(&log_file, offset, error_string.data(), error_string.length(), FsWriteOption_None)))
            return;

        offset += error_string.length();
    }
    
    void Exit(void) {
        if (!config.dev_options)
            return;
        
        fsFileClose(&log_file);
    }
}
