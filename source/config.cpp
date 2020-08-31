#include <cstdio>
#include <cstring>
#include <jansson.h>

#include "config.h"
#include "fs.h"

#define CONFIG_VERSION 0

config_t config;

namespace Config {
    static const char *config_file = "{\n\t\"config_ver\": %d,\n\t\"sort\": %d,\n\t\"dark_theme\": %d,\n\t\"last_dir\": \"%s\"\n}";
    static int config_version_holder = 0;
    static const int buf_size = 128 + FS_MAX_PATH;
    
    int Save(config_t config) {
        Result ret = 0;
        char *buf = new char[buf_size];
        u64 len = std::snprintf(buf, buf_size, config_file, CONFIG_VERSION, config.sort, config.dark_theme, config.cwd);
        
        if (R_FAILED(ret = FS::WriteFile("/switch/NX-Shell/config.json", buf, len))) {
            delete[] buf;
            return ret;
        }
        
        delete[] buf;
        return 0;
    }
    
    static void SetDefault(config_t *config) {
        config->sort = 0;
        config->dark_theme = false;
        std::strcpy(config->cwd, "/");
    }
    
    int Load(void) {
        Result ret = 0;
        
        if (!FS::DirExists("/switch/"))
            fsFsCreateDirectory(fs, "/switch");
        if (!FS::DirExists("/switch/NX-Shell/"))
            fsFsCreateDirectory(fs, "/switch/NX-Shell");
            
        if (!FS::FileExists("/switch/NX-Shell/config.json")) {
            Config::SetDefault(&config);
            return Config::Save(config);
        }
        
        FsFile file;
        if (R_FAILED(ret = fsFsOpenFile(fs, "/switch/NX-Shell/config.json", FsOpenMode_Read, &file)))
            return ret;
        
        s64 size = 0;
        if (R_FAILED(ret = fsFileGetSize(&file, &size))) {
            fsFileClose(&file);
            return ret;
        }

        char *buf =  new char[size + 1];
        if (R_FAILED(ret = fsFileRead(&file, 0, buf, static_cast<u64>(size) + 1, FsReadOption_None, nullptr))) {
            delete[] buf;
            fsFileClose(&file);
            return ret;
        }
        
        fsFileClose(&file);
        
        json_t *root;
        json_error_t error;
        root = json_loads(buf, 0, &error);
        delete[] buf;
        
        if (!root) {
            printf("error: on line %d: %s\n", error.line, error.text);
            return -1;
        }
        
        json_t *config_ver = json_object_get(root, "config_ver");
        config_version_holder = json_integer_value(config_ver);
        
        json_t *sort = json_object_get(root, "sort");
        config.sort = json_integer_value(sort);
        
        json_t *dark_theme = json_object_get(root, "dark_theme");
        config.dark_theme = json_integer_value(dark_theme);
        
        json_t *last_dir = json_object_get(root, "last_dir");
        std::strcpy(config.cwd, json_string_value(last_dir));
        
        if (!FS::DirExists(config.cwd))
            std::strcpy(config.cwd, "/");
            
        // Delete config file if config file is updated. This will rarely happen.
        if (config_version_holder  < CONFIG_VERSION) {
            fsFsDeleteFile(fs, "/switch/NX-Shell/config.json");
            Config::SetDefault(&config);
            return Config::Save(config);
        }
        
        json_decref(root);
        return 0;
    }
}
