#include <cstdio>

#include "usb.hpp"
#include "usbhsfs.h"
#include "windows.hpp"

namespace USB {
    static UEvent *status_change_event = nullptr, exit_event = {0};
    static u32 usb_device_count = 0;
    static UsbHsFsDevice *usb_devices = nullptr;
    static Thread thread = {0};
    static u32 listed_device_count = 0;
    static bool thread_created = false;

    // This function is heavily based off the example provided by DarkMatterCore
    // https://github.com/DarkMatterCore/libusbhsfs/blob/main/example/source/main.c
    static void usbMscThreadFunc(void *arg) {
        (void)arg;
        
        Result ret = 0;
        int idx = 0;
        
        /* Generate waiters for our user events. */
        Waiter status_change_event_waiter = waiterForUEvent(status_change_event);
        Waiter exit_event_waiter = waiterForUEvent(&exit_event);
        
        while(true) {
            /* Wait until an event is triggered. */
            if (R_FAILED(ret = waitMulti(&idx, -1, status_change_event_waiter, exit_event_waiter)))
                continue;
            
            /* Exit event triggered. */
            if (idx == 1)
                break;

            /* Free USB Mass Storage device data. */
            USB::Unmount();

            {
                std::scoped_lock lock(devices_list_mutex);

                /* Get mounted device count. */
                usb_device_count = usbHsFsGetMountedDeviceCount();
                if (!usb_device_count)
                    continue;

                /* Allocate mounted devices buffer. */
                usb_devices = new UsbHsFsDevice[usb_device_count];
                if (!usb_devices)
                    continue;

                /* List mounted devices. */
                if (!(listed_device_count = usbHsFsListMountedDevices(usb_devices, usb_device_count))) {
                    delete[] usb_devices;
                    usb_devices = nullptr;
                    continue;
                }

                /* Print info from mounted devices. */
                for(u32 i = 0; i < listed_device_count; i++) {
                    UsbHsFsDevice *device = std::addressof(usb_devices[i]);
                    devices_list.push_back(device->name);
                }
            }
        }
        
        /* Exit thread. */
        return;
    }

    Result Init(void) {
        std::scoped_lock lock(devices_list_mutex);

        Result ret = usbHsFsInitialize(0);
        if (R_SUCCEEDED(ret)) {
            /* Get USB Mass Storage status change event. */
            status_change_event = usbHsFsGetStatusChangeUserEvent();

            /* Create usermode thread exit event. */
            ueventCreate(&exit_event, true);

            /* Create thread. */
            if (R_SUCCEEDED(ret = threadCreate(&thread, usbMscThreadFunc, nullptr, nullptr, 0x10000, 0x2C, -2))) {
                if (R_SUCCEEDED(ret = threadStart(&thread)))
                    thread_created = true;
            }
        }

        return ret;
    }

    void Exit(void) {
        std::scoped_lock lock(devices_list_mutex);

        if (thread_created) {
            /* Signal background thread. */
            ueventSignal(&exit_event);

            /* Wait for the background thread to exit on its own. */
            threadWaitForExit(&thread);
            threadClose(&thread);

            thread_created = false;
        }

        /* Clean up and exit. */
        USB::Unmount();
        usbHsFsExit();
    }
    
    bool Connected(void) {
        std::scoped_lock lock(devices_list_mutex);
        return (listed_device_count > 0);
    }

    void Unmount(void) {
        std::scoped_lock lock(devices_list_mutex);

        /* Unmount devices. */
        if (usb_devices) {
            for(u32 i = 0; i < listed_device_count; i++) {
                UsbHsFsDevice *device = std::addressof(usb_devices[i]);
                devices_list.pop_back();
                usbHsFsUnmountDevice(device, false);
            }

            delete[] usb_devices;
            usb_devices = nullptr;
        }

        listed_device_count = 0;
    }
}
