/*
 * usbhsfs.h
 *
 * Copyright (c) 2020-2022, DarkMatterCore <pabloacurielz@gmail.com>.
 * Copyright (c) 2020-2021, XorTroll.
 * Copyright (c) 2020-2021, Rhys Koedijk.
 *
 * This file is part of libusbhsfs (https://github.com/DarkMatterCore/libusbhsfs).
 */

#pragma once

#ifndef __USBHSFS_H__
#define __USBHSFS_H__

#include <switch.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Library version.
#define LIBUSBHSFS_VERSION_MAJOR    0
#define LIBUSBHSFS_VERSION_MINOR    2
#define LIBUSBHSFS_VERSION_MICRO    7

/// Helper macro to generate a string based on a filesystem type value.
#define LIBUSBHSFS_FS_TYPE_STR(x)   ((x) == UsbHsFsDeviceFileSystemType_FAT12 ? "FAT12" : ((x) == UsbHsFsDeviceFileSystemType_FAT16 ? "FAT16" : ((x) == UsbHsFsDeviceFileSystemType_FAT32 ? "FAT32" : \
                                    ((x) == UsbHsFsDeviceFileSystemType_exFAT ? "exFAT" : ((x) == UsbHsFsDeviceFileSystemType_NTFS  ? "NTFS"  : ((x) == UsbHsFsDeviceFileSystemType_EXT2  ? "EXT2"  : \
                                    ((x) == UsbHsFsDeviceFileSystemType_EXT3  ? "EXT3"  : ((x) == UsbHsFsDeviceFileSystemType_EXT4  ? "EXT4"  : "Invalid"))))))))

/// Used to identify the filesystem type from a mounted filesystem (e.g. filesize limitations, etc.).
typedef enum {
    UsbHsFsDeviceFileSystemType_Invalid = 0,
    UsbHsFsDeviceFileSystemType_FAT12   = 1,
    UsbHsFsDeviceFileSystemType_FAT16   = 2,
    UsbHsFsDeviceFileSystemType_FAT32   = 3,
    UsbHsFsDeviceFileSystemType_exFAT   = 4,
    UsbHsFsDeviceFileSystemType_NTFS    = 5,    ///< Only returned by the GPL build of the library.
    UsbHsFsDeviceFileSystemType_EXT2    = 6,    ///< Only returned by the GPL build of the library.
    UsbHsFsDeviceFileSystemType_EXT3    = 7,    ///< Only returned by the GPL build of the library.
    UsbHsFsDeviceFileSystemType_EXT4    = 8     ///< Only returned by the GPL build of the library.
} UsbHsFsDeviceFileSystemType;

/// Filesystem mount flags.
/// Not all supported filesystems are compatible with these flags.
/// The default mount bitmask is `UsbHsFsMountFlags_UpdateAccessTimes | UsbHsFsMountFlags_ShowHiddenFiles | UsbHsFsMountFlags_ReplayJournal`.
/// It can be overriden via usbHsFsSetFileSystemMountFlags() (see below).
typedef enum {
    UsbHsFsMountFlags_None                        = 0x00000000, ///< No special action is taken.
    UsbHsFsMountFlags_IgnoreCaseSensitivity       = 0x00000001, ///< NTFS only. Case sensitivity is ignored for all filesystem operations.
    UsbHsFsMountFlags_UpdateAccessTimes           = 0x00000002, ///< NTFS only. File/directory access times are updated after each successful R/W operation.
    UsbHsFsMountFlags_ShowHiddenFiles             = 0x00000004, ///< NTFS only. Hidden file entries are returned while enumerating directories.
    UsbHsFsMountFlags_ShowSystemFiles             = 0x00000008, ///< NTFS only. System file entries are returned while enumerating directories.
    UsbHsFsMountFlags_IgnoreFileReadOnlyAttribute = 0x00000010, ///< NTFS only. Allows writing to files even if they are marked as read-only.
    UsbHsFsMountFlags_ReadOnly                    = 0x00000100, ///< NTFS and EXT only. Filesystem is mounted as read-only.
    UsbHsFsMountFlags_ReplayJournal               = 0x00000200, ///< NTFS and EXT only. Replays the log/journal to restore filesystem consistency (e.g. fix unsafe device ejections).
    UsbHsFsMountFlags_IgnoreHibernation           = 0x00010000, ///< NTFS only. Filesystem is mounted even if it's in a hibernated state. The saved Windows session is completely lost.

    ///< Pre-generated bitmasks provided for convenience.
    UsbHsFsMountFlags_SuperUser                   = (UsbHsFsMountFlags_ShowHiddenFiles | UsbHsFsMountFlags_ShowSystemFiles | UsbHsFsMountFlags_IgnoreFileReadOnlyAttribute),
    UsbHsFsMountFlags_Force                       = (UsbHsFsMountFlags_ReplayJournal | UsbHsFsMountFlags_IgnoreHibernation)
} UsbHsFsMountFlags;

/// Struct used to list mounted filesystems as devoptab devices.
/// Everything but the manufacturer, product_name and name fields is empty/zeroed-out under SX OS.
typedef struct {
    s32 usb_if_id;          ///< USB interface ID. Internal use.
    u8 lun;                 ///< Logical unit. Internal use.
    u32 fs_idx;             ///< Filesystem index. Internal use.
    bool write_protect;     ///< Set to true if the logical unit is protected against write operations.
    u16 vid;                ///< Vendor ID. Retrieved from the device descriptor. Useful if you wish to implement a filter in your application.
    u16 pid;                ///< Product ID. Retrieved from the device descriptor. Useful if you wish to implement a filter in your application.
    char manufacturer[64];  ///< UTF-8 encoded manufacturer string. Retrieved from the device descriptor or SCSI Inquiry data. May be empty.
    char product_name[64];  ///< UTF-8 encoded product name string. Retrieved from the device descriptor or SCSI Inquiry data. May be empty.
    char serial_number[64]; ///< UTF-8 encoded serial number string. Retrieved from the device descriptor. May be empty.
    u64 capacity;           ///< Raw capacity from the logical unit this filesystem belongs to. Use statvfs() to get the actual filesystem capacity. May be shared with other UsbHsFsDevice entries.
    char name[32];          ///< Mount name used by the devoptab virtual device interface (e.g. "ums0:"). Use it as a prefix in libcstd I/O calls to perform operations on this filesystem.
    u8 fs_type;             ///< UsbHsFsDeviceFileSystemType.
    u32 flags;              ///< UsbHsFsMountFlags bitmask used at mount time.
} UsbHsFsDevice;

/// Initializes the USB Mass Storage Host interface.
/// event_idx represents the event index to use with usbHsCreateInterfaceAvailableEvent() / usbHsDestroyInterfaceAvailableEvent(). Must be within the 0 - 2 range (inclusive).
/// If you're not using any usb:hs interface available events on your own, set this value to 0. If running under SX OS, this value will be ignored.
/// This function will fail if the deprecated fsp-usb service is running in the background.
Result usbHsFsInitialize(u8 event_idx);

/// Closes the USB Mass Storage Host interface.
/// If there are any UMS devices with mounted filesystems connected to the console when this function is called, their filesystems will be unmounted and their logical units will be stopped.
void usbHsFsExit(void);

/// Returns a pointer to the user-mode status change event (with autoclear enabled).
/// Useful to wait for USB Mass Storage status changes without having to constantly poll the interface.
/// Returns NULL if the USB Mass Storage Host interface hasn't been initialized.
UEvent *usbHsFsGetStatusChangeUserEvent(void);

/// Returns the mounted device count.
u32 usbHsFsGetMountedDeviceCount(void);

/// Lists up to max_count mounted devices and stores their information in the provided UsbHsFsDevice array.
/// Returns the total number of written entries.
u32 usbHsFsListMountedDevices(UsbHsFsDevice *out, u32 max_count);

/// Unmounts all filesystems from the UMS device with a USB interface ID that matches the one from the provided UsbHsFsDevice, and stops all of its logical units.
/// Can be used to safely unmount a UMS device at runtime, if that's needed for some reason. Calling this function before usbHsFsExit() isn't necessary.
/// If multiple UsbHsFsDevice entries are returned for the same UMS device, any of them can be used as the input argument for this function.
/// If successful, and signal_status_event is true, this will also fire the user-mode status change event from usbHsFsGetStatusChangeUserEvent().
/// This function has no effect at all under SX OS.
bool usbHsFsUnmountDevice(UsbHsFsDevice *device, bool signal_status_event);

/// Returns a bitmask with the current filesystem mount flags.
/// Can be used even if the USB Mass Storage Host interface hasn't been initialized.
/// This function has no effect at all under SX OS.
u32 usbHsFsGetFileSystemMountFlags(void);

/// Takes an input bitmask with the desired filesystem mount flags, which will be used for all mount operations.
/// Can be used even if the USB Mass Storage Host interface hasn't been initialized.
/// This function has no effect at all under SX OS.
void usbHsFsSetFileSystemMountFlags(u32 flags);

#ifdef __cplusplus
}
#endif

#endif  /* __USBHSFS_H__ */
