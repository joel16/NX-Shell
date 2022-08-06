#include <stdio.h>
#include <switch.h>

#include "config.hpp"
#include "fs.hpp"
#include "gui.hpp"
#include "imgui.h"
#include "log.hpp"
#include "textures.hpp"
#include "windows.hpp"
#include "usb.hpp"

char __application_path[FS_MAX_PATH];

namespace Services {
    int Init(void) {
        Result ret = 0;
        
        devices[FileSystemSDMC] = *fsdevGetDeviceFileSystem("sdmc");
        fs = std::addressof(devices[FileSystemSDMC]);

        fsOpenBisFileSystem(std::addressof(devices[FileSystemSafe]), FsBisPartitionId_SafeMode, "");
		fsdevMountDevice("safe", devices[FileSystemSafe]);

        fsOpenBisFileSystem(std::addressof(devices[FileSystemUser]), FsBisPartitionId_User, "");
		fsdevMountDevice("user", devices[FileSystemUser]);

        fsOpenBisFileSystem(std::addressof(devices[FileSystemSystem]), FsBisPartitionId_System, "");
		fsdevMountDevice("system", devices[FileSystemSystem]);
        
        Config::Load();
        Log::Init();
        socketInitializeDefault();
        nxlinkStdio();

        if (R_FAILED(ret = romfsInit())) {
            Log::Error("romfsInit() failed: 0x%x\n", ret);
            return ret;
        }
        
        if (R_FAILED(ret = nifmInitialize(NifmServiceType_User))) {
            Log::Error("nifmInitialize(NifmServiceType_User) failed: 0x%x\n", ret);
            return ret;
        }
        
        if (R_FAILED(ret = plInitialize(PlServiceType_User))) {
            Log::Error("plInitialize(PlServiceType_User) failed: 0x%x\n", ret);
            return ret;
        }

        if (R_FAILED(ret = USB::Init())) {
            Log::Error("usbHsFsInitialize(0) failed: 0x%x\n", ret);
            return ret;
        }

        if (!GUI::Init())
            Log::Error("GUI::Init() failed: 0x%x\n", ret);
        
        Textures::Init();
        plExit();
        romfsExit();
        return 0;
    }
    
    void Exit(void) {
        Textures::Exit();
        GUI::Exit();
        USB::Exit();
        nifmExit();
        socketExit();
        Log::Exit();
        
        fsdevUnmountDevice("system");
        fsdevUnmountDevice("user");
        fsdevUnmountDevice("safe");
    }
}

int main(int argc, char* argv[]) {
    u64 key = 0;

    Services::Init();

    if (!FS::GetDirList(device, cwd, data.entries)) {
        Services::Exit();
        return 0;
    }
    
    data.checkbox_data.checked.resize(data.entries.size());
    FS::GetUsedStorageSpace(data.used_storage);
    FS::GetTotalStorageSpace(data.total_storage);
    
    while (GUI::Loop(key)) {
        Windows::MainWindow(data, key, false);
        GUI::Render();
    }

    data.entries.clear();
    Services::Exit();
    return 0;
}
