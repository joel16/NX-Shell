/* dmc_unrar - A dependency-free, single-file FLOSS unrar library
 *
 * Copyright (c) 2017 by Sven Hesse (DrMcCoy) <drmccoy@drmccoy.de>
 *
 * dmc_unrar is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 2 of
 * the License, or (at your option) any later version.
 *
 * dmc_unrar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For the full text of the GNU General Public License version 2,
 * see <https://www.gnu.org/licenses/gpl-2.0.html>
 *
 * This code is based heavily on other FLOSS code. Please see the
 * bottom of this file for attribution and license information.
 */

/* This is an unpacker and decompressor of the popular RAR format by
 * Eugene Roshal and Alexander Roshal.
 *
 * Features we support:
 * - Unpacking RAR versions 1.5
 * - Unpacking RAR versions 2.0/2.6 (WinRAR 2.0)
 *   - Including normal compression and audio/media compression
 * - Unpacking RAR versions 2.9/3.6 (aka RAR4, WinRAR 3.0)
 *   - Including PPMd, delta, audio/media, RGB, x86
 * - Unpacking RAR versions 5.0 (aka RAR5, WinRAR 5.0)
 *   - Including delta, x86, ARM
 * - Solid archives (1.5, 2.0/2.6, 2.9/3.6, 5.0)
 * - SFX archives
 * - Validating extraction result against archive CRC-32
 * - Archive and file comments
 *
 * Features we don't support (in rough order from easiest to difficult)
 * - Unix owner/group info, NTFS permissions
 * - Symbolic links and hard links
 * - Large files (>= 2GB)
 * - Archives split over several volumes
 * - Encrypted files, encrypted archives
 * - RAR 2.9/3.6 archives with the Itanium filter
 *
 * Features we don't support, and don't really plan to:
 * - Creating RARs of any kind
 * - RAR version 1.3 (does anybody care about RAR 1.3?)
 * - RAR 2.9/3.6 archives with filters other than the stock filters
 *   Those run on a virtual machine, the RARVM. Not going to implement
 *   a general RARVM
 *
 * Speed-wise, we're a bit faster than The Unarchiver. Depending on the
 * compression mode and features, we're sometimes on par and sometimes a
 * bit slower than the original unrar. The distance to both the original
 * unrar and The Unarchive isn't large, though.
 *
 * Patches and pull requests are always welcome, as are general comments,
 * ideas for improvements, feature requests, etc. You can reach me at
 * <drmccoy@drmccoy.de>, or via the GitHub project page at
 * <https://github.com/DrMcCoy/dmc_unrar>.
 */

/* Version history:
 *
 * Sunday, 2017-03-19 (Version 1.5.1)
 * - Removed usage of variable name "unix"
 * - Fixed dmc_unrar_io_init_file_reader()
 *
 * Saturday, 2017-03-18 (Version 1.5.0)
 * - Documented file attributes for DOS/Windows and Unix
 * - Added more accurate detection of symbolic links
 * - Added dmc_unrar_extract_file_with_callback()
 * - Added archive extraction functions using stdio file
 * - Added simple validator typedefs for type lengths
 *
 * Monday, 2017-03-13 (Version 1.4.0)
 * - Fixed compilation on older gcc
 * - Added support for archive and file comments
 * - Changed file entry timestamp to always be POSIX time
 *   (previously, it was MS-DOS for RAR4, POSIX for RAR5)
 *
 * Sunday, 2017-03-12 (Version 1.3.0)
 * - Fixed a segfault when opening an archive fails
 * - Added CRC-32 validation after file extraction
 * - Made dmc_unrar_crc32_calculate_from_mem() public
 * - Added dmc_unrar_unicode_is_valid_utf8()
 * - Added dmc_unrar_unicode_make_valid_utf8()
 * - Added archive reading functions using stdio file
 * - Added support for SFX archives
 *
 * Saturday, 2017-03-11 (Version 1.2.0)
 * - Added support for solid archives (on all versions)
 *
 * Friday, 2017-03-10 (Version 1.1.0)
 * - Added dmc_unrar_is_rar() and dmc_unrar_is_rar_mem()
 * - Removed extra allocation in dmc_unrar_huff_create_from_lengths()
 * - Replaced a long long integer constant with a cast
 * - Plugged memory leak on fail in huffman decoder create func
 * - Plugged memory leak on fail in filter create func
 * - Implemented RAR 5.0 unpacking
 *
 * Thursday, 2017-03-09 (Version 1.0.0)
 * - Initial public release of dmc_unrar
 */

/* This is a header file library, like stb_image.c. To get only a
 * header file, either cut and paste the below header, or create
 * dmc_unrar.h, #define DMC_UNRAR_HEADER_FILE_ONLY and then include
 * dmc_unrar.c from it.
 */

#ifndef DMC_UNRAR_HEADER_INCLUDED
#define DMC_UNRAR_HEADER_INCLUDED

/* --- System properties --- */

/* 32-bit or 64-bit CPU? Set one to 1, or let the autodetect figure it out later. */
#ifndef DMC_UNRAR_32BIT
#define DMC_UNRAR_32BIT 0
#endif
#ifndef DMC_UNRAR_64BIT
#define DMC_UNRAR_64BIT 0
#endif

/* --- Library features --- */

/* Set DMC_UNRAR_DISABLE_MALLOC to 1 to disable all calls to malloc, realloc
 * and free. Note that if DMC_UNRAR_DISABLE_MALLOC is set to 1, the user must
 * always provide custom user alloc/realloc/free callbacks to the archive API. */
#ifndef DMC_UNRAR_DISABLE_MALLOC
#define DMC_UNRAR_DISABLE_MALLOC 0
#endif

/* The bitstream decoder uses be32toh()/be64toh() on GNU/Linux by default.
 * Set DMC_UNRAR_DISABLE_BE32TOH_BE64TOH to 1 if you don't want that. */
#ifndef DMC_UNRAR_DISABLE_BE32TOH_BE64TOH
#define DMC_UNRAR_DISABLE_BE32TOH_BE64TOH 0
#endif

/* Set DMC_UNRAR_DISABLE_STDIO to 1 to disable functionality that use the
 * stdio file open/read/write functions. */
#ifndef DMC_UNRAR_DISABLE_STDIO
#define DMC_UNRAR_DISABLE_STDIO 0
#endif

/* RAR 2.9/3.6 can optionally compress text using the PPMd algorithm.
 * The PPMd decoder is rather big, uses a lot of memory and needs compiler
 * support for pragma pack. If you don't need to decompress RAR archives
 * with PPMd data, set DMC_UNRAR_DISABLE_PPMD to 1 to disable the whole PPMd
 * decoder. Trying to extract PPMd'd files will then return an error.
 *
 * RAR archives with PPMd data should be relatively rare: WinRAR by default
 * doesn't use this feature and it has to be explicitly enabled. */
#ifndef DMC_UNRAR_DISABLE_PPMD
#define DMC_UNRAR_DISABLE_PPMD 0
#endif

/* RAR 2.9/3.6 can optionally filter the file data through a RARVM program.
 * We don't support generic RARVM bytecode, but we do detect certain
 * commonly used WinRAR stock filters and reimplement them in C. This
 * feature however needs another relatively large memory buffer. If you
 * don't need to decompress RAR archives with filters in them, set
 * DMC_UNRAR_DISABLE_FILTERS to 1 to disable the filters. Trying to
 * extract files with filters will then return an error.
 *
 * RAR archives with filters are relatively common, though: WinRAR by
 * defaults tries a few of them on common files formats, like JPEGs. */
#ifndef DMC_UNRAR_DISABLE_FILTERS
#define DMC_UNRAR_DISABLE_FILTERS 0
#endif

/* Initial capacity of our internal arrays. Larger values mean less
 * reallocations as new files are discovered in an archive, but wasted
 * memory on archives with few files. The arrays grow exponential, though. */
#ifndef DMC_UNRAR_ARRAY_INITIAL_CAPACITY
#define DMC_UNRAR_ARRAY_INITIAL_CAPACITY 8
#endif

/* Number of bytes used for the bitstream buffer. Larger values means more
 * speed, but also more memory. Must be a multiple of 8. 4096 seems to be
 * a good compromise. */
#ifndef DMC_UNRAR_BS_BUFFER_SIZE
#define DMC_UNRAR_BS_BUFFER_SIZE 4096
#endif

/* Max depth to create a Huffman decoding table for. The Huffman decoder uses
 * a dual tree/table approach: codes shorter than this max depth are decoded
 * directly, by a single load from this table. For longer codes, the remaining
 * bits trace a binary table.
 *
 * Higher max depths mean more memory ((2^depth) * 4 bytes), and considerable
 * longer times to build the table in the first place. 10 is a good compromise. */
#ifndef DMC_UNRAR_HUFF_MAX_TABLE_DEPTH
#define DMC_UNRAR_HUFF_MAX_TABLE_DEPTH 10
#endif

/* --- Basic types --- */

/* We need to set those to get be32toh()/be64toh(). */
#if defined(__linux__) && (DMC_UNRAR_DISABLE_BE32TOH_BE64TOH != 1)
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#endif

/* If you don't have stdint.h and/or stddef.h, you need to typedef the following types and macros. */
#include <stdint.h>
#include <stddef.h>

#if 0
typedef signed char int8_t
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef unsigned long long size_t;

#define SIZE_MAX ((size_t) -1)

/* Only used by the PPMd decoder, so could be left out with PPMd disabled. */
typedef signed long long int64_t;
typedef uint64_t uintptr_t;
#endif

#ifndef __cplusplus
/* If you don't have stdbool.h, you need to define the following types and macros. */
#include <stdbool.h>

#if 0
typedef int bool;
#define true (1)
#define false (0)
#endif

#endif /* __cplusplus */

#if DMC_UNRAR_DISABLE_STDIO != 1
#include <stdio.h>
#endif

/* --- System autodetection --- */

/* Autodetecting whether we're on a 64-bit CPU. */
#if (DMC_UNRAR_32BIT != 1) && (DMC_UNRAR_64BIT != 1)
	#undef DMC_UNRAR_32BIT
	#undef DMC_UNRAR_64BIT

	#if defined(_M_X64) || defined(_WIN64) || defined(__MINGW64__) || defined(_LP64) || defined(__LP64__) || defined(__ia64__) || defined(__x86_64__)
		#define DMC_UNRAR_32BIT 0
		#define DMC_UNRAR_64BIT 1
	#else
		#define DMC_UNRAR_32BIT 1
		#define DMC_UNRAR_64BIT 0
	#endif
#elif (DMC_UNRAR_32BIT == 1) && (DMC_UNRAR_64BIT == 1)
	#error Both DMC_UNRAR_32BIT and DMC_UNRAR_64BIT set to 1
#endif

#if (DMC_UNRAR_BS_BUFFER_SIZE <= 0) || ((DMC_UNRAR_BS_BUFFER_SIZE % 8) != 0)
	#error DMC_UNRAR_BS_BUFFER_SIZE must be a multiple of 8
#endif
/* --- API types and macros --- */

/* Heap allocation functions. */
typedef void *(*dmc_unrar_alloc_func)  (void *opaque, size_t items, size_t size);
typedef void *(*dmc_unrar_realloc_func)(void *opaque, void *address, size_t items, size_t size);
typedef void  (*dmc_unrar_free_func)   (void *opaque, void *address);

typedef size_t (*dmc_unrar_read_func)(void *opaque, void *buffer, size_t n);
typedef int    (*dmc_unrar_seek_func)(void *opaque, uint64_t offset);

/* --- Public unrar API --- */

#ifdef __cplusplus
extern "C" {
#endif

/** The return code of a dmc_unrar operation. See dmc_unrar_strerror(). */
typedef enum {
	DMC_UNRAR_OK = 0,

	DMC_UNRAR_NO_ALLOC,
	DMC_UNRAR_ALLOC_FAIL,

	DMC_UNRAR_OPEN_FAIL,
	DMC_UNRAR_READ_FAIL,
	DMC_UNRAR_WRITE_FAIL,
	DMC_UNRAR_SEEK_FAIL,

	DMC_UNRAR_INVALID_DATA,

	DMC_UNRAR_ARCHIVE_EMPTY,

	DMC_UNRAR_ARCHIVE_IS_NULL,
	DMC_UNRAR_ARCHIVE_NOT_CLEARED,
	DMC_UNRAR_ARCHIVE_MISSING_FIELDS,

	DMC_UNRAR_ARCHIVE_NOT_RAR,
	DMC_UNRAR_ARCHIVE_UNSUPPORTED_ANCIENT,

	DMC_UNRAR_ARCHIVE_UNSUPPORTED_VOLUMES,
	DMC_UNRAR_ARCHIVE_UNSUPPORTED_ENCRYPTED,

	DMC_UNRAR_FILE_IS_INVALID,
	DMC_UNRAR_FILE_IS_DIRECTORY,

	DMC_UNRAR_FILE_SOLID_BROKEN,
	DMC_UNRAR_FILE_CRC32_FAIL,

	DMC_UNRAR_FILE_UNSUPPORTED_VERSION,
	DMC_UNRAR_FILE_UNSUPPORTED_METHOD,
	DMC_UNRAR_FILE_UNSUPPORTED_ENCRYPTED,
	DMC_UNRAR_FILE_UNSUPPORTED_SPLIT,
	DMC_UNRAR_FILE_UNSUPPORTED_LINK,
	DMC_UNRAR_FILE_UNSUPPORTED_LARGE,

	DMC_UNRAR_HUFF_RESERVED_SYMBOL,
	DMC_UNRAR_HUFF_PREFIX_PRESENT,
	DMC_UNRAR_HUFF_INVALID_CODE,

	DMC_UNRAR_PPMD_INVALID_MAXORDER,

	DMC_UNRAR_FILTERS_UNKNOWN,
	DMC_UNRAR_FILTERS_INVALID_FILTER_INDEX,
	DMC_UNRAR_FILTERS_REUSE_LENGTH_NEW_FILTER,
	DMC_UNRAR_FILTERS_INVALID_LENGTH,
	DMC_UNRAR_FILTERS_INVALID_FILE_POSITION,
	DMC_UNRAR_FILTERS_XOR_SUM_NO_MATCH,
	DMC_UNRAR_FILTERS_UNSUPPORED_ITANIUM,

	DMC_UNRAR_15_INVALID_FLAG_INDEX,
	DMC_UNRAR_15_INVALID_LONG_MATCH_OFFSET_INDEX,

	DMC_UNRAR_20_INVALID_LENGTH_TABLE_DATA,

	DMC_UNRAR_30_DISABLED_FEATURE_PPMD,
	DMC_UNRAR_30_DISABLED_FEATURE_FILTERS,

	DMC_UNRAR_30_INVALID_LENGTH_TABLE_DATA,

	DMC_UNRAR_50_DISABLED_FEATURE_FILTERS,

	DMC_UNRAR_50_INVALID_LENGTH_TABLE_DATA,
	DMC_UNRAR_50_BLOCK_CHECKSUM_NO_MATCH

} dmc_unrar_return;

/** The operating system a file was packed into a RAR. */
typedef enum {
	DMC_UNRAR_HOSTOS_DOS   = 0, /**< DOS, MS-DOS. */
	DMC_UNRAR_HOSTOS_OS2   = 1, /**< OS/2. */
	DMC_UNRAR_HOSTOS_WIN32 = 2, /**< Windows. */
	DMC_UNRAR_HOSTOS_UNIX  = 3, /**< Unix. */
	DMC_UNRAR_HOSTOS_MACOS = 4, /**< Mac OS. */
	DMC_UNRAR_HOSTOS_BEOS  = 5  /**< BeOS. */
} dmc_unrar_host_os;

/** DOS/Windows file attributes. */
typedef enum {
	DMC_UNRAR_ATTRIB_DOS_READONLY    = 0x00001,
	DMC_UNRAR_ATTRIB_DOS_HIDDEN      = 0x00002,
	DMC_UNRAR_ATTRIB_DOS_SYSTEM      = 0x00004,
	DMC_UNRAR_ATTRIB_DOS_VOLUMELABEL = 0x00008,
	DMC_UNRAR_ATTRIB_DOS_DIRECTORY   = 0x00010,
	DMC_UNRAR_ATTRIB_DOS_ARCHIVE     = 0x00020,
	DMC_UNRAR_ATTRIB_DOS_DEVICE      = 0x00040,
	DMC_UNRAR_ATTRIB_DOS_NORMAL      = 0x00080,
	DMC_UNRAR_ATTRIB_DOS_TEMPORARY   = 0x00100,
	DMC_UNRAR_ATTRIB_DOS_SPARSE      = 0x00200,
	DMC_UNRAR_ATTRIB_DOS_SYMLINK     = 0x00400,
	DMC_UNRAR_ATTRIB_DOS_COMPRESSED  = 0x00800,
	DMC_UNRAR_ATTRIB_DOS_OFFLINE     = 0x01000,
	DMC_UNRAR_ATTRIB_DOS_NOTINDEXED  = 0x02000,
	DMC_UNRAR_ATTRIB_DOS_ENCRYPTED   = 0x04000,
	DMC_UNRAR_ATTRIB_DOS_INTEGRITY   = 0x08000,
	DMC_UNRAR_ATTRIB_DOS_VIRTUAL     = 0x10000,
	DMC_UNRAR_ATTRIB_DOS_NOSCRUB     = 0x20000
} dmc_unrar_windows_attribute;

/** Unix file attributes. */
typedef enum {
	/* Mask to check for the types of a file. */
	DMC_UNRAR_ATTRIB_UNIX_FILETYPE_MASK       = 0170000,
	/* Mask to check for the permissions of a file. */
	DMC_UNRAR_ATTRIB_UNIX_PERMISSIONS_MASK    = 0007777,

	/* .--- File types. Mutually exclusive. */
	DMC_UNRAR_ATTRIB_UNIX_IS_SYMBOLIC_LINK    = 0120000,
	DMC_UNRAR_ATTRIB_UNIX_IS_SOCKET           = 0140000,

	DMC_UNRAR_ATTRIB_UNIX_IS_REGULAR_FILE     = 0100000,

	DMC_UNRAR_ATTRIB_UNIX_IS_BLOCK_DEVICE     = 0060000,
	DMC_UNRAR_ATTRIB_UNIX_IS_DIRECTORY        = 0040000,
	DMC_UNRAR_ATTRIB_UNIX_IS_CHARACTER_DEVICE = 0020000,
	DMC_UNRAR_ATTRIB_UNIX_IS_FIFO             = 0010000,
	/* '--- */

	/* .--- File permissions. OR-able. */
	DMC_UNRAR_ATTRIB_UNIX_SET_USER_ID         = 0004000,
	DMC_UNRAR_ATTRIB_UNIX_SET_GROUP_ID        = 0002000,
	DMC_UNRAR_ATTRIB_UNIX_STICKY              = 0001000,

	DMC_UNRAR_ATTRIB_UNIX_USER_READ           = 0000400,
	DMC_UNRAR_ATTRIB_UNIX_USER_WRITE          = 0000200,
	DMC_UNRAR_ATTRIB_UNIX_USER_EXECUTE        = 0000100,
	DMC_UNRAR_ATTRIB_UNIX_GROUP_READ          = 0000040,
	DMC_UNRAR_ATTRIB_UNIX_GROUP_WRITE         = 0000020,
	DMC_UNRAR_ATTRIB_UNIX_GROUP_EXECUTE       = 0000010,
	DMC_UNRAR_ATTRIB_UNIX_OTHER_READ          = 0000004,
	DMC_UNRAR_ATTRIB_UNIX_OTHER_WRITE         = 0000002,
	DMC_UNRAR_ATTRIB_UNIX_OTHER_EXECUTE       = 0000001
	/* '--- */
} dmc_unrar_unix_attribute;

struct dmc_unrar_internal_state_tag;
typedef struct dmc_unrar_internal_state_tag dmc_unrar_internal_state;

/** A file entry within a RAR archive. */
typedef struct dmc_unrar_file_tag {
	uint64_t compressed_size;   /**< Size of the compressed file data, in bytes. */
	uint64_t uncompressed_size; /**< Size of the uncompressed file data, in bytes. */

	/** The operating system on which the file was packed into the RAR. */
	dmc_unrar_host_os host_os;

	bool has_crc; /**< Does this file entry have a checksum? */

	uint32_t crc;       /**< Checksum (CRC-32, 0xEDB88320 polynomial). */
	uint64_t unix_time; /**< File modification timestamp, POSIX epoch format. */

	/** File attributes, operating-system-specific.
	 *
	 *  The meaning depends on the host_os value:
	 *  - DMC_UNRAR_HOSTOS_DOS:   see dmc_unrar_windows_attribute
	 *  - DMC_UNRAR_HOSTOS_OS2:   ???
	 *  - DMC_UNRAR_HOSTOS_WIN32: see dmc_unrar_windows_attribute
	 *  - DMC_UNRAR_HOSTOS_UNIX:  see dmc_unrar_unix_attribute
	 *  - DMC_UNRAR_HOSTOS_MACOS: ???
	 *  - DMC_UNRAR_HOSTOS_BEOS:  ???
	 */
	uint64_t attrs;

} dmc_unrar_file;

typedef struct dmc_unrar_alloc_tag {
	dmc_unrar_alloc_func func_alloc;     /**< Memory allocation function, or NULL to use malloc(). */
	dmc_unrar_realloc_func func_realloc; /**< Memory allocation function, or NULL to use realloc(). */
	dmc_unrar_free_func func_free;       /**< Memory deallocation function, or NULL to use free(). */
	void *opaque;                        /**< Private data passed to func_alloc, func_realloc and func_free. */

} dmc_unrar_alloc;

typedef struct dmc_unrar_io_tag {
	dmc_unrar_read_func func_read; /**< RAR file reading function. Must not be NULL. */
	dmc_unrar_seek_func func_seek; /**< RAR file seeking function. Must not be NULL. */
	void *opaque;                  /**< Private data passed to func_read and func_seek. */

	uint64_t offset; /**< Current offset within the IO stream. */
	uint64_t size;   /**< Size of the IO stream. */

} dmc_unrar_io;

/** A RAR archive. */
typedef struct dmc_unrar_archive_tag {
	dmc_unrar_alloc alloc;
	dmc_unrar_io io;

	/** Private internal state. */
	dmc_unrar_internal_state *internal_state;

} dmc_unrar_archive;

/** Return a human-readable description of a return code. */
const char *dmc_unrar_strerror(dmc_unrar_return code);

/** Detect whether an IO structure contains a RAR archive. */
bool dmc_unrar_is_rar(dmc_unrar_io *io);

/** Detect whether the memory region contains a RAR archive. */
bool dmc_unrar_is_rar_mem(const void *mem, size_t size);

#if DMC_UNRAR_DISABLE_STDIO != 1
/* Detect whether this FILE contains a RAR archive. */
bool dmc_unrar_is_rar_file(FILE *file);

/* Detect whether the file at this path contains a RAR archive. */
bool dmc_unrar_is_rar_path(const char *path);
#endif /* DMC_UNRAR_DISABLE_STDIO */

/** Initialize/clear this archive struct.
 *
 *  @param  archive A valid pointer to an archive structure to initialize.
 *  @return DMC_UNRAR_OK on success. Any other value is an error condition.
 */
dmc_unrar_return dmc_unrar_archive_init(dmc_unrar_archive *archive);

/** Open this RAR archive, reading its block and file headers.
 *  The func_read, func_read and opaque_io fields have to be set.
 *  The func_alloc, func_realloc, func_free and opaque_mem fields may be set.
 *  All other fields must have been cleared.
 *
 *  @param  archive Pointer to the archive structure to use. Needs to be a valid
 *                  pointer, with the fields properly initialized and set.
 *  @param  size Size of the RAR file described by the archive fields.
 *  @return DMC_UNRAR_OK if the archive was successfully opened. Any other value
 *          describes an error condition.
 */
dmc_unrar_return dmc_unrar_archive_open(dmc_unrar_archive *archive, uint64_t size);

/** Open this RAR archive from a memory block, reading its block and file headers.
 *  The func_alloc, func_realloc, func_free and opaque_mem fields may be set.
 *  All other fields must have been cleared.
 *
 *  @param  archive Pointer to the archive structure to use. Needs to be a valid
 *                  pointer, with the fields properly initialized and set.
 *  @param  mem Pointer to a block of memory to read the RAR file out of.
 *  @param  size Size of the RAR memory region.
 *  @return DMC_UNRAR_OK if the archive was successfully opened. Any other value
 *          describes an error condition.
 */
dmc_unrar_return dmc_unrar_archive_open_mem(dmc_unrar_archive *archive,
	const void *mem, size_t size);

#if DMC_UNRAR_DISABLE_STDIO != 1
/** Open this RAR archive from a stdio FILE, reading its block and file headers.
 *  The func_alloc, func_realloc, func_free and opaque_mem fields may be set.
 *  All other fields must have been cleared.
 *
 *  @param  archive Pointer to the archive structure to use. Needs to be a valid
 *                  pointer, with the fields properly initialized and set.
 *  @param  file The stdio FILE structure to read out of.
 *  @return DMC_UNRAR_OK if the archive was successfully opened. Any other value
 *          describes an error condition.
 */
dmc_unrar_return dmc_unrar_archive_open_file(dmc_unrar_archive *archive, FILE *file);

/** Open this RAR archive from a path, opening the file with fopen(), and reading
 *  its block and file headers. The func_alloc, func_realloc, func_free and
 *   opaque_mem fields may be set. All other fields must have been cleared.
 *
 *  @param  archive Pointer to the archive structure to use. Needs to be a valid
 *                  pointer, with the fields properly initialized and set.
 *  @param  path The path of the file to fopen() and read out of.
 *  @return DMC_UNRAR_OK if the archive was successfully opened. Any other value
 *          describes an error condition.
 */
dmc_unrar_return dmc_unrar_archive_open_path(dmc_unrar_archive *archive, const char *path);
#endif /* DMC_UNRAR_DISABLE_STDIO */

/** Close this RAR archive again.
 *
 *  All allocated memory will be freed. */
void dmc_unrar_archive_close(dmc_unrar_archive *archive);

/** Get the global archive comment of a RAR archive.
 *
 *  Note: we don't necessarily know the encoding of this data, nor is
 *  the data always \0-terminated or even a human-readable string!
 *
 *  - RAR 5.0 always stores UTF-8 data.
 *  - RAR 2.9/3.6 stores either ASCII or UTF-16LE data.
 *    We don't know which is which.
 *  - RAR 2.0/2.6 stores *anything*.
 *  - RAR 1.5 doesn't support archive comments.
 *
 *  Use dmc_unrar_unicode_detect_encoding() to roughly detect the
 *  encoding of a comment.
 *
 *  Use dmc_unrar_unicode_convert_utf16le_to_utf8() to convert a
 *  UTF-16LE comment into UTF-8.
 *
 *  Returns the number of bytes written to comment. If comment is NULL, this function
 *  returns the number of bytes needed to fully store the comment.
 */
size_t dmc_unrar_get_archive_comment(dmc_unrar_archive *archive, void *comment, size_t comment_size);

/** Return the number of file entries in this RAR archive. */
size_t dmc_unrar_get_file_count(dmc_unrar_archive *archive);

/** Return the detailed information about a file entry, or NULL on error.
 *  Does not need to be free'd. */
const dmc_unrar_file *dmc_unrar_get_file_stat(dmc_unrar_archive *archive, size_t index);

/** Get the filename of a RAR file entry, UTF-8 encoded and \0-terminated.
 *
 *  Note: the filename is *not* checked to make sure it contains fully
 *  valid UTF-8 data. Use dmc_unrar_unicode_is_valid_utf8() and/or
 *  dmc_unrar_unicode_make_valid_utf8() for that.
 *
 *  Returns the number of bytes written to filename. If filename is NULL, this function
 *  returns the number of bytes needed to fully store the filename.
 */
size_t dmc_unrar_get_filename(dmc_unrar_archive *archive, size_t index,
	char *filename, size_t filename_size);

/** Is this file entry a directory? */
bool dmc_unrar_file_is_directory(dmc_unrar_archive *archive, size_t index);

/** Does this file entry have a comment attached? */
bool dmc_unrar_file_has_comment(dmc_unrar_archive *archive, size_t index);

/** Check if we support extracted this file entry.
 *
 *  If we do support extracting this file entry, DMC_UNRAR_OK is returned.
 *  Otherwise, the return code gives an idea why we don't have support. */
dmc_unrar_return dmc_unrar_file_is_supported(dmc_unrar_archive *archive, size_t index);

/** Get the comment of a file entry.
 *
 *  Note: we don't necessarily know the encoding of this data, nor is
 *  the data always \0-terminated or even a human-readable string!
 *
 *  Only RAR 2.0/2.6 supports file comments.
 *
 *  Use dmc_unrar_unicode_detect_encoding() to roughly detect the
 *  encoding of a comment.
 *
 *  Use dmc_unrar_unicode_convert_utf16le_to_utf8() to convert a
 *  UTF-16LE comment into UTF-8.
 *
 *  Returns the number of bytes written to comment. If comment is NULL, this function
 *  returns the number of bytes needed to fully store the comment.
 */
size_t dmc_unrar_get_file_comment(dmc_unrar_archive *archive, size_t index,
	void *comment, size_t comment_size);

/** Extract a file entry into a pre-allocated memory buffer.
 *
 *  @param  archive The archive to extract from.
 *  @param  index The index of the file entry to extract.
 *  @param  buffer The pre-allocated memory buffer to extract into.
 *  @param  buffer_size The size of the pre-allocated memory buffer.
 *  @param  uncompressed_size If != NULL, the number of bytes written
 *          to the buffer will be stored here.
 *  @param  validate_crc If true, validate the uncompressed data against
 *          the CRC-32 stored within the archive. If the validation fails,
 *          this counts as an error (DMC_UNRAR_FILE_CRC32_FAIL).
 *  @return An error condition, or DMC_UNRAR_OK if extraction succeeded.
 */
dmc_unrar_return dmc_unrar_extract_file_to_mem(dmc_unrar_archive *archive, size_t index,
	void *buffer, size_t buffer_size, size_t *uncompressed_size, bool validate_crc);

/** Extract a file entry into a dynamically allocated heap buffer.
 *
 *  @param  archive The archive to extract from.
 *  @param  index The index of the file entry to extract.
 *  @param  buffer The heap-allocated memory buffer will be stored here.
 *  @param  uncompressed_size The size of the heap-allocated memory buffer
 *          will be stored here. Must not be NULL.
 *  @param  validate_crc If true, validate the uncompressed data against
 *          the CRC-32 stored within the archive. If the validation fails,
 *          this counts as an error (DMC_UNRAR_FILE_CRC32_FAIL).
 *  @return An error condition, or DMC_UNRAR_OK if extraction succeeded.
 */
dmc_unrar_return dmc_unrar_extract_file_to_heap(dmc_unrar_archive *archive, size_t index,
	void **buffer, size_t *uncompressed_size, bool validate_crc);

/** The callback function for dmc_unrar_extract_file_with_callback().
 *
 *  Note that even with small buffer slices, decompressing a buffer
 *  full might take an unexpected long time, if the requested file
 *  is part of a solid block and/or uses the PPMd decoder.
 *
 *  @param  opaque Opaque memory pointer for personal use.
 *  @param  buffer Pointer to the buffer where the current part of the
 *          extracted file resides. Can be changed, to use a different
 *          buffer for further extraction. Can be set to NULL to let
 *          dmc_unrar_extract_file_with_callback() allocate its own
 *          internal buffer.
 *  @param  buffer_size Size of the buffer. Can be modified, to use
 *          a different buffer size for further extraction.
 *  @param  uncompressed_size Number of bytes of extracted file waiting
 *          in the buffer.
 *  @param  err In combination with returning false, the callback can
 *          set this parameter to something other than DMC_UNRAR_OK to
 *          signal an error. dmc_unrar_extract_file_with_callback() will
 *          return with that error condition.
 *  @return true if extraction should continue, false otherwise.
 */
typedef bool (*dmc_unrar_extract_callback_func)(void *opaque, void **buffer,
	size_t *buffer_size, size_t uncompressed_size, dmc_unrar_return *err);

/** Extract a file entry using a callback function.
 *
 *  Extract into the buffer of buffer_size, calling callback every time the
 *  buffer has been filled (or all the input has been processed).
 *
 *  @param  archive The archive to extract from.
 *  @param  index The index of the file entry to extract.
 *  @param  buffer The pre-allocated memory buffer to extract into. Can be
 *          NULL to mean that a buffer of buffer_size should be allocated.
 *  @param  buffer_size The size of the output buffer.
 *  @param  uncompressed_size If != NULL, the total number of bytes written
 *          to the buffer will be stored here.
 *  @param  validate_crc If true, validate the uncompressed data against
 *          the CRC-32 stored within the archive. If the validation fails,
 *          this counts as an error (DMC_UNRAR_FILE_CRC32_FAIL).
 *  @param  opaque Opaque memory pointer to pass to the callback.
 *  @param  callback The callback to call.
 *  @return An error condition, or DMC_UNRAR_OK if extraction succeeded.
 */
dmc_unrar_return dmc_unrar_extract_file_with_callback(dmc_unrar_archive *archive, size_t index,
	void *buffer, size_t buffer_size, size_t *uncompressed_size, bool validate_crc,
	void *opaque, dmc_unrar_extract_callback_func callback);

#if DMC_UNRAR_DISABLE_STDIO != 1
/** Extract a file entry into a file.
 *
 *  @param  archive The archive to extract from.
 *  @param  index The index of the file entry to extract.
 *  @param  file The file to write into.
 *  @param  uncompressed_size If not NULL, the number of bytes written
 *          to the file will be stored here.
 *  @param  validate_crc If true, validate the uncompressed data against
 *          the CRC-32 stored within the archive. If the validation fails,
 *          this counts as an error (DMC_UNRAR_FILE_CRC32_FAIL).
 *  @return An error condition, or DMC_UNRAR_OK if extraction succeeded.
 */
dmc_unrar_return dmc_unrar_extract_file_to_file(dmc_unrar_archive *archive, size_t index,
	FILE *file, size_t *uncompressed_size, bool validate_crc);

/** Open a file and extract a RAR file entry into it.
 *
 *  @param  archive The archive to extract from.
 *  @param  index The index of the file entry to extract.
 *  @param  path The file to open and write into.
 *  @param  uncompressed_size If not NULL, the number of bytes written
 *          to the file will be stored here.
 *  @param  validate_crc If true, validate the uncompressed data against
 *          the CRC-32 stored within the archive. If the validation fails,
 *          this counts as an error (DMC_UNRAR_FILE_CRC32_FAIL).
 *  @return An error condition, or DMC_UNRAR_OK if extraction succeeded.
 */
dmc_unrar_return dmc_unrar_extract_file_to_path(dmc_unrar_archive *archive, size_t index,
	const char *path, size_t *uncompressed_size, bool validate_crc);
#endif /* DMC_UNRAR_DISABLE_STDIO */

/** Return true if the given \0-terminated string contains valid UTF-8 data. */
bool dmc_unrar_unicode_is_valid_utf8(const char *str);

/** Cut off the given \0-terminated string at the first invalid UTF-8 sequence.
 *
 *  @param str The string to check and potentially modify.
 *  @return True if the string was modified, false otherwise.
 */
bool dmc_unrar_unicode_make_valid_utf8(char *str);

typedef enum {
	DMC_UNRAR_UNICODE_ENCODING_UTF8,
	DMC_UNRAR_UNICODE_ENCODING_UTF16LE,

	DMC_UNRAR_UNICODE_ENCODING_UNKNOWN

} dmc_unrar_unicode_encoding;

/** Try to detect the encoding of a memory region containing human-readable text.
 *
 *  This is of course far from 100% reliable. The detection is rather simplistic
 *  and just meant to roughly detect the encoding of archive comments.
 *
 *  This function does not check for \0-termination.
 */
dmc_unrar_unicode_encoding dmc_unrar_unicode_detect_encoding(const void *data, size_t data_size);

/** Convert UTF-16LE data into UTF-8.
 *
 *  Conversion will stop at the first invalid UTF-16 sequence. The result will
 *  always be fully valid, \0-terminated UTF-8 string, but possibly cut off.
 *
 *  A leading UTF-16LE BOM will be removed.
 *
 *  @param utf16le_size Size of utf16le_data in bytes.
 *  @param utf8_size Size of utf8_data in bytes.
 *
 *  Returns the number of bytes written to utf8_data. If utf8_data is NULL, this
 *  function returns the number of bytes needed to fully store the UTF-8 string.
 */
size_t dmc_unrar_unicode_convert_utf16le_to_utf8(const void *utf16le_data, size_t utf16le_size,
	char *utf8_data, size_t utf8_size);

/** Calculate a CRC-32 (0xEDB88320 polynomial) checksum from this memory region. */
uint32_t dmc_unrar_crc32_calculate_from_mem(const void *mem, size_t size);

/** Append the CRC-32 (0xEDB88320 polynomial) checksum calculate from this memory region
  * to the CRC-32 of a previous memory region. The result is the CRC-32 of the two
  * memory regions pasted together.
  *
  * I.e. these two functions will result in the same value:
  *
  * uint32_t crc32_1(const uint8_t *mem, size_t size) {
  *   assert(size >= 10);
  *   return dmc_unrar_crc32_calculate_from_mem(mem, size);
  * }
  *
  * uint32_t crc32_2(const uint8_t *mem, size_t size) {
  *   assert(size >= 10);
  *   uint32_t crc = dmc_unrar_crc32_calculate_from_mem(mem, 10);
  *   dmc_unrar_crc32_continue_from_mem(crc, mem + 10, size - 10);
  *   return crc;
  * }
  */
uint32_t dmc_unrar_crc32_continue_from_mem(uint32_t hash, const void *mem, size_t size);
#ifdef __cplusplus
}
#endif

#endif /* DMC_UNRAR_HEADER_INCLUDED */

/* --- End of header, implementation follows --- */

#ifndef DMC_UNRAR_HEADER_FILE_ONLY

typedef unsigned char dmc_unrar_validate_uint8 [sizeof(uint8_t )==1 ? 1 : -1];
typedef unsigned char dmc_unrar_validate_uint16[sizeof(uint16_t)==2 ? 1 : -1];
typedef unsigned char dmc_unrar_validate_uint32[sizeof(uint32_t)==4 ? 1 : -1];
typedef unsigned char dmc_unrar_validate_uint64[sizeof(uint64_t)==8 ? 1 : -1];
typedef unsigned char dmc_unrar_validate_int8  [sizeof( int8_t )==1 ? 1 : -1];
#if DMC_UNRAR_DISABLE_PPMD != 1
typedef unsigned char dmc_unrar_validate_int64 [sizeof( int64_t)==8 ? 1 : -1];
#endif

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define DMC_UNRAR_ASSERT(x) assert(x)

#if DMC_UNRAR_DISABLE_MALLOC == 1
  #define DMC_UNRAR_MALLOC(x) NULL
  #define DMC_UNRAR_REALLOC(p, x) NULL
  #define DMC_UNRAR_FREE(x) (void)x, ((void)0)
#else
  #define DMC_UNRAR_MALLOC(x) malloc(x)
  #define DMC_UNRAR_REALLOC(p, x) realloc(p, x)
  #define DMC_UNRAR_FREE(x) free(x)
#endif

#define DMC_UNRAR_MAX(a,b) (((a)>(b))?(a):(b))
#define DMC_UNRAR_MIN(a,b) (((a)<(b))?(a):(b))
#define DMC_UNRAR_ABS(a)   (((a)<0)?-(a):(a))

#define DMC_UNRAR_ARRAYSIZE(x) (sizeof(x) / sizeof(x[0]))

#define DMC_UNRAR_CLEAR_OBJ(obj)     memset(&(obj), 0, sizeof(obj))
#define DMC_UNRAR_CLEAR_OBJS(obj, n) memset((obj), 0, (n) * sizeof((obj)[0]))
#define DMC_UNRAR_CLEAR_ARRAY(obj)   memset((obj), 0, sizeof(obj))

#ifdef __cplusplus
extern "C" {
#endif

const char *dmc_unrar_strerror(dmc_unrar_return code) {
	switch (code) {
		case DMC_UNRAR_OK:
			return "Success";

		case DMC_UNRAR_NO_ALLOC:
			return "No alloc/free callbacks provided";

		case DMC_UNRAR_ALLOC_FAIL:
			return "Memory allocation failed";

		case DMC_UNRAR_OPEN_FAIL:
			return "File open error";

		case DMC_UNRAR_READ_FAIL:
			return "Read error";

		case DMC_UNRAR_WRITE_FAIL:
			return "Write error";

		case DMC_UNRAR_SEEK_FAIL:
			return "Seek error";

		case DMC_UNRAR_INVALID_DATA:
			return "Invalid data";

		case DMC_UNRAR_ARCHIVE_EMPTY:
			return "Provided archive file is empty (NULL pointer or size == 0)";

		case DMC_UNRAR_ARCHIVE_IS_NULL:
			return "Provided archive is NULL";

		case DMC_UNRAR_ARCHIVE_NOT_CLEARED:
			return "Archive has not been properly cleared";

		case DMC_UNRAR_ARCHIVE_MISSING_FIELDS:
			return "Mandatory archive fields have not set";

		case DMC_UNRAR_ARCHIVE_NOT_RAR:
			return "Not a RAR file";

		case DMC_UNRAR_ARCHIVE_UNSUPPORTED_ANCIENT:
			return "Unsupported ancient RAR version 1.3";

		case DMC_UNRAR_ARCHIVE_UNSUPPORTED_VOLUMES:
			return "Unsupported archive feature: volumes";

		case DMC_UNRAR_ARCHIVE_UNSUPPORTED_ENCRYPTED:
			return "Unsupported archive feature: encryption";

		case DMC_UNRAR_FILE_IS_INVALID:
			return "Invalid file entry";

		case DMC_UNRAR_FILE_IS_DIRECTORY:
			return "File entry is a directory";

		case DMC_UNRAR_FILE_SOLID_BROKEN:
			return "File entry is part of a broken solid block";

		case DMC_UNRAR_FILE_CRC32_FAIL:
			return "File CRC-32 checksum mismatch";

		case DMC_UNRAR_FILE_UNSUPPORTED_VERSION:
			return "Unsupported compression version";

		case DMC_UNRAR_FILE_UNSUPPORTED_METHOD:
			return "Unsupported compression method";

		case DMC_UNRAR_FILE_UNSUPPORTED_ENCRYPTED:
			return "Unsupported file feature: encryption";

		case DMC_UNRAR_FILE_UNSUPPORTED_SPLIT:
			return "Unsupported file feature: split file";

		case DMC_UNRAR_FILE_UNSUPPORTED_LINK:
			return "Unsupported file feature: link";

		case DMC_UNRAR_FILE_UNSUPPORTED_LARGE:
			return "Unsupported large file";

		case DMC_UNRAR_HUFF_RESERVED_SYMBOL:
			return "Reserved Huffman symbol";

		case DMC_UNRAR_HUFF_PREFIX_PRESENT:
			return "Huffman prefix already present";

		case DMC_UNRAR_HUFF_INVALID_CODE:
			return "Invalid Huffman code in bitstream";

		case DMC_UNRAR_PPMD_INVALID_MAXORDER:
			return "Invalid PPMd max order";

		case DMC_UNRAR_FILTERS_UNKNOWN:
			return "Unrecognized RARVM filter";

		case DMC_UNRAR_FILTERS_INVALID_FILTER_INDEX:
			return "Invalid filter index";

		case DMC_UNRAR_FILTERS_REUSE_LENGTH_NEW_FILTER:
			return "Can't reuse length on new filter";

		case DMC_UNRAR_FILTERS_INVALID_LENGTH:
			return "Invalid filter length";

		case DMC_UNRAR_FILTERS_INVALID_FILE_POSITION:
			return "Invalid file position in filter";

		case DMC_UNRAR_FILTERS_XOR_SUM_NO_MATCH:
			return "Filter xor sum doesn't match";

		case DMC_UNRAR_FILTERS_UNSUPPORED_ITANIUM:
			return "Unsupported filter: Itanium";

		case DMC_UNRAR_15_INVALID_FLAG_INDEX:
			return "Invalid flag index in RAR 1.5 decoder";

		case DMC_UNRAR_15_INVALID_LONG_MATCH_OFFSET_INDEX:
			return "Invalid long match offset index in RAR 1.5 decoder";

		case DMC_UNRAR_20_INVALID_LENGTH_TABLE_DATA:
			return "Invalid length table data in RAR 2.0/2.6 decoder";

		case DMC_UNRAR_30_DISABLED_FEATURE_PPMD:
			return "Disabled RAR 2.9/3.6 feature: PPMd";

		case DMC_UNRAR_30_DISABLED_FEATURE_FILTERS:
			return "Disabled RAR 2.9/3.6 feature: filters";

		case DMC_UNRAR_30_INVALID_LENGTH_TABLE_DATA:
			return "Invalid length table data in RAR 2.9/3.6 decoder";

		case DMC_UNRAR_50_DISABLED_FEATURE_FILTERS:
			return "Disabled RAR 5.0 feature: filters";

		case DMC_UNRAR_50_INVALID_LENGTH_TABLE_DATA:
			return "Invalid length table data in RAR 5.0 decoder";

		case DMC_UNRAR_50_BLOCK_CHECKSUM_NO_MATCH:
			return "RAR 5.0 block checksum doesn't match";

		default:
			break;
	}

	return "Unknown error";
}

typedef enum {
	DMC_UNRAR_GENERATION_INVALID = 0,

	DMC_UNRAR_GENERATION_ANCIENT,
	DMC_UNRAR_GENERATION_RAR4,
	DMC_UNRAR_GENERATION_RAR5
} dmc_unrar_generation;

/** Generic RAR4 block flags. */
enum {
	DMC_UNRAR_FLAG4_BLOCK_LONG = 0x8000 /**< This block is "long", with extra data besides the header. */
};

/** Generic RAR5 block flags. */
enum {
	DMC_UNRAR_FLAG5_BLOCK_EXTRA = 0x0001, /**< This block has extra file property data. */
	DMC_UNRAR_FLAG5_BLOCK_DATA  = 0x0002 /**< This block has data .*/
};

/** RAR4 archive header block flags. */
enum {
	DMC_UNRAR_FLAG4_ARCHIVE_VOLUMES        = 0x0001, /**< This archive is split into several volumes. */
	DMC_UNRAR_FLAG4_ARCHIVE_HASCOMMENT     = 0x0002, /**< This archive has a comment attached. */
	DMC_UNRAR_FLAG4_ARCHIVE_LOCK           = 0x0004, /**< This archive is locked (can't be modified). */
	DMC_UNRAR_FLAG4_ARCHIVE_SOLID          = 0x0008, /**< This archive contains solid blocks. */
	DMC_UNRAR_FLAG4_ARCHIVE_NEWNUMBERING   = 0x0010, /**< This archive uses the new volume numbering scheme. */
	DMC_UNRAR_FLAG4_ARCHIVE_AV             = 0x0020, /**< This archive has authenticity information. */
	DMC_UNRAR_FLAG4_ARCHIVE_PROTECT        = 0x0040, /**< This archive has a recovery record. */
	DMC_UNRAR_FLAG4_ARCHIVE_ENCRYPTED      = 0x0080, /**< This archive is fully encrypted. */
	DMC_UNRAR_FLAG4_ARCHIVE_FIRSTVOLUME    = 0x0100, /**< This archive is the first volume. */
	DMC_UNRAR_FLAG4_ARCHIVE_ENCRYPTVERSION = 0x0200  /**< This archive has an encryption version number. */
};

/** RAR4 file block flags. */
enum {
	DMC_UNRAR_FLAG4_FILE_SPLITBEFORE = 0x0001, /**< This file continues from a previous part. */
	DMC_UNRAR_FLAG4_FILE_SPLITAFTER  = 0x0002, /**< This file continues into a next part. */
	DMC_UNRAR_FLAG4_FILE_ENCRYPTED   = 0x0004, /**< This file is encrypted. */
	DMC_UNRAR_FLAG4_FILE_HASCOMMENT  = 0x0008, /**< This file has a comment attached. */
	DMC_UNRAR_FLAG4_FILE_SOLID       = 0x0010, /**< This file is part of a previous solid block. */
	DMC_UNRAR_FLAG4_FILE_LARGE       = 0x0100, /**< This file is large (> 4GB). */
	DMC_UNRAR_FLAG4_FILE_NAMEUNICODE = 0x0200, /**< This file name is encoded in Unicode. */
	DMC_UNRAR_FLAG4_FILE_HASSALT     = 0x0400, /**< This file has a salt for encryption. */
	DMC_UNRAR_FLAG4_FILE_HASVERSION  = 0x0800, /**< This file has version information. */
	DMC_UNRAR_FLAG4_FILE_HASEXTTIME  = 0x1000, /**< This file has extra time information. */
	DMC_UNRAR_FLAG4_FILE_HASEXTFLAGS = 0x2000, /**< This file has extra access permission flags. */

	DMC_UNRAR_FLAG4_FILE_WINDOWMASK  = 0x00E0, /**< The mask for the size of the sliding window. */
	DMC_UNRAR_FLAG4_FILE_WINDOW64    = 0x0000,
	DMC_UNRAR_FLAG4_FILE_WINDOW128   = 0x0020,
	DMC_UNRAR_FLAG4_FILE_WINDOW256   = 0x0040,
	DMC_UNRAR_FLAG4_FILE_WINDOW512   = 0x0060,
	DMC_UNRAR_FLAG4_FILE_WINDOW1024  = 0x0080,
	DMC_UNRAR_FLAG4_FILE_WINDOW2048  = 0x00A0,
	DMC_UNRAR_FLAG4_FILE_WINDOW4096  = 0x00C0,

	DMC_UNRAR_FLAG4_FILE_WINDOWDIR   = 0x00E0  /**< All window bits set == this is a directory. */
};

/** RAR5 file block flags. */
enum {
	DMC_UNRAR_FLAG5_FILE_ISDIRECTORY = 0x0001, /**< This file entry is a directory. */
	DMC_UNRAR_FLAG5_FILE_HASTIME     = 0x0002, /**< This file has a timestamp. */
	DMC_UNRAR_FLAG5_FILE_HASCRC      = 0x0004, /**< This file has a checksum. */
	DMC_UNRAR_FLAG5_FILE_SPLITBEFORE = 0x0008, /**< This file continues from a previous part. */
	DMC_UNRAR_FLAG5_FILE_SPLITAFTER  = 0x0010  /**< This file continues into a next part. */
};

/** Exact type of a RAR4 block. */
enum {
	DMC_UNRAR_BLOCK4_TYPE_MARKER        = 0x72, /**< File marker/magic number. */
	DMC_UNRAR_BLOCK4_TYPE_ARCHIVEHEADER = 0x73, /**< Information header describing the archive. */
	DMC_UNRAR_BLOCK4_TYPE_FILE          = 0x74, /**< A file within the archive. */
	DMC_UNRAR_BLOCK4_TYPE_COMMENT       = 0x75, /**< An archive comment. */
	DMC_UNRAR_BLOCK4_TYPE_AV            = 0x76,
	DMC_UNRAR_BLOCK4_TYPE_SUB           = 0x77,
	DMC_UNRAR_BLOCK4_TYPE_PROTECT       = 0x78,
	DMC_UNRAR_BLOCK4_TYPE_NEWSUB        = 0x7A,
	DMC_UNRAR_BLOCK4_TYPE_END           = 0x7B  /**< Archive end marker. */
};

/** Exact type of a RAR5 block. */
enum {
	DMC_UNRAR_BLOCK5_TYPE_ARCHIVEHEADER = 0x01, /**< Information header describing the archive. */
	DMC_UNRAR_BLOCK5_TYPE_FILE          = 0x02, /**< A file within the archive. */
	DMC_UNRAR_BLOCK5_TYPE_SERVICE       = 0x03, /**< Service header. */
	DMC_UNRAR_BLOCK5_TYPE_ENCRYPTION    = 0x04, /**< Archive encryption header. */
	DMC_UNRAR_BLOCK5_TYPE_END           = 0x05  /**< Archive end marker. */
};

/** Type of a RAR5 file property field. */
enum {
	DMC_UNRAR_FILE5_PROPERTY_ENCRYPTION  = 0x01, /**< Encryption info. */
	DMC_UNRAR_FILE5_PROPERTY_HASH        = 0x02, /**< File hash. */
	DMC_UNRAR_FILE5_PROPERTY_TIME        = 0x03, /**< File time. */
	DMC_UNRAR_FILE5_PROPERTY_VERSION     = 0x04, /**< File version. */
	DMC_UNRAR_FILE5_PROPERTY_LINK        = 0x05, /**< Hard/symbolic link. */
	DMC_UNRAR_FILE5_PROPERTY_POSIXPERM   = 0x06, /**< POSIX owner, group, permissions. */
	DMC_UNRAR_FILE5_PROPERTY_SERVICEDATA = 0x07  /**< Service data. */
};

/** The general compression method (from worst to best). */
enum {
	DMC_UNRAR_METHOD_STORE   = 0x30, /**< Uncompressed. */
	DMC_UNRAR_METHOD_FASTEST = 0x31,
	DMC_UNRAR_METHOD_FAST    = 0x32,
	DMC_UNRAR_METHOD_NORMAL  = 0x33,
	DMC_UNRAR_METHOD_GOOD    = 0x34,
	DMC_UNRAR_METHOD_BEST    = 0x35
};

typedef struct dmc_unrar_block_header_tag {
	uint64_t start_pos; /**< The offset within the file the block start at. */
	uint64_t extra_pos; /**< The offset within the file the extra header data starts. */

	uint64_t type; /**< The type of the block. */

	uint32_t crc;   /**< Checksum. */
	uint64_t flags; /**< flags describing this block. */

	uint64_t header_size; /**< Size of the full block header in bytes. */
	uint64_t data_size;   /**< Size of the extra block data in bytes. */

	uint64_t extra_size; /** Size of extra file properties in RAR5, in bytes. */

} dmc_unrar_block_header;

struct dmc_unrar_file_block_tag;
typedef struct dmc_unrar_file_block_tag dmc_unrar_file_block;

struct dmc_unrar_file_block_tag {
	size_t index; /** The index of this file within the files array. */

	uint64_t start_pos; /**< The offset within the file *after* the whole file block header. */

	uint64_t flags; /**< flags describing the file. */

	uint16_t version; /**< RAR compression version for this file. */
	uint8_t  method;  /**< RAR compression method for this file. */

	uint64_t name_offset; /**< Offset to the name field. */
	uint64_t name_size;   /**< Size of the name field. */

	bool is_split;     /**< This file is a split file. */
	bool is_solid;     /**< This is a solid file. */
	bool is_link;      /**< This file is hard or symbolic link. */
	bool is_encrypted; /**< This file is encrypted. */

	uint64_t dict_size; /**< Dictionary size in bytes. */

	dmc_unrar_file_block *solid_start; /**< The first file entry in a solid block. */
	dmc_unrar_file_block *solid_prev;  /**< The previous file entry in a solid block. */
	dmc_unrar_file_block *solid_next;  /**< The next file entry in a solid block. */

	dmc_unrar_file file; /**< Public file structure. */
};

struct dmc_unrar_rar_context_tag;
typedef struct dmc_unrar_rar_context_tag dmc_unrar_rar_context;

struct dmc_unrar_internal_state_tag {
	/** RAR generation. RAR4 (1.5 - 3.6) vs RAR5 (5.0). */
	dmc_unrar_generation generation;

	uint16_t archive_flags; /**< Global archive flags. */

	dmc_unrar_block_header *comment; /**< Archive comments block. */

	size_t block_count;             /**< Number of blocks in this RAR archive. */
	dmc_unrar_block_header *blocks; /**< All blocks in this RAR archive. */
	size_t block_capacity;          /**< Memory capacity of the blocks array. */

	size_t file_count;           /**< Number of files (and directories) in this RAR archive. */
	dmc_unrar_file_block *files; /**< All files (and directories) in this RAR archive. */
	size_t file_capacity;        /**< Memory capacity of the files array. */

	/** Saved unpack context, for sequential solid block unpacking. */
	dmc_unrar_rar_context *unpack_context;
};

/* .--- Default allocation functions using malloc/realloc/free */
static void *dmc_unrar_def_alloc_func(void *opaque, size_t items, size_t size) {
	(void)opaque; (void)items; (void)size;

	return DMC_UNRAR_MALLOC(items * size);
}

static void *dmc_unrar_def_realloc_func(void *opaque, void *address, size_t items, size_t size) {
	(void)opaque; (void)address; (void)items; (void)size;
	return DMC_UNRAR_REALLOC(address, items * size);
}

static void dmc_unrar_def_free_func(void *opaque, void *address) {
	(void)opaque; (void)address;
	DMC_UNRAR_FREE(address);
}
/* '--- */

/* .--- Convenience allocation functions */
static void *dmc_unrar_malloc(dmc_unrar_alloc *alloc, size_t items, size_t size) {
	DMC_UNRAR_ASSERT(alloc && alloc->func_alloc);

	return alloc->func_alloc(alloc->opaque, items, size);
}

static void *dmc_unrar_realloc(dmc_unrar_alloc *alloc, void *address, size_t items, size_t size) {
	DMC_UNRAR_ASSERT(alloc && alloc->func_realloc);

	return alloc->func_realloc(alloc->opaque, address, items, size);
}

static void dmc_unrar_free(dmc_unrar_alloc *alloc, void *address) {
	DMC_UNRAR_ASSERT(alloc && alloc->func_free);

	alloc->func_free(alloc->opaque, address);
}
/* '--- */

/* .--- Memory IO functions */
typedef struct dmc_unrar_mem_reader_tag {
	const uint8_t *buffer;
	uint64_t size;
	uint64_t offset;
} dmc_unrar_mem_reader;

static size_t dmc_unrar_mem_read_func(void *opaque, void *buffer, size_t n) {
	if (!opaque)
		return 0;

	{
		dmc_unrar_mem_reader *mem = (dmc_unrar_mem_reader *)opaque;

		n = DMC_UNRAR_MIN(n, mem->size - mem->offset);

		memcpy(buffer, mem->buffer + mem->offset, n);

		mem->offset += n;
	}

	return n;
}

static int dmc_unrar_mem_seek_func(void *opaque, uint64_t offset) {
	if (!opaque)
		return -1;

	{
		dmc_unrar_mem_reader *mem = (dmc_unrar_mem_reader *)opaque;
		if (offset > mem->size)
			return -1;

		mem->offset = offset;
	}

	return 0;
}

static void dmc_unrar_io_init_mem_reader(dmc_unrar_io *io, dmc_unrar_mem_reader *mem_reader,
		const void *mem, size_t size) {

	DMC_UNRAR_ASSERT(io && mem_reader && mem);

	mem_reader->buffer = (const uint8_t *)mem;
	mem_reader->size   = size;
	mem_reader->offset = 0;

	io->func_read = &dmc_unrar_mem_read_func;
	io->func_seek = &dmc_unrar_mem_seek_func;
	io->opaque    = mem_reader;
}

static bool dmc_unrar_io_is_mem_reader(dmc_unrar_io *io) {
	if (!io)
		return false;

	return io->opaque &&
	       (io->func_read == &dmc_unrar_mem_read_func) &&
	       (io->func_seek == &dmc_unrar_mem_seek_func);
}
/* '--- */

/* .--- File IO functions */
#if DMC_UNRAR_DISABLE_STDIO != 1
typedef struct dmc_unrar_file_reader_tag {
	FILE *file;
	uint64_t size;

	bool need_close;
} dmc_unrar_file_reader;

static FILE *dmc_unrar_file_fopen(const char *path) {
	return fopen(path, "rb");
}

static uint64_t dmc_unrar_file_get_size(FILE *file) {
	long pos = ftell(file), size;

	if (pos == -1)
		return 0;

	if (fseek(file, 0, SEEK_END) != 0)
		return 0;

	size = ftell(file);
	if (size == -1)
		return 0;

	if (fseek(file, pos, SEEK_SET) != 0)
		return 0;

	return (uint64_t)size;
}

static dmc_unrar_return dmc_unrar_file_set(dmc_unrar_file_reader *reader, FILE *file) {
	DMC_UNRAR_ASSERT(reader && file);

	reader->file       = file;
	reader->size       = dmc_unrar_file_get_size(file);
	reader->need_close = false;

	return DMC_UNRAR_OK;
}

static dmc_unrar_return dmc_unrar_file_open(dmc_unrar_file_reader *reader, const char *path) {
	DMC_UNRAR_ASSERT(reader && path);

	if (!(reader->file = dmc_unrar_file_fopen(path)))
		return DMC_UNRAR_OPEN_FAIL;

	reader->size       = dmc_unrar_file_get_size(reader->file);
	reader->need_close = true;

	return DMC_UNRAR_OK;
}

static void dmc_unrar_file_close(dmc_unrar_file_reader *reader) {
	if (!reader)
		return;

	if (reader->need_close)
		fclose(reader->file);

	reader->file       = NULL;
	reader->need_close = false;
}

static size_t dmc_unrar_file_read_func(void *opaque, void *buffer, size_t n) {
	if (!opaque)
		return 0;

	{
		dmc_unrar_file_reader *file = (dmc_unrar_file_reader *)opaque;

		n = fread(buffer, 1, n, file->file);
	}

	return n;
}

static int dmc_unrar_file_seek_func(void *opaque, uint64_t offset) {
	if (!opaque)
		return -1;

	{
		dmc_unrar_file_reader *file = (dmc_unrar_file_reader *)opaque;

		return fseek(file->file, offset, SEEK_SET);
	}
}

static void dmc_unrar_io_init_file_reader(dmc_unrar_io *io, dmc_unrar_file_reader *file_reader) {
	DMC_UNRAR_ASSERT(io && file_reader && file_reader->file);

	io->func_read = &dmc_unrar_file_read_func;
	io->func_seek = &dmc_unrar_file_seek_func;
	io->opaque    = file_reader;

	io->offset = 0;
	io->size   = file_reader->size;
}

static bool dmc_unrar_io_is_file_reader(dmc_unrar_io *io) {
	if (!io)
		return false;

	return io->opaque &&
	       (io->func_read == &dmc_unrar_file_read_func) &&
	       (io->func_seek == &dmc_unrar_file_seek_func);
}
#endif /* DMC_UNRAR_DISABLE_STDIO */
/* '--- */

/* .--- Convenience IO functions */
static uint16_t dmc_unrar_get_uint16le(const uint8_t *data) {
	return (((uint16_t)data[1]) << 8) |
	                   data[0];
}

static uint32_t dmc_unrar_get_uint32le(const uint8_t *data) {
	return (((uint32_t)data[3]) << 24) |
	       (((uint32_t)data[2]) << 16) |
	       (((uint32_t)data[1]) <<  8) |
	                   data[0];
}

static bool dmc_unrar_archive_seek(dmc_unrar_io *io, uint64_t offset) {
	bool result;
	DMC_UNRAR_ASSERT(io);

	result = io->func_seek(io->opaque, offset) == 0;
	if (result)
		io->offset = offset;

	return result;
}

static size_t dmc_unrar_archive_read(dmc_unrar_io *io, void *buffer, size_t n) {
	size_t result;
	DMC_UNRAR_ASSERT(io);

	n = DMC_UNRAR_MIN((uint64_t)(io->size - io->offset), (uint64_t)n);

	result = io->func_read(io->opaque, buffer, n);

	io->offset += result;
	return result;
}

static bool dmc_unrar_archive_read_checked(dmc_unrar_io *io, void *buffer, size_t n) {
	return dmc_unrar_archive_read(io, buffer, n) == n;
}

static bool dmc_unrar_archive_read_uint8(dmc_unrar_io *io, uint8_t *value) {
	return dmc_unrar_archive_read_checked(io, value, 1);
}

static bool dmc_unrar_archive_read_uint16le(dmc_unrar_io *io, uint16_t *value) {
	uint8_t data[2];

	if (!dmc_unrar_archive_read_checked(io, data, 2))
		return false;

	*value = dmc_unrar_get_uint16le(data);
	return true;
}

static bool dmc_unrar_archive_read_uint32le(dmc_unrar_io *io, uint32_t *value) {
	uint8_t data[4];

	if (!dmc_unrar_archive_read_checked(io, data, 4))
		return false;

	*value = dmc_unrar_get_uint32le(data);
	return true;
}
/* '--- */

/* .--- Common RAR unpacking functions, interface */
static dmc_unrar_rar_context *dmc_unrar_rar_context_alloc(dmc_unrar_alloc *alloc);

static dmc_unrar_return dmc_unrar_rar_context_create(dmc_unrar_rar_context *ctx,
	dmc_unrar_archive *archive, dmc_unrar_file_block *file);

static void dmc_unrar_rar_context_destroy(dmc_unrar_rar_context *ctx);

static dmc_unrar_return dmc_unrar_rar_context_init(dmc_unrar_rar_context *ctx,
	dmc_unrar_archive *archive, dmc_unrar_file_block *file, void *buffer, size_t buffer_size);

static bool dmc_unrar_rar_context_check(dmc_unrar_rar_context *ctx,
	dmc_unrar_archive *archive, dmc_unrar_file_block *file, dmc_unrar_return *return_code);

static bool dmc_unrar_rar_context_file_match(dmc_unrar_rar_context *ctx,
	dmc_unrar_file_block *f1, dmc_unrar_file_block *f2);

static dmc_unrar_return dmc_unrar_rar_context_unpack(dmc_unrar_rar_context *ctx,
	dmc_unrar_archive *archive, dmc_unrar_file_block *file,
	void *buffer, size_t buffer_size, size_t *uncompressed_size, uint32_t *crc,
	void *opaque, dmc_unrar_extract_callback_func callback);
/* '--- */

/* .--- Opening/closing a RAR archive */
static int dmc_unrar_identify_generation(dmc_unrar_io *io);
static dmc_unrar_return dmc_unrar_archive_open_internal(dmc_unrar_archive *archive);

bool dmc_unrar_is_rar(dmc_unrar_io *io) {
	int generation;

	if (!io)
		return false;

	generation = dmc_unrar_identify_generation(io);
	return generation > (int)DMC_UNRAR_GENERATION_INVALID;
}

bool dmc_unrar_is_rar_mem(const void *mem, size_t size) {
	dmc_unrar_mem_reader mem_reader;
	dmc_unrar_io io;

	if (!mem || !size)
		return false;

	dmc_unrar_io_init_mem_reader(&io, &mem_reader, mem, size);

	return dmc_unrar_is_rar(&io);
}

#if DMC_UNRAR_DISABLE_STDIO != 1
bool dmc_unrar_is_rar_file(FILE *file) {
	dmc_unrar_file_reader file_reader;
	dmc_unrar_io io;
	bool result;

	if (!file)
		return false;

	if (dmc_unrar_file_set(&file_reader, file) != DMC_UNRAR_OK)
		return false;

	dmc_unrar_io_init_file_reader(&io, &file_reader);

	result = dmc_unrar_is_rar(&io);

	dmc_unrar_file_close(&file_reader);
	return result;
}

bool dmc_unrar_is_rar_path(const char *path) {
	dmc_unrar_file_reader file_reader;
	dmc_unrar_io io;
	bool result;

	if (!path)
		return false;

	if (dmc_unrar_file_open(&file_reader, path) != DMC_UNRAR_OK)
		return false;

	dmc_unrar_io_init_file_reader(&io, &file_reader);

	result = dmc_unrar_is_rar(&io);

	dmc_unrar_file_close(&file_reader);
	return result;
}
#endif /* DMC_UNRAR_DISABLE_STDIO */

dmc_unrar_return dmc_unrar_archive_init(dmc_unrar_archive *archive) {
	if (!archive)
		return DMC_UNRAR_ARCHIVE_IS_NULL;

	DMC_UNRAR_CLEAR_OBJ(*archive);

	return DMC_UNRAR_OK;
}

/** Initialize and validate the allocation callbacks. */
static dmc_unrar_return dmc_unrar_archive_check_alloc(dmc_unrar_alloc *alloc) {
	DMC_UNRAR_ASSERT(alloc);

	/* Our own, standard allocators don't use the opaque_mem value. */
	if (!alloc->func_alloc && !alloc->func_realloc && !alloc->func_free && alloc->opaque)
		return DMC_UNRAR_ARCHIVE_NOT_CLEARED;

#if DMC_UNRAR_DISABLE_MALLOC == 1
	/* If we're compiling without malloc, we *need* to pass allocators. */
	if (!alloc->func_alloc || !alloc->func_realloc || !alloc->func_free)
		return DMC_UNRAR_NO_ALLOC;
#endif

	/* Set default allocators. */
	if (!alloc->func_alloc)
		alloc->func_alloc = &dmc_unrar_def_alloc_func;
	if (!alloc->func_realloc)
		alloc->func_realloc = &dmc_unrar_def_realloc_func;
	if (!alloc->func_free)
		alloc->func_free = &dmc_unrar_def_free_func;

	return DMC_UNRAR_OK;
}

dmc_unrar_return dmc_unrar_archive_open(dmc_unrar_archive *archive, uint64_t size) {
	if (!archive)
		return DMC_UNRAR_ARCHIVE_IS_NULL;

	/* This *needs* to be cleared. */
	if (archive->internal_state)
		return DMC_UNRAR_ARCHIVE_NOT_CLEARED;

	/* These *need* to be set. */
	if (!archive->io.func_read || !archive->io.func_seek || !archive->io.opaque)
		return DMC_UNRAR_ARCHIVE_MISSING_FIELDS;

	archive->io.offset = 0;
	archive->io.size   = size;

	/* Initialize allocators. */
	{
		const dmc_unrar_return alloc_check = dmc_unrar_archive_check_alloc(&archive->alloc);
		if (alloc_check != DMC_UNRAR_OK)
			return alloc_check;
	}

	/* And pass to the reader function that does the actual work. */
	{
		const dmc_unrar_return open_archive = dmc_unrar_archive_open_internal(archive);
		if (open_archive != DMC_UNRAR_OK) {
			dmc_unrar_archive_close(archive);
			return open_archive;
		}
	}

	return DMC_UNRAR_OK;
}

dmc_unrar_return dmc_unrar_archive_open_mem(dmc_unrar_archive *archive,
		const void *mem, size_t size) {

	/* Sanity checks. */
	if (!archive)
		return DMC_UNRAR_ARCHIVE_IS_NULL;
	if (!mem || !size)
		return DMC_UNRAR_ARCHIVE_EMPTY;

	/* Initialize allocators. */
	{
		const dmc_unrar_return alloc_check = dmc_unrar_archive_check_alloc(&archive->alloc);
		if (alloc_check != DMC_UNRAR_OK)
			return alloc_check;
	}

	/* Allocate and initialize a simple memory reader. */
	{
		dmc_unrar_mem_reader *mem_reader = (dmc_unrar_mem_reader *)
			dmc_unrar_malloc(&archive->alloc, 1, sizeof(dmc_unrar_mem_reader));

		if (!mem_reader)
			return DMC_UNRAR_ALLOC_FAIL;

		dmc_unrar_io_init_mem_reader(&archive->io, mem_reader, mem, size);
	}

	/* Pass to the generic open function. */
	{
		const dmc_unrar_return open_archive = dmc_unrar_archive_open(archive, size);
		if (open_archive != DMC_UNRAR_OK) {
			dmc_unrar_archive_close(archive);
			return open_archive;
		}
	}

	return DMC_UNRAR_OK;
}

#if DMC_UNRAR_DISABLE_STDIO != 1
static dmc_unrar_return dmc_unrar_archive_open_file_reader(dmc_unrar_archive *archive,
		dmc_unrar_file_reader *file) {

	DMC_UNRAR_ASSERT(archive && file);

	/* Initialize allocators. */
	{
		const dmc_unrar_return alloc_check = dmc_unrar_archive_check_alloc(&archive->alloc);
		if (alloc_check != DMC_UNRAR_OK) {
			return alloc_check;
		}
	}

	/* Allocate and initialize a simple file reader. */
	{
		dmc_unrar_file_reader *file_reader = (dmc_unrar_file_reader *)
			dmc_unrar_malloc(&archive->alloc, 1, sizeof(dmc_unrar_file_reader));

		if (!file_reader) {
			dmc_unrar_file_close(file);
			return DMC_UNRAR_ALLOC_FAIL;
		}

		*file_reader = *file;
		DMC_UNRAR_CLEAR_OBJ(*file);

		dmc_unrar_io_init_file_reader(&archive->io, file_reader);

		/* Pass to the generic open function. */
		{
			const dmc_unrar_return open_archive = dmc_unrar_archive_open(archive, file_reader->size);
			if (open_archive != DMC_UNRAR_OK) {
				dmc_unrar_archive_close(archive);
				return open_archive;
			}
		}
	}

	return DMC_UNRAR_OK;
}

dmc_unrar_return dmc_unrar_archive_open_file(dmc_unrar_archive *archive, FILE *file) {
	dmc_unrar_file_reader file_reader;

	/* Sanity checks. */
	if (!archive)
		return DMC_UNRAR_ARCHIVE_IS_NULL;
	if (!file)
		return DMC_UNRAR_ARCHIVE_EMPTY;

	{
		dmc_unrar_return file_open = dmc_unrar_file_set(&file_reader, file);
		if (file_open != DMC_UNRAR_OK)
			return file_open;
	}

	return dmc_unrar_archive_open_file_reader(archive, &file_reader);
}

dmc_unrar_return dmc_unrar_archive_open_path(dmc_unrar_archive *archive, const char *path) {
	dmc_unrar_file_reader file_reader;

	/* Sanity checks. */
	if (!archive)
		return DMC_UNRAR_ARCHIVE_IS_NULL;
	if (!path)
		return DMC_UNRAR_ARCHIVE_EMPTY;

	{
		dmc_unrar_return file_open = dmc_unrar_file_open(&file_reader, path);
		if (file_open != DMC_UNRAR_OK)
			return file_open;
	}

	return dmc_unrar_archive_open_file_reader(archive, &file_reader);
}
#endif /* DMC_UNRAR_DISABLE_STDIO */

void dmc_unrar_archive_close(dmc_unrar_archive *archive) {
	if (!archive)
		return;

	/* If we have no deallocator, there's nothing we can do except clear the context. */
	if (!archive->alloc.func_free) {
		DMC_UNRAR_CLEAR_OBJ(*archive);
		return;
	}

#if DMC_UNRAR_DISABLE_STDIO != 1
	/* If we're using our own file reader, clean and free its context. */
	if (dmc_unrar_io_is_file_reader(&archive->io)) {
		dmc_unrar_file_close((dmc_unrar_file_reader *)archive->io.opaque);
		dmc_unrar_free(&archive->alloc, archive->io.opaque);
		DMC_UNRAR_CLEAR_OBJ(archive->io);
	}
#endif /* DMC_UNRAR_DISABLE_STDIO */

	/* If we're using our own memory reader, clean and free its context. */
	if (dmc_unrar_io_is_mem_reader(&archive->io)) {
		dmc_unrar_free(&archive->alloc, archive->io.opaque);
		DMC_UNRAR_CLEAR_OBJ(archive->io);
	}

	/* Deallocate the internal state. */
	if (archive->internal_state) {

		/* Destroy the saved unpack context, if we have one. */
		dmc_unrar_rar_context_destroy(archive->internal_state->unpack_context);

		dmc_unrar_free(&archive->alloc, archive->internal_state->unpack_context);

		dmc_unrar_free(&archive->alloc, archive->internal_state->blocks);
		dmc_unrar_free(&archive->alloc, archive->internal_state->files);

		dmc_unrar_free(&archive->alloc, archive->internal_state);
	}

	DMC_UNRAR_CLEAR_OBJ(*archive);
}

static bool dmc_unrar_init_internal_blocks(dmc_unrar_archive *archive);
static bool dmc_unrar_init_internal_files(dmc_unrar_archive *archive);

static dmc_unrar_return dmc_unrar_rar4_collect_blocks(dmc_unrar_archive *archive);
static dmc_unrar_return dmc_unrar_rar5_collect_blocks(dmc_unrar_archive *archive);

/** Internal entry function that actually reads the RAR. */
static dmc_unrar_return dmc_unrar_archive_open_internal(dmc_unrar_archive *archive) {
	DMC_UNRAR_ASSERT(archive);

	if (archive->io.size == 0)
		return DMC_UNRAR_ARCHIVE_EMPTY;

	/* Initialize our internal state. */

	archive->internal_state = (dmc_unrar_internal_state *)
		dmc_unrar_malloc(&archive->alloc, 1, sizeof(dmc_unrar_internal_state));

	if (!archive->internal_state)
		return DMC_UNRAR_ALLOC_FAIL;

	DMC_UNRAR_CLEAR_OBJ(*archive->internal_state);

	archive->internal_state->unpack_context = dmc_unrar_rar_context_alloc(&archive->alloc);

	if (!archive->internal_state->unpack_context)
		return DMC_UNRAR_ALLOC_FAIL;

	/* Identify the RAR generation (RAR4? RAR5?). */
	{
		int generation = dmc_unrar_identify_generation(&archive->io);
		if (generation < 0)
			return (dmc_unrar_return)-generation;

		archive->internal_state->generation = (dmc_unrar_generation)generation;
	}

	/* Not a RAR file?. */
	if (archive->internal_state->generation == DMC_UNRAR_GENERATION_INVALID)
		return DMC_UNRAR_ARCHIVE_NOT_RAR;

	/* Ancient RAR 1.3. We don't support it. */
	if (archive->internal_state->generation == DMC_UNRAR_GENERATION_ANCIENT)
		return DMC_UNRAR_ARCHIVE_UNSUPPORTED_ANCIENT;

	DMC_UNRAR_ASSERT((archive->internal_state->generation == DMC_UNRAR_GENERATION_RAR4) ||
	                 (archive->internal_state->generation == DMC_UNRAR_GENERATION_RAR5));

	/* Initialize the block and file arrays. */
	if (!dmc_unrar_init_internal_blocks(archive))
		return DMC_UNRAR_ALLOC_FAIL;
	if (!dmc_unrar_init_internal_files(archive))
		return DMC_UNRAR_ALLOC_FAIL;

	/* And run through the archive to collect all blocks and files. */
	if (archive->internal_state->generation == DMC_UNRAR_GENERATION_RAR4) {
		const dmc_unrar_return collect_blocks = dmc_unrar_rar4_collect_blocks(archive);
		if (collect_blocks != DMC_UNRAR_OK)
			return collect_blocks;

	} else {
		const dmc_unrar_return collect_blocks = dmc_unrar_rar5_collect_blocks(archive);
		if (collect_blocks != DMC_UNRAR_OK)
			return collect_blocks;
	}

	return DMC_UNRAR_OK;
}
/* '--- */

/* .--- RAR generation */
static void *dmc_unrar_memmem(const void *haystack, size_t haystack_size,
                              const void *needle, size_t needle_size) {

	char *cur, *last;
	const char *haystack_char = (const char *)haystack;
	const char *needle_char   = (const char *)needle;

	/* We need something to compare. */
	if (haystack_size == 0 || needle_size == 0)
		return NULL;

	/* "needle" must be smaller or equal to "haystack". */
	if (haystack_size < needle_size)
		return NULL;

	/* Special case where needle_size == 1. */
	if (needle_size == 1)
		return memchr((void *)haystack, (int)*needle_char, haystack_size);

	/* The last position where it's possible to find "needle" in "haystack". */
	last = (char *)haystack_char + haystack_size - needle_size;

	for (cur = (char *)haystack_char; cur <= last; cur++)
		if (cur[0] == needle_char[0] && memcmp(cur, needle_char, needle_size) == 0)
			return cur;

	return NULL;
}

typedef struct dmc_unrar_magic_tag {
	const void *magic;
	size_t size;
	dmc_unrar_generation gen;

} dmc_unrar_magic;

static dmc_unrar_generation dmc_unrar_find_generation(uint8_t *buffer, size_t buffer_size, size_t *offset) {
	static const uint8_t DMC_UNRAR_MAGIC_13[] = { 0x52, 0x45, 0x7E, 0x5E };
	static const uint8_t DMC_UNRAR_MAGIC_15[] = { 'R', 'a', 'r', '!', 0x1A, 0x07, 0x00 };
	static const uint8_t DMC_UNRAR_MAGIC_50[] = { 'R', 'a', 'r', '!', 0x1A, 0x07, 0x01, 0x00 };

	static const dmc_unrar_magic DMC_UNRAR_MAGICS[] = {
		{ DMC_UNRAR_MAGIC_50, DMC_UNRAR_ARRAYSIZE(DMC_UNRAR_MAGIC_50), DMC_UNRAR_GENERATION_RAR5 },
		{ DMC_UNRAR_MAGIC_15, DMC_UNRAR_ARRAYSIZE(DMC_UNRAR_MAGIC_15), DMC_UNRAR_GENERATION_RAR4 },
		{ DMC_UNRAR_MAGIC_13, DMC_UNRAR_ARRAYSIZE(DMC_UNRAR_MAGIC_13), DMC_UNRAR_GENERATION_ANCIENT }
	};

	size_t i;
	for (i = 0; i < DMC_UNRAR_ARRAYSIZE(DMC_UNRAR_MAGICS); i++) {
		const uint8_t *found = (const uint8_t *)dmc_unrar_memmem(buffer, buffer_size,
		                       DMC_UNRAR_MAGICS[i].magic, DMC_UNRAR_MAGICS[i].size);

		if (found) {
			*offset = (found - buffer) + DMC_UNRAR_MAGICS[i].size;

			return DMC_UNRAR_MAGICS[i].gen;
		}
	}

	return DMC_UNRAR_GENERATION_INVALID;
}

/** Identify a RAR file generation by its magic number. */
static int dmc_unrar_identify_generation(dmc_unrar_io *io) {
	size_t buffer_size, read_count;
	uint8_t buffer[4096];

	DMC_UNRAR_ASSERT(io);

	if (!dmc_unrar_archive_seek(io, 0))
		return -DMC_UNRAR_SEEK_FAIL;

	read_count  = dmc_unrar_archive_read(io, buffer, 8);
	buffer_size = read_count;

	while (read_count != 0) {
		size_t offset;
		dmc_unrar_generation gen = dmc_unrar_find_generation(buffer, buffer_size, &offset);

		if (gen != DMC_UNRAR_GENERATION_INVALID) {
			if (!dmc_unrar_archive_seek(io, io->offset + offset - buffer_size))
				return -DMC_UNRAR_SEEK_FAIL;

			return gen;
		}

		{
			size_t to_copy = DMC_UNRAR_MIN(8, buffer_size);
			memmove(buffer, buffer + buffer_size - to_copy, to_copy);

			buffer_size = to_copy;

			read_count   = dmc_unrar_archive_read(io, buffer + buffer_size,
			               DMC_UNRAR_ARRAYSIZE(buffer) - buffer_size);

			buffer_size += read_count;
		}
	}

	return DMC_UNRAR_GENERATION_INVALID;
}
/* '--- */

/* .--- Managing the block/file arrays */
/** Initialize the blocks array with a starting capacity. */
static bool dmc_unrar_init_internal_blocks(dmc_unrar_archive *archive) {
	archive->internal_state->block_count    = 0;
	archive->internal_state->block_capacity = DMC_UNRAR_ARRAY_INITIAL_CAPACITY;

	archive->internal_state->blocks = (dmc_unrar_block_header *)
		dmc_unrar_malloc(&archive->alloc, archive->internal_state->block_capacity,
		                 sizeof(dmc_unrar_block_header));

	return archive->internal_state->blocks != NULL;
}

/** Make sure we have enough space in the blocks array for one more block. */
static bool dmc_unrar_ensure_block_capacity(dmc_unrar_archive *archive) {
	if (archive->internal_state->block_count < archive->internal_state->block_capacity)
		return true;

	archive->internal_state->block_capacity =
		DMC_UNRAR_MAX(archive->internal_state->block_capacity, 1);

	archive->internal_state->block_capacity *= 2;
	archive->internal_state->blocks = (dmc_unrar_block_header *)
		dmc_unrar_realloc(&archive->alloc, archive->internal_state->blocks,
		                  archive->internal_state->block_capacity, sizeof(dmc_unrar_block_header));

	return archive->internal_state->blocks != NULL;
}

/** Add one more element to the blocks array. */
static bool dmc_unrar_grow_blocks(dmc_unrar_archive *archive) {
	if (!dmc_unrar_ensure_block_capacity(archive))
		return false;

	archive->internal_state->block_count++;
	return true;
}

/** Initialize the files array with a starting capacity. */
static bool dmc_unrar_init_internal_files(dmc_unrar_archive *archive) {
	archive->internal_state->file_count    = 0;
	archive->internal_state->file_capacity = DMC_UNRAR_ARRAY_INITIAL_CAPACITY;

	archive->internal_state->files = (dmc_unrar_file_block *)
		dmc_unrar_malloc(&archive->alloc, archive->internal_state->file_capacity,
		                 sizeof(dmc_unrar_file_block));

	return archive->internal_state->files != NULL;
}

/** Make sure we have enough space in the files array for one more files. */
static bool dmc_unrar_ensure_file_capacity(dmc_unrar_archive *archive) {
	if (archive->internal_state->file_count < archive->internal_state->file_capacity)
		return true;

	archive->internal_state->file_capacity = DMC_UNRAR_MAX(archive->internal_state->file_capacity, 1);

	archive->internal_state->file_capacity *= 2;
	archive->internal_state->files = (dmc_unrar_file_block *)
		dmc_unrar_realloc(&archive->alloc, archive->internal_state->files,
		                  archive->internal_state->file_capacity, sizeof(dmc_unrar_file_block));

	return archive->internal_state->files != NULL;
}

/** Add one more element to the files array. */
static bool dmc_unrar_grow_files(dmc_unrar_archive *archive) {
	if (!dmc_unrar_ensure_file_capacity(archive))
		return false;

	archive->internal_state->file_count++;
	return true;
}
/* '--- */

/* .--- Reading RAR blocks and file headers */
static dmc_unrar_return dmc_unrar_rar4_read_block_header(dmc_unrar_archive *archive,
	dmc_unrar_block_header *block);
static dmc_unrar_return dmc_unrar_rar4_read_archive_header(dmc_unrar_archive *archive,
	dmc_unrar_block_header *block);
static dmc_unrar_return dmc_unrar_rar4_read_archive_sub(dmc_unrar_archive *archive,
	dmc_unrar_block_header *block);
static dmc_unrar_return dmc_unrar_rar4_read_file_header(dmc_unrar_archive *archive,
	dmc_unrar_block_header *block, dmc_unrar_file_block *file, bool modify_block);

/** Connect the file entries in solid blocks together. */
static void dmc_unrar_connect_solid(dmc_unrar_archive *archive) {
	dmc_unrar_internal_state *state = archive->internal_state;
	dmc_unrar_file_block *file = NULL, *start = NULL, *prev = NULL;

	size_t i;
	for (i = 0, file = state->files; i < state->file_count; i++, file++) {
		if (!file->is_solid) {
			file->solid_start = file;
			file->solid_prev  = NULL;
			file->solid_next  = NULL;

			start = file;
			prev  = file;

			continue;
		}

		file->solid_start = start;
		file->solid_prev  = prev;
		file->solid_next  = NULL;

		if (prev)
			prev->solid_next = file;

		prev = file;
	}
}

/** Run through the archive and collect all blocks (and files) in a RAR4 archive. */
static dmc_unrar_return dmc_unrar_rar4_collect_blocks(dmc_unrar_archive *archive) {
	dmc_unrar_internal_state *state = archive->internal_state;
	state->archive_flags = 0;

	while (archive->io.offset < archive->io.size) {
		/* One more block. */
		if (!dmc_unrar_grow_blocks(archive))
			return DMC_UNRAR_ALLOC_FAIL;

		{
			dmc_unrar_block_header *block = &state->blocks[state->block_count - 1];

			/* Read the block. */
			const dmc_unrar_return read_block = dmc_unrar_rar4_read_block_header(archive, block);
			if (read_block != DMC_UNRAR_OK)
				return read_block;

			/* It's an ending marker, so we're done. */
			if (block->type == DMC_UNRAR_BLOCK4_TYPE_END)
				break;

			/* It's an archive information header. */
			if (block->type == DMC_UNRAR_BLOCK4_TYPE_ARCHIVEHEADER) {
				const dmc_unrar_return read_header = dmc_unrar_rar4_read_archive_header(archive, block);
				if (read_header != DMC_UNRAR_OK)
					return read_header;
			}

			/* Might contain an archive comment. */
			if (block->type == DMC_UNRAR_BLOCK4_TYPE_NEWSUB) {
				const dmc_unrar_return read_sub = dmc_unrar_rar4_read_archive_sub(archive, block);
				if (read_sub != DMC_UNRAR_OK)
					return read_sub;
			}

			/* It's a file. */
			if (block->type == DMC_UNRAR_BLOCK4_TYPE_FILE) {
				if (!dmc_unrar_grow_files(archive))
					return DMC_UNRAR_ALLOC_FAIL;

				/* Read the rest of the file header. */
				{
					dmc_unrar_file_block *file = &state->files[state->file_count - 1];

					const dmc_unrar_return read_file = dmc_unrar_rar4_read_file_header(archive, block, file, true);
					if (read_file != DMC_UNRAR_OK)
						return read_file;
				}
			}

			/* Seek past this block, so we can read the next one. */
			if (!dmc_unrar_archive_seek(&archive->io, block->start_pos + block->header_size + block->data_size))
				return DMC_UNRAR_SEEK_FAIL;
		}
	}

	dmc_unrar_connect_solid(archive);

	return DMC_UNRAR_OK;
}

/** Read a RAR4 block header. */
static dmc_unrar_return dmc_unrar_rar4_read_block_header(dmc_unrar_archive *archive,
		dmc_unrar_block_header *block) {

	uint8_t type;
	uint16_t crc, flags, header_size;

	DMC_UNRAR_ASSERT(archive && block);

	block->start_pos = archive->io.offset;

	/* TODO: Validate the checksum. */
	if (!dmc_unrar_archive_read_uint16le(&archive->io, &crc))
		return DMC_UNRAR_READ_FAIL;
	if (!dmc_unrar_archive_read_uint8(&archive->io, &type))
		return DMC_UNRAR_READ_FAIL;
	if (!dmc_unrar_archive_read_uint16le(&archive->io, &flags))
		return DMC_UNRAR_READ_FAIL;
	if (!dmc_unrar_archive_read_uint16le(&archive->io, &header_size))
		return DMC_UNRAR_READ_FAIL;

	block->type        = type;
	block->crc         = crc;
	block->flags       = flags;
	block->header_size = header_size;
	block->extra_size  = 0;

	/* We just read 7 bytes, so.... */
	if (block->header_size < 7)
		return DMC_UNRAR_INVALID_DATA;

	/* Does the block have data attached, after the header?. */
	{
		bool has_data = (block->flags & DMC_UNRAR_FLAG4_BLOCK_LONG) ||
		                (block->type == DMC_UNRAR_BLOCK4_TYPE_FILE);
		uint32_t data_size = 0;

		if (has_data)
			if (!dmc_unrar_archive_read_uint32le(&archive->io, &data_size))
				return DMC_UNRAR_READ_FAIL;

		block->data_size = data_size;
	}

	block->extra_pos = archive->io.offset;

	return DMC_UNRAR_OK;
}

static dmc_unrar_return dmc_unrar_rar4_check_archive_flags(uint16_t flags);

static dmc_unrar_return dmc_unrar_rar4_read_archive_header(dmc_unrar_archive *archive,
	dmc_unrar_block_header *block) {

	const dmc_unrar_return check_flags = dmc_unrar_rar4_check_archive_flags(block->flags);
	if (check_flags != DMC_UNRAR_OK)
		return check_flags;

	archive->internal_state->archive_flags = block->flags;

	if (!dmc_unrar_archive_seek(&archive->io, archive->io.offset + 6))
		return DMC_UNRAR_SEEK_FAIL;

	if (block->flags & DMC_UNRAR_FLAG4_ARCHIVE_ENCRYPTVERSION)
		if (!dmc_unrar_archive_seek(&archive->io, archive->io.offset + 1))
			return DMC_UNRAR_SEEK_FAIL;

	if (block->flags & DMC_UNRAR_FLAG4_ARCHIVE_HASCOMMENT)
		archive->internal_state->comment = block;

	return DMC_UNRAR_OK;
}

/** Check the RAR4 archive flags for unsupported features. */
static dmc_unrar_return dmc_unrar_rar4_check_archive_flags(uint16_t flags) {
	/* Archive split into several volumes. We don't support that. */
	if (flags & DMC_UNRAR_FLAG4_ARCHIVE_VOLUMES)
		return DMC_UNRAR_ARCHIVE_UNSUPPORTED_VOLUMES;

	/* Complete archive is encrypted. We don't support that either. */
	if (flags & DMC_UNRAR_FLAG4_ARCHIVE_ENCRYPTED)
		return DMC_UNRAR_ARCHIVE_UNSUPPORTED_ENCRYPTED;

	return DMC_UNRAR_OK;
}

static dmc_unrar_return dmc_unrar_rar4_read_archive_sub(dmc_unrar_archive *archive,
	dmc_unrar_block_header *block) {

	dmc_unrar_return return_code;
	dmc_unrar_file_block sub_block;
	char name[3];

	return_code = dmc_unrar_rar4_read_file_header(archive, block, &sub_block, false);
	if (return_code != DMC_UNRAR_OK)
		return return_code;

	if (sub_block.name_size != 3)
		return DMC_UNRAR_OK;

	if (!dmc_unrar_archive_seek(&archive->io, sub_block.name_offset))
		return DMC_UNRAR_SEEK_FAIL;

	if (!dmc_unrar_archive_read_checked(&archive->io, name, 3))
		return DMC_UNRAR_READ_FAIL;

	/* For "comment", I guess. */
	if (strncmp(name, "CMT", 3))
		return DMC_UNRAR_OK;

	/* Yup, this seems to be a comment. Store it in our archive struct. */

	archive->internal_state->comment = block;
	return DMC_UNRAR_OK;
}

static uint64_t dmc_unrar_rar4_get_dict_size(const dmc_unrar_file_block *file) {
	/* TODO: Check the window size flags? */

	switch (file->version) {
		case 15:
			return 0x10000;

		case 20:
		case 26:
			return 0x100000;

		case 29:
		case 36:
			return 0x400000;

		default:
			break;
	}

	return 0;
}

/** Explode a MS-DOS timestamp into its component parts. */
static void dmc_unrar_decode_dos_time(uint32_t dos_time,
		int *year, int *month, int *day, int *hour, int *minute, int *second) {

	*year   = (dos_time >> 25)       + 1980;
	*month  = (dos_time >> 21) & 15;
	*day    = (dos_time >> 16) & 31;
	*hour   = (dos_time >> 11) & 31;
	*minute = (dos_time >>  5) & 63;
	*second = (dos_time        & 31) *    2;
}

/** Create a POSIX timestamp from date and time. */
static uint64_t dmc_unrar_time_to_unix_time(int year, int month, int day, int hour, int minute, int second) {
	static const uint16_t days_to_month_start[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
	uint32_t years, leap_years;
	uint64_t unix_time = 0;

	if (year < 1970)
		return 0;

	years      = year - 1970;
	leap_years = ((year - 1) - 1968) / 4 - ((year - 1) - 1900) / 100 + ((year - 1) - 1600) / 400;

	unix_time += second + 60 * minute + 60 * 60 * hour;
	unix_time += (days_to_month_start[month - 1] + day - 1) * 60 * 60 * 24;
	unix_time += (years * 365 + leap_years) * 60 * 60 * 24;

	if ((month > 2) && (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)))
		unix_time += 60 * 60 * 24;

	return unix_time;
}

static bool dmc_unrar_rar_file_is_link(dmc_unrar_file_block *file) {
	if ((file->file.host_os == DMC_UNRAR_HOSTOS_DOS) || (file->file.host_os == DMC_UNRAR_HOSTOS_WIN32))
		return (file->file.attrs & DMC_UNRAR_ATTRIB_DOS_SYMLINK) != 0;

	if (file->file.host_os == DMC_UNRAR_HOSTOS_UNIX)
		return (file->file.attrs & DMC_UNRAR_ATTRIB_UNIX_FILETYPE_MASK) == DMC_UNRAR_ATTRIB_UNIX_IS_SYMBOLIC_LINK;

	return false;
}

/** Read a RAR4 file entry. */
static dmc_unrar_return dmc_unrar_rar4_read_file_header(dmc_unrar_archive *archive,
		dmc_unrar_block_header *block, dmc_unrar_file_block *file, bool modify_block) {

	DMC_UNRAR_ASSERT(archive && block && file);

	file->index = archive->internal_state->file_count - 1;

	file->start_pos = block->start_pos + block->header_size;
	file->flags     = block->flags;

	/* The compressed size is the data size read from the block header. */
	file->file.compressed_size = block->data_size;

	/* Read the uncompressed size now. */
	{
		uint32_t uncompressed_size;
		if (!dmc_unrar_archive_read_uint32le(&archive->io, &uncompressed_size))
			return DMC_UNRAR_READ_FAIL;

		file->file.uncompressed_size = uncompressed_size;
	}

	/* And read the other file properties. */

	{
		uint8_t host_os;
		if (!dmc_unrar_archive_read_uint8(&archive->io, &host_os))
			return DMC_UNRAR_READ_FAIL;

		file->file.host_os = (dmc_unrar_host_os)host_os;
	}

	file->file.has_crc = true;
	if (!dmc_unrar_archive_read_uint32le(&archive->io, &file->file.crc))
		return DMC_UNRAR_READ_FAIL;

	{
		uint32_t dos_time;
		int year, month, day, hour, minute, second;

		if (!dmc_unrar_archive_read_uint32le(&archive->io, &dos_time))
			return DMC_UNRAR_READ_FAIL;

		dmc_unrar_decode_dos_time(dos_time, &year, &month, &day, &hour, &minute, &second);
		file->file.unix_time = dmc_unrar_time_to_unix_time(year, month, day, hour, minute, second);
	}

	{
		uint8_t version;
		if (!dmc_unrar_archive_read_uint8(&archive->io, &version))
			return DMC_UNRAR_READ_FAIL;
		if (!dmc_unrar_archive_read_uint8(&archive->io, &file->method))
			return DMC_UNRAR_READ_FAIL;

		file->version = version;
	}

	{
		uint16_t name_size;
		if (!dmc_unrar_archive_read_uint16le(&archive->io, &name_size))
			return DMC_UNRAR_READ_FAIL;

		file->name_size = name_size;
	}

	{
		uint32_t attrs;
		if (!dmc_unrar_archive_read_uint32le(&archive->io, &attrs))
			return DMC_UNRAR_READ_FAIL;

		file->file.attrs = attrs;
	}

	/* If this is a larger file, read the upper 32-bits of the size values. */
	if (file->flags & DMC_UNRAR_FLAG4_FILE_LARGE) {
		uint32_t high_uncomp, high_comp;

		if (!dmc_unrar_archive_read_uint32le(&archive->io, &high_uncomp))
			return DMC_UNRAR_READ_FAIL;
		if (!dmc_unrar_archive_read_uint32le(&archive->io, &high_comp))
			return DMC_UNRAR_READ_FAIL;

		file->file.uncompressed_size += ((uint64_t)high_uncomp) << 32;
		file->file.compressed_size   += ((uint64_t)high_comp)   << 32;

		/* This of course also modifies the block data size. */
		if (modify_block)
			block->data_size = file->file.compressed_size;
	}

	/* The filename would be here now. Remember the offset. */
	file->name_offset = archive->io.offset;

	file->is_encrypted = (file->flags & DMC_UNRAR_FLAG4_FILE_ENCRYPTED) != 0;

	/* RAR 1.5 can only put all files into a single solid block, while later
	 * version can create multiple smaller blocks. */

	if (file->version < 20)
		file->is_solid = (file->index > 0) &&
		                 (archive->internal_state->archive_flags & DMC_UNRAR_FLAG4_ARCHIVE_SOLID) != 0;
	else
		file->is_solid = (file->flags & DMC_UNRAR_FLAG4_FILE_SOLID) != 0;

	file->solid_prev  = NULL;
	file->solid_next  = NULL;
	file->solid_start = NULL;

	file->is_link = dmc_unrar_rar_file_is_link(file);

	file->dict_size = dmc_unrar_rar4_get_dict_size(file);

	file->is_split = (file->flags & DMC_UNRAR_FLAG4_FILE_SPLITBEFORE) ||
	                 (file->flags & DMC_UNRAR_FLAG4_FILE_SPLITAFTER);

	return DMC_UNRAR_OK;
}

/** Read a variable-length RAR5 number. */
static bool dmc_unrar_rar5_read_number(dmc_unrar_io *io, uint64_t *number) {
	int pos;

	DMC_UNRAR_ASSERT(io && number);

	*number = 0;
	for (pos = 0; pos < 64; pos += 7) {
		uint8_t value;

		if (!dmc_unrar_archive_read_uint8(io, &value))
			return false;

		*number |= (value & 0x7F) << pos;
		if (!(value & 0x80))
			break;
	}

	return pos <= 70;
}

static dmc_unrar_return dmc_unrar_rar5_read_block_header(dmc_unrar_archive *archive,
	dmc_unrar_block_header *block);
static dmc_unrar_return dmc_unrar_rar5_read_file_header(dmc_unrar_archive *archive,
	dmc_unrar_block_header *block, dmc_unrar_file_block *file);
static dmc_unrar_return dmc_unrar_rar5_read_service_block(dmc_unrar_archive *archive,
	dmc_unrar_block_header *block);

/** Run through the archive and collect all blocks (and files) in a RAR5 archive. */
static dmc_unrar_return dmc_unrar_rar5_collect_blocks(dmc_unrar_archive *archive) {
	dmc_unrar_internal_state *state = archive->internal_state;

	while (archive->io.offset < archive->io.size) {
		/* One more block. */
		if (!dmc_unrar_grow_blocks(archive))
			return DMC_UNRAR_ALLOC_FAIL;

		{
			dmc_unrar_block_header *block = &state->blocks[state->block_count - 1];

			/* Read the block. */
			const dmc_unrar_return read_block = dmc_unrar_rar5_read_block_header(archive, block);
			if (read_block != DMC_UNRAR_OK)
				return read_block;

			/* It's an ending marker, so we're done. */
			if (block->type == DMC_UNRAR_BLOCK5_TYPE_END)
				break;

			/* It's a file. */
			if (block->type == DMC_UNRAR_BLOCK5_TYPE_FILE) {
				if (!dmc_unrar_grow_files(archive))
					return DMC_UNRAR_ALLOC_FAIL;

				/* Read the rest of the file header. */
				{
					dmc_unrar_file_block *file = &state->files[state->file_count - 1];

					const dmc_unrar_return read_file = dmc_unrar_rar5_read_file_header(archive, block, file);
					if (read_file != DMC_UNRAR_OK)
						return read_file;
				}
			}

			/* "Service" block. Might be an archive comment. */
			if (block->type == DMC_UNRAR_BLOCK5_TYPE_SERVICE) {
				const dmc_unrar_return read_service = dmc_unrar_rar5_read_service_block(archive, block);
				if (read_service != DMC_UNRAR_OK)
					return read_service;
			}

			/* Seek past this block, so we can read the next one. */
			if (!dmc_unrar_archive_seek(&archive->io, block->start_pos + block->header_size + block->data_size))
				return DMC_UNRAR_SEEK_FAIL;
		}
	}

	dmc_unrar_connect_solid(archive);

	return DMC_UNRAR_OK;
}

/** Read a RAR5 block header. */
static dmc_unrar_return dmc_unrar_rar5_read_block_header(dmc_unrar_archive *archive,
	dmc_unrar_block_header *block) {

	DMC_UNRAR_ASSERT(archive && block);

	if (!dmc_unrar_archive_read_uint32le(&archive->io, &block->crc))
		return DMC_UNRAR_READ_FAIL;
	if (!dmc_unrar_rar5_read_number(&archive->io, &block->header_size))
		return DMC_UNRAR_READ_FAIL;

	block->start_pos = archive->io.offset;

	if (!dmc_unrar_rar5_read_number(&archive->io, &block->type))
		return DMC_UNRAR_READ_FAIL;
	if (!dmc_unrar_rar5_read_number(&archive->io, &block->flags))
		return DMC_UNRAR_READ_FAIL;

	block->data_size  = 0;
	block->extra_size = 0;

	if (block->flags & DMC_UNRAR_FLAG5_BLOCK_EXTRA)
		if (!dmc_unrar_rar5_read_number(&archive->io, &block->extra_size))
			return DMC_UNRAR_READ_FAIL;

	if (block->flags & DMC_UNRAR_FLAG5_BLOCK_DATA)
		if (!dmc_unrar_rar5_read_number(&archive->io, &block->data_size))
			return DMC_UNRAR_READ_FAIL;

	block->extra_pos = archive->io.offset;

	return DMC_UNRAR_OK;
}

/** Read a RAR5 file entry. */
static dmc_unrar_return dmc_unrar_rar5_read_file_header(dmc_unrar_archive *archive,
		dmc_unrar_block_header *block, dmc_unrar_file_block *file) {

	DMC_UNRAR_ASSERT(archive && block && file);

	file->index = archive->internal_state->file_count - 1;

	file->start_pos = block->start_pos + block->header_size;

	if (!dmc_unrar_rar5_read_number(&archive->io, &file->flags))
		return DMC_UNRAR_READ_FAIL;

	/* The compressed size is the data size read from the block header. */
	file->file.compressed_size = block->data_size;

	/* Read the uncompressed size now. */
	if (!dmc_unrar_rar5_read_number(&archive->io, &file->file.uncompressed_size))
		return DMC_UNRAR_READ_FAIL;

	/* Attributes. */
	if (!dmc_unrar_rar5_read_number(&archive->io, &file->file.attrs))
		return DMC_UNRAR_READ_FAIL;

	/* File timestamp. */
	{
		uint32_t unix_time = 0;

		if (file->flags & DMC_UNRAR_FLAG5_FILE_HASTIME)
			if (!dmc_unrar_archive_read_uint32le(&archive->io, &unix_time))
				return DMC_UNRAR_READ_FAIL;

		file->file.unix_time = unix_time;
	}

	/* Checksum. */
	file->file.has_crc = (file->flags & DMC_UNRAR_FLAG5_FILE_HASCRC) != 0;
	file->file.crc     = 0;

	if (file->file.has_crc)
		if (!dmc_unrar_archive_read_uint32le(&archive->io, &file->file.crc))
			return DMC_UNRAR_READ_FAIL;

	{
		uint64_t comp_info;
		if (!dmc_unrar_rar5_read_number(&archive->io, &comp_info))
			return DMC_UNRAR_READ_FAIL;

		if (!(file->flags & DMC_UNRAR_FLAG5_FILE_ISDIRECTORY)) {
			int dict_shift = (comp_info & 0x3C00) >> 10;

			file->version   =  (comp_info & 0x03F);
			file->is_solid  = ((comp_info & 0x040) >> 6) != 0;
			file->method    =  (comp_info & 0x380) >> 7;
			file->dict_size = ((uint64_t)0x20000) << dict_shift;
		} else {
			file->version   = 0;
			file->is_solid  = false;
			file->method    = 0;
			file->dict_size = 0;
		}

		file->version += 0x5000;
		file->method  += 0x30;
	}

	file->solid_prev  = NULL;
	file->solid_next  = NULL;
	file->solid_start = NULL;

	{
		uint64_t host_os;
		if (!dmc_unrar_rar5_read_number(&archive->io, &host_os))
			return DMC_UNRAR_READ_FAIL;

		switch (host_os) {
			default:
			case 0:
				file->file.host_os = DMC_UNRAR_HOSTOS_WIN32;
				break;
			case 1:
				file->file.host_os = DMC_UNRAR_HOSTOS_UNIX;
		}
	}

	if (!dmc_unrar_rar5_read_number(&archive->io, &file->name_size))
		return DMC_UNRAR_READ_FAIL;

	/* The filename would be here now. Remember the offset. */
	file->name_offset = archive->io.offset;

	file->is_encrypted = false;
	file->is_link = dmc_unrar_rar_file_is_link(file);

	if (block->extra_size) {
		const uint64_t extra_end = block->start_pos + block->header_size;
		uint64_t pos = archive->io.offset + file->name_size;

		while (pos < extra_end) {
			uint64_t size, type;

			if (!dmc_unrar_archive_seek(&archive->io, pos))
				return DMC_UNRAR_SEEK_FAIL;

			if (!dmc_unrar_rar5_read_number(&archive->io, &size))
				return DMC_UNRAR_READ_FAIL;
			if (!dmc_unrar_rar5_read_number(&archive->io, &type))
				return DMC_UNRAR_READ_FAIL;

			switch (type) {
				case DMC_UNRAR_FILE5_PROPERTY_ENCRYPTION:
					file->is_encrypted = true;

				case DMC_UNRAR_FILE5_PROPERTY_LINK:
					file->is_link = true;

				default:
					break;
			}

			pos += size;
		}
	}

	file->is_split = (file->flags & DMC_UNRAR_FLAG5_FILE_SPLITBEFORE) ||
	                 (file->flags & DMC_UNRAR_FLAG5_FILE_SPLITAFTER);

	return DMC_UNRAR_OK;
}

static dmc_unrar_return dmc_unrar_rar5_read_service_block(dmc_unrar_archive *archive,
	dmc_unrar_block_header *block) {

	dmc_unrar_return return_code;
	dmc_unrar_file_block service_block;
	char name[3];

	return_code = dmc_unrar_rar5_read_file_header(archive, block, &service_block);
	if (return_code != DMC_UNRAR_OK)
		return return_code;

	if (service_block.name_size != 3)
		return DMC_UNRAR_OK;

	if (!dmc_unrar_archive_seek(&archive->io, service_block.name_offset))
		return DMC_UNRAR_SEEK_FAIL;

	if (!dmc_unrar_archive_read_checked(&archive->io, name, 3))
		return DMC_UNRAR_READ_FAIL;

	/* For "comment", I guess. */
	if (strncmp(name, "CMT", 3))
		return DMC_UNRAR_OK;

	/* Yup, this seems to be a comment. Store it in our archive struct. */

	archive->internal_state->comment = block;
	return DMC_UNRAR_OK;
}

/* '--- */

/* .--- Unicode data handling */
#define DMC_UNRAR_UNICODE_LEAD_SURROGATE_MIN  0xD800
#define DMC_UNRAR_UNICODE_LEAD_SURROGATE_MAX  0xDBFF
#define DMC_UNRAR_UNICODE_TRAIL_SURROGATE_MIN 0xDC00
#define DMC_UNRAR_UNICODE_TRAIL_SURROGATE_MAX 0xDFFF

#define DMC_UNRAR_UNICODE_SURROGATE_OFFSET (0x10000 - (DMC_UNRAR_UNICODE_LEAD_SURROGATE_MIN << 10) - DMC_UNRAR_UNICODE_TRAIL_SURROGATE_MIN)

#define DMC_UNRAR_UNICODE_CODEPOINT_MAX 0x0010FFFF

static bool dmc_unrar_unicode_utf16_is_lead_surrogate(uint16_t code) {
	return (code >= DMC_UNRAR_UNICODE_LEAD_SURROGATE_MIN) && (code <= DMC_UNRAR_UNICODE_LEAD_SURROGATE_MAX);
}

static bool dmc_unrar_unicode_utf16_is_trail_surrogate(uint16_t code) {
	return (code >= DMC_UNRAR_UNICODE_TRAIL_SURROGATE_MIN) && (code <= DMC_UNRAR_UNICODE_TRAIL_SURROGATE_MAX);
}

static bool dmc_unrar_unicode_utf16_is_surrogate(uint16_t code) {
	return (code >= DMC_UNRAR_UNICODE_LEAD_SURROGATE_MIN) && (code <= DMC_UNRAR_UNICODE_TRAIL_SURROGATE_MAX);
}

static bool dmc_unrar_unicode_utf32_is_valid(uint32_t code) {
	return (code <= DMC_UNRAR_UNICODE_CODEPOINT_MAX) && !dmc_unrar_unicode_utf16_is_surrogate(code);
}

static bool dmc_unrar_unicode_utf32_is_overlong(uint32_t code, size_t length) {
	if (code < 0x00080)
		return length != 1;

	if (code < 0x00800)
		return length != 2;

	if (code < 0x10000)
		return length != 3;

	return length != 4;
}

/** Return the number of octets the Unicode codepoint takes as a UTF-8 code unit. */
static size_t dmc_unrar_unicode_utf8_get_octect_count(uint32_t code) {
	if (!dmc_unrar_unicode_utf32_is_valid(code))
		return 0;

	if (code < 0x80)
		return 1;

	if (code < 0x800)
		return 2;

	if (code < 0x10000)
		return 3;

	return 4;
}

/** Determine the length of an UTF-8 sequence by its first octet. */
static size_t dmc_unrar_unicode_utf8_get_sequence_length(const uint8_t *data) {
	if (!data)
		return 0;

	if       (*data        < 0x80)
		return 1;
	else if ((*data >> 5) == 0x06)
		return 2;
	else if ((*data >> 4) == 0x0E)
		return 3;
	else if ((*data >> 3) == 0x1E)
		return 4;

	return 0;
}

/** Do we have enough space in this \0-terminated string for n UTF-8 octets? */
static bool dmc_unrar_unicode_utf8_has_space(const uint8_t *str, size_t n) {
	if (!str)
		return false;

	while (n-- > 0)
		if (!*str++)
			return false;

	return true;
}

/** Write the Unicode codepoint into the buffer, encoded as UTF-8. */
static bool dmc_unrar_unicode_utf8_put(uint8_t *data, uint32_t code) {
	if (!data || !dmc_unrar_unicode_utf32_is_valid(code))
		return false;

	if        (code < 0x80) {
			*(data++) = (uint8_t)(code);

	} else if (code < 0x800) {
			*(data++) = (uint8_t)((code >> 6)            | 0xC0);
			*(data++) = (uint8_t)((code & 0x3f)          | 0x80);

	} else if (code < 0x10000) {
			*(data++) = (uint8_t)((code >> 12)           | 0xE0);
			*(data++) = (uint8_t)(((code >> 6) & 0x3F)   | 0x80);
			*(data++) = (uint8_t)((code & 0x3F)          | 0x80);

	} else {
			*(data++) = (uint8_t)((code >> 18)           | 0xF0);
			*(data++) = (uint8_t)(((code >> 12) & 0x3F)  | 0x80);
			*(data++) = (uint8_t)(((code >> 6) & 0x3F)   | 0x80);
			*(data++) = (uint8_t)((code & 0x3F)          | 0x80);
	}

	return true;
}

/** Read a UTF-8 sequence of length bytes. */
static uint32_t dmc_unrar_unicode_utf8_get_sequence(const uint8_t *data, size_t length) {
	uint32_t codepoint = 0;

	switch (length) {
		case 1:
			codepoint = data[0];
			break;

		case 2:
			codepoint = ((data[0] << 6) & 0x7FF) +
			            ( data[1]       & 0x03F);
			break;

		case 3:
			codepoint = ((data[0] << 12) & 0xFFFF) +
			            ((data[1] <<  6) & 0x0FFF) +
			            ( data[2]        & 0x993F);
			break;

		case 4:
		codepoint = ((data[0] << 18) & 0x1FFFFF) +
		            ((data[1] << 12) & 0x03FFFF) +
		            ((data[2] <<  6) & 0x000FFF) +
		            ( data[3]        & 0x00003F);
			break;

		default:
			break;
	}

	return codepoint;
}

/* Read a UTF-8 sequence as a Unicode codepoint out of the buffer and return
 * the length of the UTF-8 sequence in bytes. */
static size_t dmc_unrar_unicode_utf8_get(const uint8_t *data, uint32_t *codepoint) {
	if (!data || !codepoint)
		return 0;

	{
		const size_t length = dmc_unrar_unicode_utf8_get_sequence_length(data);

		if (!length || !dmc_unrar_unicode_utf8_has_space(data, length))
			return 0;

		*codepoint = dmc_unrar_unicode_utf8_get_sequence(data, length);

		return length;
	}
}

/* Combine the surrogate pair into a full Unicode codepoint. */
static uint32_t dmc_unrar_unicode_combine_surrogates(uint16_t lead, uint16_t trail) {
	return (lead << 10) + trail + DMC_UNRAR_UNICODE_SURROGATE_OFFSET;
}

typedef uint16_t (*dmc_unrar_unicode_read_uint16le_func)(const void *opaque);
typedef const void *(*dmc_unrar_unicode_advance_uint16le)(const void *opaque);

static uint16_t dmc_unrar_unicode_read_uint16le_from_uint16(const void *opaque) {
	return *((const uint16_t *)opaque);
}

static const void *dmc_unrar_unicode_advance_uint16(const void *opaque) {
	return (const void *)(((const uint16_t *)opaque) + 1);
}

static uint16_t dmc_unrar_unicode_read_uint16le_from_uint8(const void *opaque) {
	return dmc_unrar_get_uint16le((const uint8_t *)opaque);
}

static const void *dmc_unrar_unicode_advance_uint8(const void *opaque) {
	return (const void *)(((const uint8_t *)opaque) + 2);
}

/** Convert a whole UTF-16 string into UTF-8. */
static bool dmc_unrar_unicode_utf16_to_utf8(const void *utf16_data, size_t utf16_size,
	char *utf8_data, size_t utf8_size, size_t *out_size,
	dmc_unrar_unicode_read_uint16le_func read_func,
	dmc_unrar_unicode_advance_uint16le advance_func) {

	size_t i;

	if (out_size)
		*out_size = 0;

	for (i = 0; i < utf16_size; i += 2, utf16_data = advance_func(utf16_data)) {
		uint32_t codepoint = read_func(utf16_data);
		size_t length;

		if (dmc_unrar_unicode_utf16_is_lead_surrogate(codepoint)) {
			uint16_t trail;

			if ((i + 2) >= utf16_size)
				/* Unpaired leading surrogate => broken data. */
				return false;

			utf16_data = advance_func(utf16_data);
			i += 2;

			trail = read_func(utf16_data);
			if (!dmc_unrar_unicode_utf16_is_trail_surrogate(trail))
				/* Unpaired leading surrogate => broken data. */
				return false;

			codepoint = dmc_unrar_unicode_combine_surrogates(codepoint, trail);

		} else if (dmc_unrar_unicode_utf16_is_trail_surrogate(codepoint))
			/* Unpaired trailing surrogate => broken data. */
			return false;

		if (!dmc_unrar_unicode_utf32_is_valid(codepoint))
		  /* Codepoint is not valid => broken data. */
			return false;

		length = dmc_unrar_unicode_utf8_get_octect_count(codepoint);
		if (utf8_size < length)
			break;

		utf8_size -= length;
		if (out_size)
			(*out_size) += length;

		if (utf8_data) {
			if (!dmc_unrar_unicode_utf8_put((uint8_t *)utf8_data, codepoint))
				return false;

			utf8_data += length;
		}
	}

	return true;
}

static const char *dmc_unrar_utf8_get_first_invalid(const char *str, size_t size) {
	while ((size > 0) && *str) {
		uint32_t codepoint;
		const size_t length = dmc_unrar_unicode_utf8_get((const uint8_t *)str, &codepoint);

		if (!length || (length > size))
			return str;

		if (!dmc_unrar_unicode_utf32_is_valid   (codepoint) ||
		     dmc_unrar_unicode_utf32_is_overlong(codepoint, length))
			return str;

		str  += length;
		size -= length;
	}

	return NULL;
}

bool dmc_unrar_unicode_is_valid_utf8(const char *str) {
	if (!str)
		return false;

	return dmc_unrar_utf8_get_first_invalid(str, SIZE_MAX) == NULL;
}

bool dmc_unrar_unicode_make_valid_utf8(char *str) {
	if (!str)
		return false;

	{
		char *first_invalid = (char *)dmc_unrar_utf8_get_first_invalid(str, SIZE_MAX);
		if (!first_invalid)
			return false;

		*first_invalid = '\0';
	}

	return true;;
}

dmc_unrar_unicode_encoding dmc_unrar_unicode_detect_encoding(const void *data, size_t data_size) {
	const uint8_t *bytes = (const uint8_t *)data;
	bool has_null = false;
	size_t i;

	if (!bytes || !data_size)
		return DMC_UNRAR_UNICODE_ENCODING_UNKNOWN;

	/* BOM. */
	if ((data_size >= 2) && ((data_size % 1) == 0))
		if ((bytes[0] == 0xFF) && (bytes[1] == 0xFE))
			return DMC_UNRAR_UNICODE_ENCODING_UTF16LE;

	i = 0;
	while (i < data_size)
		if (!bytes[i++])
			break;

	has_null = i < (data_size - 1);

	if (!has_null)
		if (dmc_unrar_utf8_get_first_invalid((const char *)bytes, data_size) == NULL)
			return DMC_UNRAR_UNICODE_ENCODING_UTF8;

	if (dmc_unrar_unicode_utf16_to_utf8(bytes, data_size, NULL, SIZE_MAX, NULL,
	    &dmc_unrar_unicode_read_uint16le_from_uint8, &dmc_unrar_unicode_advance_uint8))
		return DMC_UNRAR_UNICODE_ENCODING_UTF16LE;

	return DMC_UNRAR_UNICODE_ENCODING_UNKNOWN;
}

size_t dmc_unrar_unicode_convert_utf16le_to_utf8(const void *utf16le_data, size_t utf16le_size,
	char *utf8_data, size_t utf8_size) {

	const uint8_t *bytes = (const uint8_t *)utf16le_data;

	if (!bytes || !utf16le_size)
		return 0;

	/* Remove the BOM. */
	if ((utf16le_size >= 2) && ((utf16le_size % 1) == 0)) {
		if ((bytes[0] == 0xFF) && (bytes[1] == 0xFE)) {
			bytes += 2;
			utf16le_size -= 2;
		}
	}

	if (!utf8_data) {
		if (!dmc_unrar_unicode_utf16_to_utf8(bytes, utf16le_size, NULL, SIZE_MAX, &utf8_size,
		    &dmc_unrar_unicode_read_uint16le_from_uint8, &dmc_unrar_unicode_advance_uint8))
			return 0;

		return utf8_size + 1;
	}

	if (!dmc_unrar_unicode_utf16_to_utf8(bytes, utf16le_size, utf8_data, utf8_size - 1, &utf8_size,
	    &dmc_unrar_unicode_read_uint16le_from_uint8, &dmc_unrar_unicode_advance_uint8))
		return 0;

	utf8_data[utf8_size] = '\0';
	return utf8_size + 1;
}

/* '--- */

/* .--- Information about files in RAR archives */
size_t dmc_unrar_get_file_count(dmc_unrar_archive *archive) {
	if (!archive || !archive->internal_state)
		return 0;

	return archive->internal_state->file_count;
}

static dmc_unrar_file_block *dmc_unrar_get_file(dmc_unrar_archive *archive, size_t index) {
	if (!archive || !archive->internal_state)
		return NULL;

	if (!archive->internal_state->files || (index >= archive->internal_state->file_count))
		return NULL;

	return &archive->internal_state->files[index];
}

const dmc_unrar_file *dmc_unrar_get_file_stat(dmc_unrar_archive *archive, size_t index) {
	dmc_unrar_file_block *file = dmc_unrar_get_file(archive, index);
	if (!file)
		return NULL;

	return &file->file;
}

#define DMC_UNRAR_FILENAME_MAX_LENGTH 512

static bool dmc_unrar_get_filename_utf16(const uint8_t *data, size_t data_size,
		uint16_t *name_utf16, size_t *name_utf16_length) {

	/* Unicode filenames in RAR archives might be UTF-16 encoded, and then
	 * compressed with a by remembering a common high byte value and
	 * some simple RLE.
	 *
	 * The data is split in a data section for RLE and a command section,
	 * split by a 0-byte.
	 *
	 * If there is no 0-byte, the data is just UTF-8 encoded.
	 */

	const uint8_t *utf16_data;
	size_t utf16_data_begin = 0, utf16_data_length = 0, i;

	uint16_t high_byte;
	int flag_byte = 0, flag_bits = 0;

	*name_utf16_length = 0;

	/* Look for the beginning of the command section. */
	while (utf16_data_begin < data_size && data[utf16_data_begin])
		utf16_data_begin++;

	utf16_data_length = data_size - utf16_data_begin - 1;

	/* No command section, so the data is just UTF-8 encoded. */
	if ((utf16_data_begin == data_size) || (utf16_data_length <= 1))
		return false;

	utf16_data = data + utf16_data_begin + 1;

	/* Common high byte. */
	high_byte = *utf16_data++;
	utf16_data_length--;

	while (utf16_data_length > 0) {
		if (flag_bits == 0) {
			flag_byte = *utf16_data++;
			flag_bits = 8;

			utf16_data_length--;
		}

		if ((utf16_data_length == 0) || (*name_utf16_length == DMC_UNRAR_FILENAME_MAX_LENGTH))
			break;

		flag_bits -= 2;
		switch ((flag_byte >> flag_bits) & 3) {
			case 0: /* High byte 0x00 (Basic Latin + Latin-1 Supplement). */
				if (utf16_data_length >= 1) {
					name_utf16[(*name_utf16_length)++] = *utf16_data++;
					utf16_data_length--;
				}
				break;

			case 1: /* Common high byte. */
				if (utf16_data_length >= 1) {
					name_utf16[(*name_utf16_length)++] = high_byte + *utf16_data++;
					utf16_data_length--;
				}
				break;

			case 2: /* Full 2-byte UTF-16 code unit. */
				if (utf16_data_length >= 2) {
					name_utf16[(*name_utf16_length)++] = dmc_unrar_get_uint16le(utf16_data);
					utf16_data_length -= 2;
				}
				break;

			case 3: /* RLE. */
				if (utf16_data_length >= 1) {
					uint8_t length;
					bool with_correction;

					length = *utf16_data++;
					utf16_data_length--;

					with_correction = length & 0x80;
					length = DMC_UNRAR_MIN((size_t) ((length & 0x7F) + 2),
					                       (size_t) (DMC_UNRAR_FILENAME_MAX_LENGTH - *name_utf16_length));

					if (with_correction) {
						/* With correction and common high byte. */

						if (utf16_data_length >= 1) {
							uint8_t correction = *utf16_data++;
							utf16_data_length--;

							for (i = 0; i < length; i++, (*name_utf16_length)++)
								name_utf16[*name_utf16_length] = high_byte + data[*name_utf16_length] + correction;
						}
					} else {
						/* High byte 0x00 (Basic Latin + Latin-1 Supplement). */

						for (i = 0; i < length; i++, (*name_utf16_length)++)
							name_utf16[*name_utf16_length] = data[*name_utf16_length];
					}
				}
				break;
		}
	}

	return true;
}

static size_t dmc_unrar_get_filename_length(dmc_unrar_archive *archive, size_t index) {
	dmc_unrar_file_block *file = dmc_unrar_get_file(archive, index);
	size_t name_size;

	if (!file)
		return 0;

	name_size = file->name_size;

	/* RAR5 should always store the name in UTF-8. */
	if (archive->internal_state->generation == DMC_UNRAR_GENERATION_RAR5)
		return name_size + 1;

	DMC_UNRAR_ASSERT(archive->internal_state->generation == DMC_UNRAR_GENERATION_RAR4);

	/* ASCII name. We can just pass through the size from the file block. */
	if (!(file->flags & DMC_UNRAR_FLAG4_FILE_NAMEUNICODE))
		return name_size + 1;

	/* Unicode name. We actually need to read the name from file now. */

	{
		uint8_t name[DMC_UNRAR_FILENAME_MAX_LENGTH];

		if (name_size > DMC_UNRAR_FILENAME_MAX_LENGTH)
			return 0;

		if (!dmc_unrar_archive_seek(&archive->io, file->name_offset))
			return 0;

		if (!dmc_unrar_archive_read_checked(&archive->io, name, name_size))
			return 0;

		{
			uint16_t name_utf16[DMC_UNRAR_FILENAME_MAX_LENGTH];
			size_t utf16_length = 0;

			/* The name is not UTF-16 encoded, but UTF-8. We know the length then. */
			if (!dmc_unrar_get_filename_utf16(name, name_size, name_utf16, &utf16_length))
				return name_size + 1;

			/* The name is UTF-16 encoded. Convert to UTF-8 to figure out the length. */
			if (!dmc_unrar_unicode_utf16_to_utf8(name_utf16, utf16_length * 2, NULL, SIZE_MAX, &name_size,
			     &dmc_unrar_unicode_read_uint16le_from_uint16, &dmc_unrar_unicode_advance_uint16))
				return 0;
		}
	}

	return name_size + 1;
}

size_t dmc_unrar_get_filename(dmc_unrar_archive *archive, size_t index,
		char *filename, size_t filename_size) {

	dmc_unrar_file_block *file = dmc_unrar_get_file(archive, index);
	size_t name_size;

	if (!file)
		return 0;

	/* If filename is NULL, return the number of bytes in the complete name. */
	if (!filename)
		return dmc_unrar_get_filename_length(archive, index);

	if (!dmc_unrar_archive_seek(&archive->io, file->name_offset))
		return 0;

	name_size = file->name_size;

	if ((archive->internal_state->generation == DMC_UNRAR_GENERATION_RAR4) &&
	    (file->flags & DMC_UNRAR_FLAG4_FILE_NAMEUNICODE)) {

		/* RAR4 Unicode name. */

		uint8_t name_unicode[DMC_UNRAR_FILENAME_MAX_LENGTH];
		uint16_t name_utf16[DMC_UNRAR_FILENAME_MAX_LENGTH];
		size_t utf16_length = 0;

		if (name_size > DMC_UNRAR_FILENAME_MAX_LENGTH)
			return 0;

		name_size = dmc_unrar_archive_read(&archive->io, name_unicode, name_size);
		if (name_size == 0)
			return 0;

		if (dmc_unrar_get_filename_utf16(name_unicode, name_size, name_utf16, &utf16_length)) {
			/* UTF-16 encoded. */

			if (!dmc_unrar_unicode_utf16_to_utf8(name_utf16, utf16_length * 2,
			     filename, filename_size - 1, &filename_size,
			     &dmc_unrar_unicode_read_uint16le_from_uint16, &dmc_unrar_unicode_advance_uint16))
				return 0;

		} else {
			/* UTF-8 encoded. */

			filename_size = DMC_UNRAR_MIN(filename_size - 1, name_size);
			memcpy(filename, name_unicode, filename_size);
		}

	} else {
		/* RAR4 ASCII or RAR5 UTF-8 name. */

		filename_size = DMC_UNRAR_MIN(filename_size, name_size + 1);
		if (filename_size == 0)
			return 0;

		filename_size = dmc_unrar_archive_read(&archive->io, filename, filename_size - 1);
	}

	filename[filename_size] = '\0';

	/* Use portable path separators. */
	{
		size_t i;
		for (i = 0; i < filename_size; i++)
			if (filename[i] == '\\')
				filename[i] = '/';
	}

	return filename_size + 1;
}

static dmc_unrar_return dmc_unrar_file_extract_mem_simple(dmc_unrar_archive *archive,
	dmc_unrar_file_block *file, void *buffer, size_t buffer_size, size_t *uncompressed_size);

bool dmc_unrar_extract_callback_mem(void *opaque, void **buffer,
	size_t *buffer_size, size_t uncompressed_size, dmc_unrar_return *err);

static bool dmc_unrar_20_read_comment_file_at_position(dmc_unrar_archive *archive,
		dmc_unrar_file_block *file) {

	uint8_t version, method;
	uint16_t uncompressed_size;
	dmc_unrar_block_header comment_block;

	if (dmc_unrar_rar4_read_block_header(archive, &comment_block) != DMC_UNRAR_OK)
		return false;

	if (comment_block.type != DMC_UNRAR_BLOCK4_TYPE_COMMENT)
		return false;

	if (!dmc_unrar_archive_read_uint16le(&archive->io, &uncompressed_size))
		return false;
	if (!dmc_unrar_archive_read_uint8(&archive->io, &version))
		return false;
	if (!dmc_unrar_archive_read_uint8(&archive->io, &method))
		return false;

	file->start_pos = archive->io.offset + 2;

	file->file.compressed_size   = comment_block.header_size - 13;
	file->file.uncompressed_size = uncompressed_size;

	file->version = version;
	file->method  = method;

	file->dict_size = dmc_unrar_rar4_get_dict_size(file);

	return true;
}

static bool dmc_unrar_20_read_comment_file(dmc_unrar_archive *archive, dmc_unrar_block_header *block,
		dmc_unrar_file_block *file) {

	/* 2.0/2.6 comments aren't really normal files. We hack one together anyways. */

	if (!dmc_unrar_archive_seek(&archive->io, block->extra_pos))
		return false;

	if (!dmc_unrar_archive_seek(&archive->io, archive->io.offset + 6))
		return false;

	if (block->flags & DMC_UNRAR_FLAG4_ARCHIVE_ENCRYPTVERSION)
		if (!dmc_unrar_archive_seek(&archive->io, archive->io.offset + 1))
			return DMC_UNRAR_SEEK_FAIL;

	return dmc_unrar_20_read_comment_file_at_position(archive, file);
}

static bool dmc_unrar_30_read_comment_file(dmc_unrar_archive *archive, dmc_unrar_block_header *block,
		dmc_unrar_file_block *file) {

	/* 2.9/3.6 comments are normal files with a name "CMT" in a sub block. */

	if (!dmc_unrar_archive_seek(&archive->io, block->extra_pos))
		return false;

	if (dmc_unrar_rar4_read_file_header(archive, block, file, false) != DMC_UNRAR_OK)
		return false;

	return true;
}

static bool dmc_unrar_50_read_comment_file(dmc_unrar_archive *archive, dmc_unrar_block_header *block,
		dmc_unrar_file_block *file) {

	/* 5.0 comments are normal files with a name "CMT" in a service block. */

	if (!dmc_unrar_archive_seek(&archive->io, block->extra_pos))
		return false;

	if (dmc_unrar_rar5_read_file_header(archive, block, file) != DMC_UNRAR_OK)
		return false;

	return true;
}

size_t dmc_unrar_get_archive_comment(dmc_unrar_archive *archive, void *comment, size_t comment_size) {
	dmc_unrar_block_header *block;
	dmc_unrar_file_block comment_file;

	if (!archive || !archive->internal_state)
		return 0;

	block = archive->internal_state->comment;

	/* No comment, no dice. */
	if (!block)
		return 0;

	DMC_UNRAR_CLEAR_OBJ(comment_file);

	if (archive->internal_state->generation == DMC_UNRAR_GENERATION_RAR4) {

		if (block->type == DMC_UNRAR_BLOCK4_TYPE_ARCHIVEHEADER) {
			/* RAR 2.0/2.6 style: within the archive header block. */

			if (!dmc_unrar_20_read_comment_file(archive, block, &comment_file))
				return 0;

		} else if (block->type == DMC_UNRAR_BLOCK4_TYPE_NEWSUB) {
			/* RAR 2.9/3.6 style: within a sub block. */

			if (!dmc_unrar_30_read_comment_file(archive, block, &comment_file))
				return 0;

		}

	} else if (archive->internal_state->generation == DMC_UNRAR_GENERATION_RAR5) {
		/* RAR 5.0 style: within a service block. */

		if (block->type == DMC_UNRAR_BLOCK5_TYPE_SERVICE) {

			if (!dmc_unrar_50_read_comment_file(archive, block, &comment_file))
				return 0;
		}

	}

	/* No comment. */
	if (comment_file.file.uncompressed_size == 0)
		return 0;

	/* If comment is NULL, return the number of bytes in the complete comment. */
	if (!comment)
		return comment_file.file.uncompressed_size;

	comment_size = DMC_UNRAR_MIN(comment_size, comment_file.file.uncompressed_size);
	if (comment_size == 0)
		return 0;

	{
		dmc_unrar_return extract;

		extract = dmc_unrar_file_extract_mem_simple(archive, &comment_file, comment, comment_size, &comment_size);
		if (extract != DMC_UNRAR_OK)
			return 0;
	}

	return comment_size;
}

size_t dmc_unrar_get_file_comment(dmc_unrar_archive *archive, size_t index,
	void *comment, size_t comment_size) {

	dmc_unrar_file_block *file = dmc_unrar_get_file(archive, index);
	dmc_unrar_file_block comment_file;

	if (!file)
		return 0;

	if ((archive->internal_state->generation != DMC_UNRAR_GENERATION_RAR4) ||
	   (!(file->flags & DMC_UNRAR_FLAG4_FILE_HASCOMMENT)))
		return 0;

	if (!dmc_unrar_archive_seek(&archive->io, file->name_offset + file->name_size))
		return 0;

	DMC_UNRAR_CLEAR_OBJ(comment_file);

	if (!dmc_unrar_20_read_comment_file_at_position(archive, &comment_file))
		return 0;

	/* If comment is NULL, return the number of bytes in the complete comment. */
	if (!comment)
		return comment_file.file.uncompressed_size;

	comment_size = DMC_UNRAR_MIN(comment_size, comment_file.file.uncompressed_size);
	if (comment_size == 0)
		return 0;

	{
		dmc_unrar_return extract;

		extract = dmc_unrar_file_extract_mem_simple(archive, &comment_file, comment, comment_size, &comment_size);
		if (extract != DMC_UNRAR_OK)
			return 0;
	}

	return comment_size;
}

bool dmc_unrar_file_is_directory(dmc_unrar_archive *archive, size_t index) {
	dmc_unrar_file_block *file = dmc_unrar_get_file(archive, index);
	if (!file)
		return false;

	/* RAR5 has a simple flag in the file entry. */
	if (archive->internal_state->generation == DMC_UNRAR_GENERATION_RAR5)
		return file->flags & DMC_UNRAR_FLAG5_FILE_ISDIRECTORY;

	DMC_UNRAR_ASSERT(archive->internal_state->generation == DMC_UNRAR_GENERATION_RAR4);

	/* All bits in the sliding window size set means it's a directory. */
	if ((file->flags & DMC_UNRAR_FLAG4_FILE_WINDOWMASK) == DMC_UNRAR_FLAG4_FILE_WINDOWDIR)
		return true;

	/* Version 1.5 set the name in the DOS attribute flags. */

	if ((file->version == 15) && (file->file.host_os == DMC_UNRAR_HOSTOS_DOS) &&
			(file->file.attrs & DMC_UNRAR_ATTRIB_DOS_DIRECTORY))
		return true;

	if ((file->version == 15) && (file->file.host_os == DMC_UNRAR_HOSTOS_WIN32) &&
			(file->file.attrs & DMC_UNRAR_ATTRIB_DOS_DIRECTORY))
		return true;

	return false;
}

bool dmc_unrar_file_has_comment(dmc_unrar_archive *archive, size_t index) {
	dmc_unrar_file_block *file = dmc_unrar_get_file(archive, index);
	if (!file)
		return false;

	return (archive->internal_state->generation == DMC_UNRAR_GENERATION_RAR4) &&
	       (file->flags & DMC_UNRAR_FLAG4_FILE_HASCOMMENT);
}

dmc_unrar_return dmc_unrar_file_is_supported(dmc_unrar_archive *archive, size_t index) {
	if (!archive || !archive->internal_state)
		return DMC_UNRAR_ARCHIVE_IS_NULL;

	if (!archive->internal_state->files || (index >= archive->internal_state->file_count))
		return DMC_UNRAR_FILE_IS_INVALID;

	/* We can't extract a directory. */
	if (dmc_unrar_file_is_directory(archive, index))
		return DMC_UNRAR_FILE_IS_DIRECTORY;

	{
		dmc_unrar_file_block *file = &archive->internal_state->files[index];

		/* We only know about these versions. */
		if ((file->version != 15) &&
		    (file->version != 20) && (file->version != 26) &&
		    (file->version != 29) && (file->version != 36) &&
		    (file->version != 0x5000))
			return DMC_UNRAR_FILE_UNSUPPORTED_VERSION;

		if (file->dict_size == 0)
			return DMC_UNRAR_FILE_UNSUPPORTED_VERSION;

		/* We only know about these methods. */
		if ((file->method != DMC_UNRAR_METHOD_STORE) &&
		    (file->method != DMC_UNRAR_METHOD_FASTEST) &&
		    (file->method != DMC_UNRAR_METHOD_FAST) &&
		    (file->method != DMC_UNRAR_METHOD_NORMAL) &&
		    (file->method != DMC_UNRAR_METHOD_GOOD) &&
		    (file->method != DMC_UNRAR_METHOD_BEST))
			return DMC_UNRAR_FILE_UNSUPPORTED_METHOD;

		/* We don't support large files. */
		if (file->file.uncompressed_size >= 0x7FFFFFFF)
			return DMC_UNRAR_FILE_UNSUPPORTED_LARGE;

		/* We don't support split files. */
		if (file->is_split)
			return DMC_UNRAR_FILE_UNSUPPORTED_SPLIT;

		/* Check that a solid block has a start entry. */
		if (file->is_solid)
			if (!file->solid_start || (file->solid_start == file))
				return DMC_UNRAR_FILE_SOLID_BROKEN;

		/* We don't support links. */
		if (file->is_link)
			return DMC_UNRAR_FILE_UNSUPPORTED_LINK;

		/* We don't support encrypted files. */
		if (file->is_encrypted)
			return DMC_UNRAR_FILE_UNSUPPORTED_ENCRYPTED;

		/* Uncompressed file, but uncompressed size differs from the compressed size?!?
		   Used by symbolic links in RAR5, but we catch that above. */
		if ((file->method == DMC_UNRAR_METHOD_STORE) &&
		    (file->file.uncompressed_size != file->file.compressed_size))
			return DMC_UNRAR_INVALID_DATA;

	}

	return DMC_UNRAR_OK;
}

/* .--- Extracting file entries */
bool dmc_unrar_extract_callback_mem(void *opaque, void **buffer,
	size_t *buffer_size, size_t uncompressed_size, dmc_unrar_return *err) {

	(void)opaque; (void)buffer; (void)buffer_size; (void)uncompressed_size; (void)err;
	return false;
}

static dmc_unrar_return dmc_unrar_file_extract(dmc_unrar_archive *archive, dmc_unrar_file_block *file,
	void *buffer, size_t buffer_size, size_t *uncompressed_size, uint32_t *crc,
	void *opaque, dmc_unrar_extract_callback_func callback);

dmc_unrar_return dmc_unrar_extract_file_to_mem(dmc_unrar_archive *archive, size_t index,
		void *buffer, size_t buffer_size, size_t *uncompressed_size, bool validate_crc) {

	size_t output_size = 0;

	if (!archive || !buffer)
		return DMC_UNRAR_ARCHIVE_EMPTY;

	if (uncompressed_size)
		*uncompressed_size = 0;

	{
		dmc_unrar_return is_supported = dmc_unrar_file_is_supported(archive, index);
		if (is_supported != DMC_UNRAR_OK)
			return is_supported;
	}

	{
		dmc_unrar_file_block *file = &archive->internal_state->files[index];
		uint32_t crc;

		buffer_size = DMC_UNRAR_MIN(buffer_size, file->file.uncompressed_size);

		{
			dmc_unrar_return uncompressed =
				dmc_unrar_file_extract(archive, file, buffer, buffer_size, &output_size,
				                       &crc, NULL, &dmc_unrar_extract_callback_mem);

			if (uncompressed != DMC_UNRAR_OK)
				return uncompressed;
		}

		if (uncompressed_size)
			*uncompressed_size = output_size;

		if (validate_crc)
			if (file->file.has_crc && (file->file.crc != crc))
				return DMC_UNRAR_FILE_CRC32_FAIL;
	}

	return DMC_UNRAR_OK;
}

/** Extract a file entry into a dynamically allocated heap buffer. */
dmc_unrar_return dmc_unrar_extract_file_to_heap(dmc_unrar_archive *archive, size_t index,
		void **buffer, size_t *uncompressed_size, bool validate_crc) {

	if (!buffer || !uncompressed_size)
		return DMC_UNRAR_ARCHIVE_EMPTY;

	{
		dmc_unrar_return is_supported = dmc_unrar_file_is_supported(archive, index);
		if (is_supported != DMC_UNRAR_OK)
			return is_supported;
	}

	{
		dmc_unrar_file_block *file = &archive->internal_state->files[index];

		void *heap_buffer = dmc_unrar_malloc(&archive->alloc, file->file.uncompressed_size, 1);
		if (!heap_buffer)
			return DMC_UNRAR_ALLOC_FAIL;

		{
			dmc_unrar_return extracted =
				dmc_unrar_extract_file_to_mem(archive, index, heap_buffer,
				                              file->file.uncompressed_size, uncompressed_size,
				                              validate_crc);

			if (extracted != DMC_UNRAR_OK) {
				dmc_unrar_free(&archive->alloc, buffer);
				return extracted;
			}
		}

		*buffer = heap_buffer;
	}

	return DMC_UNRAR_OK;
}

#if DMC_UNRAR_DISABLE_STDIO != 1
bool dmc_unrar_extract_callback_file(void *opaque, void **buffer,
	size_t *buffer_size, size_t uncompressed_size, dmc_unrar_return *err) {

	FILE *file = (FILE *)opaque;
	DMC_UNRAR_ASSERT(file);

	if (fwrite(*buffer, 1, uncompressed_size, file) != uncompressed_size) {
		*err = DMC_UNRAR_WRITE_FAIL;
		return false;
	}

	(void)buffer_size;
	return true;
}

static dmc_unrar_return dmc_unrar_get_file_checked(dmc_unrar_archive *archive, size_t index,
		dmc_unrar_file_block **file) {

	const dmc_unrar_return is_supported = dmc_unrar_file_is_supported(archive, index);
	if (is_supported != DMC_UNRAR_OK)
		return is_supported;

	assert(file);

	*file = dmc_unrar_get_file(archive, index);
	assert(*file);

	return DMC_UNRAR_OK;
}

dmc_unrar_return dmc_unrar_extract_file_to_file(dmc_unrar_archive *archive, size_t index,
	FILE *file, size_t *uncompressed_size, bool validate_crc) {

	uint8_t buffer[4096];
	dmc_unrar_file_block *file_entry = NULL;

	if (!archive || !file)
		return DMC_UNRAR_ARCHIVE_EMPTY;

	if (uncompressed_size)
		*uncompressed_size = 0;

	{
		dmc_unrar_return has_file;
		if ((has_file = dmc_unrar_get_file_checked(archive, index, &file_entry)) != DMC_UNRAR_OK)
			return has_file;
	}

	assert(file_entry);

	{
		uint32_t crc = 0;
		size_t output_size;
		dmc_unrar_return extracted;

		extracted = dmc_unrar_file_extract(archive, file_entry, buffer, DMC_UNRAR_ARRAYSIZE(buffer),
		            &output_size, &crc, file, &dmc_unrar_extract_callback_file);

		if (extracted != DMC_UNRAR_OK)
			return extracted;

		if (uncompressed_size)
			*uncompressed_size = output_size;

		if (validate_crc)
			if (file_entry->file.has_crc && (file_entry->file.crc != crc))
				return DMC_UNRAR_FILE_CRC32_FAIL;
	}

	if (fflush(file) || ferror(file))
		return DMC_UNRAR_WRITE_FAIL;

	return DMC_UNRAR_OK;
}

dmc_unrar_return dmc_unrar_extract_file_to_path(dmc_unrar_archive *archive, size_t index,
	const char *path, size_t *uncompressed_size, bool validate_crc) {

	dmc_unrar_return return_code;
	FILE *file = NULL;

	if (!(file = fopen(path, "wb")))
		return DMC_UNRAR_OPEN_FAIL;

	return_code = dmc_unrar_extract_file_to_file(archive, index, file, uncompressed_size, validate_crc);

	fclose(file);
	return return_code;
}
#endif /* DMC_UNRAR_DISABLE_STDIO */

typedef size_t (*dmc_unrar_extractor_func)(void *opaque, void *buffer, size_t buffer_size,
	dmc_unrar_return *err);

static dmc_unrar_return dmc_unrar_file_extract_with_callback_and_extractor(dmc_unrar_archive *archive,
	dmc_unrar_file_block *file, void *buffer, size_t buffer_size, size_t *uncompressed_size,
	uint32_t *crc, void *opaque_callback, dmc_unrar_extract_callback_func callback,
	void *opaque_extractor, dmc_unrar_extractor_func extractor);

static dmc_unrar_return dmc_unrar_file_unstore(dmc_unrar_archive *archive, dmc_unrar_file_block *file,
	void *buffer, size_t buffer_size, size_t *uncompressed_size, uint32_t *crc,
	void *opaque, dmc_unrar_extract_callback_func callback);
static dmc_unrar_return dmc_unrar_file_unpack(dmc_unrar_archive *archive, dmc_unrar_file_block *file,
	void *buffer, size_t buffer_size, size_t *uncompressed_size, uint32_t *crc,
	void *opaque, dmc_unrar_extract_callback_func callback);

/** Internal function that calls the method-specific file extractors. */
static dmc_unrar_return dmc_unrar_file_extract(dmc_unrar_archive *archive, dmc_unrar_file_block *file,
		void *buffer, size_t buffer_size, size_t *uncompressed_size, uint32_t *crc,
		void *opaque, dmc_unrar_extract_callback_func callback) {

	DMC_UNRAR_ASSERT(archive && file);

	switch (file->method) {
		case DMC_UNRAR_METHOD_STORE:
			return dmc_unrar_file_unstore(archive, file, buffer, buffer_size,
			                              uncompressed_size, crc, opaque, callback);

		case DMC_UNRAR_METHOD_FASTEST:
		case DMC_UNRAR_METHOD_FAST:
		case DMC_UNRAR_METHOD_NORMAL:
		case DMC_UNRAR_METHOD_GOOD:
		case DMC_UNRAR_METHOD_BEST:
			return dmc_unrar_file_unpack(archive, file, buffer, buffer_size,
			                             uncompressed_size, crc, opaque, callback);

		default:
			break;
	}

	return DMC_UNRAR_FILE_UNSUPPORTED_METHOD;
}

static dmc_unrar_return dmc_unrar_file_extract_with_callback_and_extractor(dmc_unrar_archive *archive,
		dmc_unrar_file_block *file, void *buffer, size_t buffer_size, size_t *uncompressed_size,
		uint32_t *crc, void *opaque_callback, dmc_unrar_extract_callback_func callback,
		void *opaque_extractor, dmc_unrar_extractor_func extractor) {

	size_t total_size;
	bool buffer_allocated = false, finished = false;
	dmc_unrar_return return_code = DMC_UNRAR_OK;

	DMC_UNRAR_ASSERT(archive && archive->internal_state && file && crc);

	*crc = 0;
	if (uncompressed_size)
		*uncompressed_size = 0;

	total_size = file->file.uncompressed_size;
	while (!finished && (total_size > 0) && (buffer_size > 0)) {
		void *old_buffer;
		size_t old_buffer_size, read_size;

		if (!buffer) {
			buffer_allocated = true;

			buffer = dmc_unrar_malloc(&archive->alloc, buffer_size, 1);
			if (!buffer)
				return DMC_UNRAR_ALLOC_FAIL;
		}

		read_size = extractor(opaque_extractor, buffer, DMC_UNRAR_MIN(buffer_size, total_size), &return_code);
		if ((return_code != DMC_UNRAR_OK) || (read_size == 0))
			break;

		*crc = dmc_unrar_crc32_continue_from_mem(*crc, buffer, read_size);

		if (uncompressed_size)
			*uncompressed_size += read_size;
		total_size -= read_size;

		old_buffer = buffer;
		old_buffer_size = buffer_size;

		finished = !callback(opaque_callback, (void **) &buffer, &buffer_size, read_size, &return_code);

		if ((old_buffer != buffer) || (old_buffer_size != buffer_size)) {
			if (buffer_allocated)
				dmc_unrar_free(&archive->alloc, old_buffer);

			buffer_allocated = false;
		}
	}

	if (buffer_allocated)
		dmc_unrar_free(&archive->alloc, buffer);

	return return_code;
}

static size_t dmc_unrar_extractor_unstore(void *opaque, void *buffer, size_t buffer_size,
		dmc_unrar_return *err) {

	(void)err;
	return dmc_unrar_archive_read((dmc_unrar_io *)opaque, buffer, buffer_size);
}

/** Extract an uncompressed (stored) file. */
static dmc_unrar_return dmc_unrar_file_unstore(dmc_unrar_archive *archive, dmc_unrar_file_block *file,
		void *buffer, size_t buffer_size, size_t *uncompressed_size, uint32_t *crc,
		void *opaque, dmc_unrar_extract_callback_func callback) {

	DMC_UNRAR_ASSERT(archive && archive->internal_state && file && crc);
	DMC_UNRAR_ASSERT(file->file.compressed_size == file->file.uncompressed_size);

	if (!dmc_unrar_archive_seek(&archive->io, file->start_pos))
		return DMC_UNRAR_SEEK_FAIL;

	return dmc_unrar_file_extract_with_callback_and_extractor(archive, file,
		buffer, buffer_size, uncompressed_size, crc, opaque, callback,
		&archive->io, &dmc_unrar_extractor_unstore);
}

static dmc_unrar_return dmc_unrar_rar15_create(dmc_unrar_rar_context *ctx,
	dmc_unrar_archive *archive, dmc_unrar_file_block *file);
static dmc_unrar_return dmc_unrar_rar20_create(dmc_unrar_rar_context *ctx,
	dmc_unrar_archive *archive, dmc_unrar_file_block *file);
static dmc_unrar_return dmc_unrar_rar30_create(dmc_unrar_rar_context *ctx,
	dmc_unrar_archive *archive, dmc_unrar_file_block *file);
static dmc_unrar_return dmc_unrar_rar50_create(dmc_unrar_rar_context *ctx,
	dmc_unrar_archive *archive, dmc_unrar_file_block *file);

/** Extract a compressed file. */
static dmc_unrar_return dmc_unrar_file_unpack(dmc_unrar_archive *archive, dmc_unrar_file_block *file,
		void *buffer, size_t buffer_size, size_t *uncompressed_size, uint32_t *crc,
		void *opaque, dmc_unrar_extract_callback_func callback) {

	dmc_unrar_return return_code;
	switch (file->version) {
		case 15:
			return_code = dmc_unrar_rar15_create(archive->internal_state->unpack_context, archive, file);
			break;

		case 20:
		case 26:
			return_code = dmc_unrar_rar20_create(archive->internal_state->unpack_context, archive, file);
			break;

		case 29:
		case 36:
			return_code = dmc_unrar_rar30_create(archive->internal_state->unpack_context, archive, file);
			break;

		case 0x5000:
			return_code = dmc_unrar_rar50_create(archive->internal_state->unpack_context, archive, file);
			break;

		default:
			return_code = DMC_UNRAR_FILE_UNSUPPORTED_VERSION;
			break;
	}

	if (return_code != DMC_UNRAR_OK) {
		dmc_unrar_rar_context_destroy(archive->internal_state->unpack_context);
		return return_code;
	}

	return dmc_unrar_rar_context_unpack(archive->internal_state->unpack_context,
		archive, file, buffer, buffer_size, uncompressed_size, crc, opaque, callback);
}

static dmc_unrar_return dmc_unrar_file_extract_mem_simple(dmc_unrar_archive *archive,
		dmc_unrar_file_block *file, void *buffer, size_t buffer_size, size_t *uncompressed_size) {

	uint32_t crc;
	return dmc_unrar_file_extract(archive, file, buffer, buffer_size, uncompressed_size, &crc,
		NULL, &dmc_unrar_extract_callback_mem);
}
/* '--- */

/* .--- Bitstream interface */
#if DMC_UNRAR_64BIT == 1
typedef uint64_t dmc_unrar_bs_cache_t;
#else
typedef uint32_t dmc_unrar_bs_cache_t;
#endif

/** The context structure for a bitstream. */
typedef struct dmc_unrar_bs_tag {
	dmc_unrar_io io;

	bool error; /**< Was an error condition raised? */

	/** The number of unaligned bytes in the L2 cache.
	 *  This will always be 0 until the end of the stream is hit.
	 *
	 *  At the end of the stream there will be a number of bytes that don't cleanly
	 *  fit in an L1 cache line, so we use this variable to know whether or not the
	 *  bistreamer needs to run on a slower path to read those last bytes. This will
	 *  never be more than sizeof(dmc_unrar_bs_cache_t). */
	size_t unaligned_byte_count;

	/** The content of the unaligned bytes. */
	dmc_unrar_bs_cache_t unaligned_cache;

	/** The index of the next valid cache line in the "L2" cache. */
	size_t next_l2_line;

	/** The number of bits that have been consumed by the cache. */
	size_t consumed_bits;

	/** Offset within the bitstream, in bits. */
	size_t offset_bits;

	/** The cached data which was most recently read from the client.
	 *
	 *  There are two levels of cache. Data flows as such:
	 *  Client -> L2 -> L1
	 *
	 *  The L2 -> L1 movement is aligned and runs on a fast path in just a few instructions. */
	dmc_unrar_bs_cache_t cache_l2[DMC_UNRAR_BS_BUFFER_SIZE / sizeof(dmc_unrar_bs_cache_t)];

	/** The "L1" cache. */
	dmc_unrar_bs_cache_t cache;
} dmc_unrar_bs;

/** Initialize a bitstreamer from an IO's current position. */
static bool dmc_unrar_bs_init_from_io(dmc_unrar_bs *bs, dmc_unrar_io *io, uint64_t size);

/** Was an error condition raised in this bitstreamer? */
static bool dmc_unrar_bs_has_error(dmc_unrar_bs *bs);

/** Was the end of the bitstream reached? */
static bool dmc_unrar_bs_eos(dmc_unrar_bs *bs);

/** Do we have at least this number of bits left in the bitstream? */
static bool dmc_unrar_bs_has_at_least(dmc_unrar_bs *bs, size_t n);

/** Skip this amount of bits. */
static bool dmc_unrar_bs_skip_bits(dmc_unrar_bs *bs, size_t n);

/** Skip as many bits are necessary to align the reading position to a byte boundary. */
static bool dmc_unrar_bs_skip_to_byte_boundary(dmc_unrar_bs *bs);

/** Read this amount of bits, consuming them. */
static uint32_t dmc_unrar_bs_read_bits(dmc_unrar_bs *bs, size_t n);

/** Peek this amount of bits, without consuming them. */
static uint32_t dmc_unrar_bs_peek_bits(dmc_unrar_bs *bs, size_t n);
/* '--- */

/* .--- Huffman interface */
/** The context structure for a Huffman code. */
typedef struct dmc_unrar_huff_tag {
	dmc_unrar_alloc *alloc;

	size_t node_count; /**< Number of nodes in the decoder tree. */
	uint32_t *tree;    /**< The decoder tree. */

	size_t table_size; /**< Size of the decoder table. */
	uint32_t *table;   /**< The decoder tree. */

} dmc_unrar_huff;

/** Create a Huffman decoder, allocating memory for its internal structures. */
static dmc_unrar_return dmc_unrar_huff_create(dmc_unrar_alloc *alloc, dmc_unrar_huff *huff,
	uint8_t max_length, size_t code_count, const uint32_t *codes, const uint8_t *lengths,
	const uint32_t *symbols);

/** Create a Huffman decoder, just from the code lengths array. */
static dmc_unrar_return dmc_unrar_huff_create_from_lengths(dmc_unrar_alloc *alloc,
	dmc_unrar_huff *huff, const uint8_t *lengths, size_t code_count, uint8_t max_length);

/** Destroy a Huffman decoder, deallocating its internal memory. */
static void dmc_unrar_huff_destroy(dmc_unrar_huff *huff);

/** Decode the next Huffman symbol out of this bitstream. */
static uint32_t dmc_unrar_huff_get_symbol(dmc_unrar_huff *huff,
	dmc_unrar_bs *bs, dmc_unrar_return *err);
/* '--- */

/* .--- LZSS interface */
/** The context structure for an LZSS decoder. */
typedef struct dmc_unrar_lzss_tag {
	dmc_unrar_alloc *alloc;

	size_t window_size; /**< Size of the LSZZ window. */
	size_t window_mask; /**< Mask to clamp an offset to the window. */

	uint8_t *window; /**< The LZSS window. */

	size_t window_offset; /**< Current offset into the window. */

	size_t copy_offset; /**< Overhang copy offset. */
	size_t copy_size;   /**< Overhang copy size. */

} dmc_unrar_lzss;

/** Create an LZSS decoder, allocating a window buffer of the specified size (must be a power of 2!). */
static dmc_unrar_return dmc_unrar_lzss_create(dmc_unrar_alloc *alloc, dmc_unrar_lzss *lzss,
	size_t window_size);

/** Destroy an LZSS decoder, deallocating its internal memory. */
static void dmc_unrar_lzss_destroy(dmc_unrar_lzss *lzss);

/** Emit a literal byte into the buffer and the LZSS window. */
static size_t dmc_unrar_lzss_emit_literal(dmc_unrar_lzss *lzss, uint8_t *buffer,
	size_t buffer_size, size_t buffer_offset, uint8_t literal, size_t *running_output_count);

/** Emit a copy from the LZSS window into the buffer (and back into the LZSS window).
 *
 *  If the buffer runs out before copy_size bytes have been copied, the
 *  remainder is saved as an overhang. Emitting this overhang can be
 *  continued with dmc_unrar_lzss_emit_copy_overhang(). */
static size_t dmc_unrar_lzss_emit_copy(dmc_unrar_lzss *lzss, uint8_t *buffer,
	size_t buffer_size, size_t buffer_offset, size_t copy_offset, size_t copy_size,
	size_t *running_output_count);

/** Continue emitting a copy from the last dmc_unrar_lzss_emit_copy() that was interrupted.
 *
 *  If no overhang is available, this function will do nothing. If the buffer
 *  runs out again before the remainder is copied, the remainder is again
 *  saved as an overhang. */
static size_t dmc_unrar_lzss_emit_copy_overhang(dmc_unrar_lzss *lzss, uint8_t *buffer,
	size_t buffer_size, size_t buffer_offset, size_t *running_output_count);

/** Do we *have* LZSS overhang? */
static bool dmc_unrar_lzss_has_overhang(dmc_unrar_lzss *lzss);
/* '--- */

/* .--- PPMd interface */
#if DMC_UNRAR_DISABLE_PPMD != 1

struct dmc_unrar_ppmd_model_h_tag;
typedef struct dmc_unrar_ppmd_model_h_tag dmc_unrar_ppmd_model_h;

struct dmc_unrar_ppmd_suballocator_h_tag;
typedef struct dmc_unrar_ppmd_suballocator_h_tag dmc_unrar_ppmd_suballocator_h;

/** The context structure for a PPMd decoder. */
typedef struct dmc_unrar_ppmd_tag {
	dmc_unrar_alloc *alloc;

	dmc_unrar_ppmd_model_h *model;
	dmc_unrar_ppmd_suballocator_h *suballoc;

} dmc_unrar_ppmd;

static dmc_unrar_return dmc_unrar_ppmd_create(dmc_unrar_alloc *alloc, dmc_unrar_ppmd *ppmd);
static void dmc_unrar_ppmd_destroy(dmc_unrar_ppmd *ppmd);

static dmc_unrar_return dmc_unrar_ppmd_start(dmc_unrar_ppmd *ppmd, dmc_unrar_bs *bs,
	size_t heap_size_mb, size_t max_order);

static dmc_unrar_return dmc_unrar_ppmd_restart(dmc_unrar_ppmd *ppmd, dmc_unrar_bs *bs);

static uint8_t dmc_unrar_ppmd_get_byte(dmc_unrar_ppmd *ppmd);
#endif /* DMC_UNRAR_DISABLE_PPMD */
/* '--- */

/* .--- Filters interface */
#if DMC_UNRAR_DISABLE_FILTERS != 1

/** Max number of bytes a filter can munge. */
#define DMC_UNRAR_FILTERS_MEMORY_SIZE 0x3C000
/** Max number of bytes a filter bytecode (including static data) we recognize can be. */
#define DMC_UNRAR_FILTERS_BYTECODE_SIZE 256

struct dmc_unrar_filters_interal_state_tag;
typedef struct dmc_unrar_filters_interal_state_tag dmc_unrar_filters_interal_state;

/** The context for the filter stack. */
typedef struct dmc_unrar_filters_tag {
	dmc_unrar_alloc *alloc;

	dmc_unrar_filters_interal_state *internal_state;

} dmc_unrar_filters;

/** Create a filter stack context. */
static dmc_unrar_return dmc_unrar_filters_create(dmc_unrar_alloc *alloc,
	dmc_unrar_filters *filters);
/** Destroy a filter stack context. */
static void dmc_unrar_filters_destroy(dmc_unrar_filters *filters);

/** Is the filter stack empty? I.e. are there no filters available at the moment? */
static bool dmc_unrar_filters_empty(dmc_unrar_filters *filters);

/** Return the offset of the first filter in the stack. */
static size_t dmc_unrar_filters_get_first_offset(dmc_unrar_filters *filters);
/** Return the length of the first filter in the stack. */
static size_t dmc_unrar_filters_get_first_length(dmc_unrar_filters *filters);

/** Parse RAR4 filter bytecode, adding a filter to the filter stack. */
static dmc_unrar_return dmc_unrar_filters_rar4_parse(dmc_unrar_filters *filters,
	const uint8_t *data, size_t data_size, uint8_t flags, size_t current_output_offset);

/** Parse RAR5 filter, adding a filter to the filter stack. */
static dmc_unrar_return dmc_unrar_filters_rar5_parse(dmc_unrar_filters *filters,
	dmc_unrar_bs *bs, size_t current_output_offset);

/** Return the internal memory of the filter stack. */
static uint8_t *dmc_unrar_filters_get_memory(dmc_unrar_filters *filters);

/** Run the filter stack on the data currently in its internal memory.
 *
 *  The output is also found in the filter stack's internal memory, at
 *  the offset out_offset, with a length of out_length bytes. */
static dmc_unrar_return dmc_unrar_filters_run(dmc_unrar_filters *filters,
	size_t current_output_offset, size_t current_file_start, size_t *out_offset, size_t *out_length);

#endif /* DMC_UNRAR_DISABLE_FILTERS */
/* '--- */

/* .--- Common RAR unpacking functions, implementation */
typedef void (*dmc_unrar_rar_context_destroy_func)(dmc_unrar_rar_context *ctx);
typedef dmc_unrar_return (*dmc_unrar_rar_context_unpack_func)(dmc_unrar_rar_context *ctx);

struct dmc_unrar_rar_context_tag {
	uint16_t version; /**< Version of the decoder this context is for. */

	/** Functor destroying this context. */
	dmc_unrar_rar_context_destroy_func destroy;
	/** Functor unpacking a RAR file using this context. */
	dmc_unrar_rar_context_unpack_func unpack;

	dmc_unrar_archive *archive; /**< Archive to work on. */
	dmc_unrar_file_block *file; /**< File to extract. */

	uint8_t *buffer; /**< The buffer to extract into. */

	size_t buffer_size;   /** Size of the buffer. */
	size_t buffer_offset; /** Current offset into the buffer. */
	size_t solid_offset;  /** Number of bytes decoded for this solid part. */
	size_t output_offset; /** Number of bytes decoded in total, including previous solid parts. */

	bool start_new_file; /** Do we need another solid part? */

	size_t current_file_start; /** Offset the current solid part starts at. */

	dmc_unrar_bs bs;     /** Bitstream input. */
	dmc_unrar_lzss lzss; /** LZSS context. */

	/** Does this decoder have a proper solid part end marker?
	 *
	 *  If so, we need to consume all the input data, to make sure we
	 *  get the state for the next solid part right.
	 *
	 *  If not, we need to stop decoding right when the final output
	 *  byte was generated instead. */
	bool has_end_marker;

	/** Private, decoder-specific state. */
	void *internal_state;
};

static dmc_unrar_rar_context *dmc_unrar_rar_context_alloc(dmc_unrar_alloc *alloc) {
	dmc_unrar_rar_context *ctx =
		(dmc_unrar_rar_context *)dmc_unrar_malloc(alloc, 1, sizeof(dmc_unrar_rar_context));

	if (!ctx)
		return 0;

	DMC_UNRAR_CLEAR_OBJ(*ctx);
	return ctx;
}

static dmc_unrar_return dmc_unrar_rar_context_create(dmc_unrar_rar_context *ctx,
		dmc_unrar_archive *archive, dmc_unrar_file_block *file) {

	DMC_UNRAR_ASSERT(ctx && archive && file);

	DMC_UNRAR_CLEAR_OBJ(*ctx);

	ctx->version = file->version;
	ctx->archive = archive;

	return dmc_unrar_lzss_create(&archive->alloc, &ctx->lzss, file->dict_size);
}

static void dmc_unrar_rar_context_destroy(dmc_unrar_rar_context *ctx) {
	if (!ctx)
		return;

	dmc_unrar_lzss_destroy(&ctx->lzss);
	if (ctx->destroy)
		ctx->destroy(ctx);

	DMC_UNRAR_CLEAR_OBJ(*ctx);
}

static dmc_unrar_return dmc_unrar_rar_context_init(dmc_unrar_rar_context *ctx,
		dmc_unrar_archive *archive, dmc_unrar_file_block *file, void *buffer, size_t buffer_size) {

	DMC_UNRAR_ASSERT(ctx && archive && file);

	ctx->archive     = archive;
	ctx->file        = file;
	ctx->buffer      = (uint8_t *)buffer;
	ctx->buffer_size = buffer_size;

	ctx->start_new_file     = false;
	ctx->current_file_start = ctx->output_offset;

	ctx->buffer_offset = 0;
	ctx->solid_offset  = 0;

	if (!dmc_unrar_archive_seek(&archive->io, file->start_pos))
		return DMC_UNRAR_SEEK_FAIL;

	if (!dmc_unrar_bs_init_from_io(&ctx->bs, &archive->io, file->file.compressed_size))
		return DMC_UNRAR_INVALID_DATA;

	return DMC_UNRAR_OK;
}

static void dmc_unrar_rar_context_continue(dmc_unrar_rar_context *ctx,
		void *buffer, size_t buffer_size) {

	ctx->solid_offset += ctx->buffer_offset;

	ctx->buffer        = (uint8_t *)buffer;
	ctx->buffer_size   = buffer_size;
	ctx->buffer_offset = 0;
}

static bool dmc_unrar_rar_context_file_match(dmc_unrar_rar_context *ctx,
		dmc_unrar_file_block *f1, dmc_unrar_file_block *f2) {

	if (!ctx || !f1 || !f2 || (f1 == f2))
		return false;

	if ((f1->method == DMC_UNRAR_METHOD_STORE) || (f2->method == DMC_UNRAR_METHOD_STORE))
		return false;

	if ((f1->version != f2->version) || (ctx->version != f1->version))
		return false;

	if (f1->dict_size != f2->dict_size)
		return false;

	return true;
}

static bool dmc_unrar_rar_context_check(dmc_unrar_rar_context *ctx,
		dmc_unrar_archive *archive, dmc_unrar_file_block *file, dmc_unrar_return *return_code) {

	*return_code = DMC_UNRAR_OK;

	/* Is this context already filled with the previous part of a solid block? */
	if (ctx->internal_state && ctx->file && dmc_unrar_rar_context_file_match(ctx, ctx->file, file))
		if (file->is_solid && (file->solid_prev == ctx->file) && (ctx->file->solid_next == file))
			if (file->solid_start == ctx->file->solid_start)
				return true;

	/* If not, we need to recreate the context for this file. */

	dmc_unrar_rar_context_destroy(ctx);
	*return_code = dmc_unrar_rar_context_create(ctx, archive, file);

	return false;
}

static size_t dmc_unrar_extractor_unpack(void *opaque, void *buffer, size_t buffer_size,
		dmc_unrar_return *err) {

	dmc_unrar_rar_context *ctx = (dmc_unrar_rar_context *)opaque;

	dmc_unrar_rar_context_continue(ctx, buffer, buffer_size);

	*err = ctx->unpack(ctx);

	return ctx->buffer_offset;
}

static dmc_unrar_return dmc_unrar_rar_context_unpack(dmc_unrar_rar_context *ctx,
		dmc_unrar_archive *archive, dmc_unrar_file_block *file,
		void *buffer, size_t buffer_size, size_t *uncompressed_size, uint32_t *crc,
		void *opaque, dmc_unrar_extract_callback_func callback) {

	dmc_unrar_return return_code;

	DMC_UNRAR_ASSERT(ctx && ctx->unpack && archive && file);

	if (file->is_solid) {
		/* If this is a solid file, we need to start with the file
		 * entry that begins the solid block the file is in.
		 *
		 * If we already have a context with a file, that means this
		 * should be the previous file in the block (so we don't
		 * need to retrace our steps and can continue from there). */

		if (!ctx->file) {
			if (!file->solid_start)
				goto broken;

			ctx->file = file->solid_start;

		} else {
			if (!ctx->file->solid_next || !file->solid_prev)
				goto broken;
			if ((ctx->file->solid_next != file) || (file->solid_prev != ctx->file))
				goto broken;
			if (ctx->file->solid_start != file->solid_start)
				goto broken;

			ctx->file = file;
		}
	}

	if (ctx->file) {
		/* Go through all the parts in the solid block we don't care about.
		 * We decompressing them all while throwing away the output (we only
		 * care about the end state of the decoder context). */

		dmc_unrar_file_block *part = ctx->file;

		if (!file->is_solid)
			goto broken;

		while (part && (part != file)) {
			const size_t part_size = ctx->has_end_marker ? SIZE_MAX : part->file.uncompressed_size;

			if (!dmc_unrar_rar_context_file_match(ctx, part, file))
				goto broken;

			return_code = dmc_unrar_rar_context_init(ctx, archive, part, NULL, part_size);
			if (return_code != DMC_UNRAR_OK)
				goto fail;

			if ((return_code = ctx->unpack(ctx)) != DMC_UNRAR_OK)
				goto fail;

			part = part->solid_next;
		}

		if (!part && (ctx->file != file))
			goto broken;
	}

	/* Unpack the file we care about into the buffer. */

	return_code = dmc_unrar_rar_context_init(ctx, archive, file, buffer, buffer_size);
	if (return_code != DMC_UNRAR_OK)
		goto fail;

	return_code = dmc_unrar_file_extract_with_callback_and_extractor(ctx->archive, ctx->file,
		buffer, buffer_size, uncompressed_size, crc, opaque, callback,
		ctx, &dmc_unrar_extractor_unpack);

	if (return_code != DMC_UNRAR_OK)
		goto fail;

	/* If the file is part of a solid block (and not the end), finish off
	 * the input buffer, even though we already have all the output for this
	 * file. The input might still have state the next solid part needs. This
	 * way, we can keep the context around and reuse it, should the next file
	 * we want to decompress the is next part in the solid block. Otherwise,
	 * we would have to retrace our steps, which is not optimal.
	 *
	 * If the file is not part of a solid block (or the end of one), clear
	 * the context. We don't need it anymore.
	 *
	 * TODO: How should we behave if the callback function decided to end
	 * decompression before the end of the file was reached? Currently, we
	 * still decompress the rest if it's part of a solid block here. But
	 * maybe we should skip that then as well? */

	if (ctx->file->solid_next) {

		if (ctx->has_end_marker) {
			ctx->buffer      = NULL;
			ctx->buffer_size = SIZE_MAX;

			if ((return_code = ctx->unpack(ctx)) != DMC_UNRAR_OK)
				goto fail;
		}

	} else
		dmc_unrar_rar_context_destroy(ctx);

	return DMC_UNRAR_OK;

broken:
	return_code = DMC_UNRAR_FILE_SOLID_BROKEN;
fail:
	dmc_unrar_rar_context_destroy(ctx);
	return return_code;
}
/* '--- */

/* .--- Unpacking RAR 1.5 */
#define DMC_UNRAR_15_LENGTH_CODE_COUNT  256
#define DMC_UNRAR_15_LENGTH_CODE_LENGTH  12
#define DMC_UNRAR_15_MAIN_CODE0_COUNT   257
#define DMC_UNRAR_15_MAIN_CODE0_LENGTH   12
#define DMC_UNRAR_15_MAIN_CODE1_COUNT   257
#define DMC_UNRAR_15_MAIN_CODE1_LENGTH   12
#define DMC_UNRAR_15_MAIN_CODE2_COUNT   257
#define DMC_UNRAR_15_MAIN_CODE2_LENGTH   10
#define DMC_UNRAR_15_MAIN_CODE3_COUNT   257
#define DMC_UNRAR_15_MAIN_CODE3_LENGTH   10
#define DMC_UNRAR_15_MAIN_CODE4_COUNT   257
#define DMC_UNRAR_15_MAIN_CODE4_LENGTH    9
#define DMC_UNRAR_15_SHORT_CODE0_COUNT   15
#define DMC_UNRAR_15_SHORT_CODE1_COUNT   14
#define DMC_UNRAR_15_SHORT_CODE2_COUNT   15
#define DMC_UNRAR_15_SHORT_CODE3_COUNT   14

typedef struct dmc_unrar_rar15_context_tag {
	dmc_unrar_rar_context *ctx;

	bool stored_block;

	unsigned int flags;
	unsigned int flag_bits;

	unsigned int literal_weight;
	unsigned int match_weight;

	unsigned int repeat_literal_count;
	unsigned int repeat_last_match_count;

	unsigned int running_average_selector;
	unsigned int running_average_literal;
	unsigned int running_average_length;
	unsigned int running_average_offset;
	unsigned int running_average_below_maximum;

	size_t maximum_offset;

	bool bug_fix_flag;

	size_t last_offset;
	size_t last_length;

	size_t old_offset[4];
	size_t old_offset_index;

	unsigned int flag_table[256];
	unsigned int flag_reverse[256];

	unsigned int literal_table[256];
	unsigned int literal_reverse[256];

	unsigned int offset_table[256];
	unsigned int offset_reverse[256];

	unsigned int short_offset_table[256];

	dmc_unrar_huff huff_lengths1;
	dmc_unrar_huff huff_lengths2;

	dmc_unrar_huff huff_main0;
	dmc_unrar_huff huff_main1;
	dmc_unrar_huff huff_main2;
	dmc_unrar_huff huff_main3;
	dmc_unrar_huff huff_main4;

	dmc_unrar_huff huff_short0;
	dmc_unrar_huff huff_short1;
	dmc_unrar_huff huff_short2;
	dmc_unrar_huff huff_short3;

} dmc_unrar_rar15_context;

static dmc_unrar_return dmc_unrar_rar15_init_huffman(dmc_unrar_rar15_context *ctx);
static void dmc_unrar_rar15_reset_table(unsigned int *table, unsigned int *reverse);

static void dmc_unrar_rar15_destroy(dmc_unrar_rar_context *ctx);
static dmc_unrar_return dmc_unrar_rar15_unpack(dmc_unrar_rar_context *ctx);

static dmc_unrar_return dmc_unrar_rar15_create(dmc_unrar_rar_context *ctx,
		dmc_unrar_archive *archive, dmc_unrar_file_block *file) {

	dmc_unrar_return return_code;

	DMC_UNRAR_ASSERT(ctx && archive && file);

	if (dmc_unrar_rar_context_check(ctx, archive, file, &return_code))
		return DMC_UNRAR_OK;

	if (return_code != DMC_UNRAR_OK)
		return return_code;

	DMC_UNRAR_ASSERT(ctx->archive == archive);

	ctx->has_end_marker = false;

	ctx->internal_state = dmc_unrar_malloc(&archive->alloc, 1, sizeof(dmc_unrar_rar15_context));
	if (!ctx->internal_state)
		return DMC_UNRAR_ALLOC_FAIL;

	ctx->destroy = &dmc_unrar_rar15_destroy;
	ctx->unpack  = &dmc_unrar_rar15_unpack;

	{
		dmc_unrar_rar15_context *ictx = (dmc_unrar_rar15_context *)ctx->internal_state;
		DMC_UNRAR_CLEAR_OBJ(*ictx);

		ictx->ctx = ctx;

		ictx->literal_weight = ictx->match_weight = 0x80;

		ictx->running_average_literal = 0x3500;
		ictx->maximum_offset = 0x2001;

		{
			int i;
			for (i = 0; i < 256; i++) {
				ictx->flag_table[i] = ((-i) & 0xFF) << 8;

				ictx->literal_table[i] = i << 8;
				ictx->offset_table [i] = i << 8;

				ictx->short_offset_table[i] = i;
			}
		}

		dmc_unrar_rar15_reset_table(ictx->offset_table, ictx->offset_reverse);

		return dmc_unrar_rar15_init_huffman(ictx);
	}
}

static void dmc_unrar_rar15_destroy(dmc_unrar_rar_context *ctx) {
	if (!ctx || !ctx->internal_state)
		return;

	{
		dmc_unrar_rar15_context *ictx = (dmc_unrar_rar15_context *)ctx->internal_state;

		dmc_unrar_huff_destroy(&ictx->huff_lengths1);
		dmc_unrar_huff_destroy(&ictx->huff_lengths2);

		dmc_unrar_huff_destroy(&ictx->huff_main0);
		dmc_unrar_huff_destroy(&ictx->huff_main1);
		dmc_unrar_huff_destroy(&ictx->huff_main2);
		dmc_unrar_huff_destroy(&ictx->huff_main3);
		dmc_unrar_huff_destroy(&ictx->huff_main4);

		dmc_unrar_huff_destroy(&ictx->huff_short0);
		dmc_unrar_huff_destroy(&ictx->huff_short1);
		dmc_unrar_huff_destroy(&ictx->huff_short2);
		dmc_unrar_huff_destroy(&ictx->huff_short3);

		if (ctx->archive)
			dmc_unrar_free(&ctx->archive->alloc, ictx);

		ctx->internal_state = NULL;
	}
}

static dmc_unrar_return dmc_unrar_rar15_decompress(dmc_unrar_rar15_context *ctx);

static dmc_unrar_return dmc_unrar_rar15_unpack(dmc_unrar_rar_context *ctx) {
	DMC_UNRAR_ASSERT(ctx && ctx->internal_state);

	return dmc_unrar_rar15_decompress((dmc_unrar_rar15_context *)ctx->internal_state);
}

static void dmc_unrar_rar15_reset_table(unsigned int *table, unsigned int *reverse) {
	int i, j;

	for (i = 0; i < 8; i++)
		for (j = 0; j < 32; j++)
			table[i * 32 + j] = (table[i * 32 + j] & ~0xFF) | (7 - i);

	DMC_UNRAR_CLEAR_OBJS(reverse, 256);

	for (i = 0; i < 7; i++)
		reverse[i] = (7 - i) * 32;
}

static dmc_unrar_return dmc_unrar_rar15_init_huffman(dmc_unrar_rar15_context *ctx) {
	static const uint8_t DMC_UNRAR_15_LENGTH_CODE1[DMC_UNRAR_15_LENGTH_CODE_COUNT] = {
		2,2,3,4,4,5,5,6,6,6,6,7,7,7,7,7,8,8,8,8,9,9,9,9,10,10,10,10,10,10,10,10,12,12,
		12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
		12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
		12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
		12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
		12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
		12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
		12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
		12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
		12,12,12,12,12,12,12,12,12,12,12,12,12,12
	};
	static const uint8_t DMC_UNRAR_15_LENGTH_CODE2[DMC_UNRAR_15_LENGTH_CODE_COUNT] = {
		3,3,3,3,3,4,4,5,5,6,6,6,6,7,7,7,7,7,8,8,8,8,9,9,9,9,10,10,10,10,10,10,10,10,11,
		11,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
		12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
		12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
		12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
		12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
		12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
		12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
		12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
		12,12,12,12,12,12,12,12,12,12,12,12,12
	};
	static const uint8_t DMC_UNRAR_15_MAIN_CODE0[DMC_UNRAR_15_MAIN_CODE0_COUNT] = {
		4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,12,12,12,12,
		12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
		12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
		12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
		12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
		12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
		12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
		12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
		12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
		12,12,12,12,12,12,12,12,12,12,12,12
	};
	static const uint8_t DMC_UNRAR_15_MAIN_CODE1[DMC_UNRAR_15_MAIN_CODE1_COUNT] = {
		5,5,5,5,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
		6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,9,9,
		9,9,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,
		11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,12,12,12,12,
		12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
		12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
		12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
		12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
		12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12
	};
	static const uint8_t DMC_UNRAR_15_MAIN_CODE2[DMC_UNRAR_15_MAIN_CODE2_COUNT] = {
		5,5,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
		9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
		9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,10,
		10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10
	};
	static const uint8_t DMC_UNRAR_15_MAIN_CODE3[DMC_UNRAR_15_MAIN_CODE3_COUNT] = {
		6,6,7,7,7,7,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
		9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,10,10,10,10,10,10
	};
	static const uint8_t DMC_UNRAR_15_MAIN_CODE4[DMC_UNRAR_15_MAIN_CODE4_COUNT] = {
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,9,9
	};
	static const uint8_t DMC_UNRAR_15_SHORT_CODE0_LENGTHS[DMC_UNRAR_15_SHORT_CODE0_COUNT] = {
		1, 4, 4, 4, 5, 6, 7, 8, 8, 4, 4, 5, 6, 6, 4
	};
	static const uint32_t DMC_UNRAR_15_SHORT_CODE0_CODES[DMC_UNRAR_15_SHORT_CODE0_COUNT] = {
		0x00, 0x0A, 0x0D, 0x0E, 0x1E, 0x3E, 0x7E, 0xFE, 0xFF, 0x0C, 0x08, 0x12, 0x26, 0x27, 0x0B
	};
	static const uint32_t DMC_UNRAR_15_SHORT_CODE0_SYMBOLS[DMC_UNRAR_15_SHORT_CODE0_COUNT] = {
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14
	};
	static const uint8_t DMC_UNRAR_15_SHORT_CODE1_LENGTHS[DMC_UNRAR_15_SHORT_CODE1_COUNT] = {
		1, 3, 4, 4, 5, 6, 7, 8, 8, 4, 4, 5, 6, 6,
	};
	static const uint32_t DMC_UNRAR_15_SHORT_CODE1_CODES[DMC_UNRAR_15_SHORT_CODE1_COUNT] = {
		0x00, 0x05, 0x0D, 0x0E, 0x1E, 0x3E, 0x7E, 0xFE, 0xFF, 0x0C, 0x08, 0x12, 0x26, 0x27,
	};
	static const uint32_t DMC_UNRAR_15_SHORT_CODE1_SYMBOLS[DMC_UNRAR_15_SHORT_CODE1_COUNT] = {
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
	};
	static const uint8_t DMC_UNRAR_15_SHORT_CODE2_LENGTHS[DMC_UNRAR_15_SHORT_CODE2_COUNT] = {
		2, 3, 3, 4, 4, 4, 5, 6, 6, 4, 4, 5, 6, 6, 4,
	};
	static const uint32_t DMC_UNRAR_15_SHORT_CODE2_CODES[DMC_UNRAR_15_SHORT_CODE2_COUNT] = {
		0x00, 0x02, 0x03, 0x0A, 0x0D, 0x3E, 0x1E, 0x3E, 0x3F, 0x0C, 0x08, 0x12, 0x26, 0x27, 0x0B,
	};
	static const uint32_t DMC_UNRAR_15_SHORT_CODE2_SYMBOLS[DMC_UNRAR_15_SHORT_CODE2_COUNT] = {
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
	};
	static const uint8_t DMC_UNRAR_15_SHORT_CODE3_LENGTHS[DMC_UNRAR_15_SHORT_CODE3_COUNT] = {
		2, 3, 3, 3, 4, 4, 5, 6, 6, 4, 4, 5, 6, 6,
	};
	static const uint32_t DMC_UNRAR_15_SHORT_CODE3_CODES[DMC_UNRAR_15_SHORT_CODE3_COUNT] = {
		0x00, 0x02, 0x03, 0x05, 0x0D, 0x0E, 0x1E, 0x3E, 0x3F, 0x0C, 0x08, 0x12, 0x26, 0x27,
	};
	static const uint32_t DMC_UNRAR_15_SHORT_CODE3_SYMBOLS[DMC_UNRAR_15_SHORT_CODE3_COUNT] = {
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
	};

	dmc_unrar_alloc *alloc = &ctx->ctx->archive->alloc;
	dmc_unrar_return return_code;

	return_code = dmc_unrar_huff_create_from_lengths(alloc, &ctx->huff_lengths1,
		DMC_UNRAR_15_LENGTH_CODE1, DMC_UNRAR_15_LENGTH_CODE_COUNT, DMC_UNRAR_15_LENGTH_CODE_LENGTH);

	if (return_code != DMC_UNRAR_OK)
		return return_code;

	return_code = dmc_unrar_huff_create_from_lengths(alloc, &ctx->huff_lengths2,
		DMC_UNRAR_15_LENGTH_CODE2, DMC_UNRAR_15_LENGTH_CODE_COUNT, DMC_UNRAR_15_LENGTH_CODE_LENGTH);

	if (return_code != DMC_UNRAR_OK)
		return return_code;


	return_code = dmc_unrar_huff_create_from_lengths(alloc, &ctx->huff_main0,
		DMC_UNRAR_15_MAIN_CODE0, DMC_UNRAR_15_MAIN_CODE0_COUNT, DMC_UNRAR_15_MAIN_CODE0_LENGTH);

	if (return_code != DMC_UNRAR_OK)
		return return_code;

	return_code = dmc_unrar_huff_create_from_lengths(alloc, &ctx->huff_main1,
		DMC_UNRAR_15_MAIN_CODE1, DMC_UNRAR_15_MAIN_CODE1_COUNT, DMC_UNRAR_15_MAIN_CODE1_LENGTH);

	if (return_code != DMC_UNRAR_OK)
		return return_code;

	return_code = dmc_unrar_huff_create_from_lengths(alloc, &ctx->huff_main2,
		DMC_UNRAR_15_MAIN_CODE2, DMC_UNRAR_15_MAIN_CODE2_COUNT, DMC_UNRAR_15_MAIN_CODE2_LENGTH);

	if (return_code != DMC_UNRAR_OK)
		return return_code;

	return_code = dmc_unrar_huff_create_from_lengths(alloc, &ctx->huff_main3,
		DMC_UNRAR_15_MAIN_CODE3, DMC_UNRAR_15_MAIN_CODE3_COUNT, DMC_UNRAR_15_MAIN_CODE3_LENGTH);

	if (return_code != DMC_UNRAR_OK)
		return return_code;

	return_code = dmc_unrar_huff_create_from_lengths(alloc, &ctx->huff_main4,
		DMC_UNRAR_15_MAIN_CODE4, DMC_UNRAR_15_MAIN_CODE4_COUNT, DMC_UNRAR_15_MAIN_CODE4_LENGTH);

	if (return_code != DMC_UNRAR_OK)
		return return_code;


	return_code = dmc_unrar_huff_create(alloc, &ctx->huff_short0, 0,
		DMC_UNRAR_15_SHORT_CODE0_COUNT, DMC_UNRAR_15_SHORT_CODE0_CODES,
		DMC_UNRAR_15_SHORT_CODE0_LENGTHS, DMC_UNRAR_15_SHORT_CODE0_SYMBOLS);

	if (return_code != DMC_UNRAR_OK)
		return return_code;

	return_code = dmc_unrar_huff_create(alloc, &ctx->huff_short1, 0,
		DMC_UNRAR_15_SHORT_CODE1_COUNT, DMC_UNRAR_15_SHORT_CODE1_CODES,
		DMC_UNRAR_15_SHORT_CODE1_LENGTHS, DMC_UNRAR_15_SHORT_CODE1_SYMBOLS);

	if (return_code != DMC_UNRAR_OK)
		return return_code;

	return_code = dmc_unrar_huff_create(alloc, &ctx->huff_short2, 0,
		DMC_UNRAR_15_SHORT_CODE2_COUNT, DMC_UNRAR_15_SHORT_CODE2_CODES,
		DMC_UNRAR_15_SHORT_CODE2_LENGTHS, DMC_UNRAR_15_SHORT_CODE2_SYMBOLS);

	if (return_code != DMC_UNRAR_OK)
		return return_code;

	return_code = dmc_unrar_huff_create(alloc, &ctx->huff_short3, 0,
		DMC_UNRAR_15_SHORT_CODE3_COUNT, DMC_UNRAR_15_SHORT_CODE3_CODES,
		DMC_UNRAR_15_SHORT_CODE3_LENGTHS, DMC_UNRAR_15_SHORT_CODE3_SYMBOLS);

	if (return_code != DMC_UNRAR_OK)
		return return_code;


	return DMC_UNRAR_OK;
}

static bool dmc_unrar_rar15_get_flag_bit(dmc_unrar_rar15_context *ctx, dmc_unrar_return *err);
static dmc_unrar_return dmc_unrar_rar15_decode_literal(dmc_unrar_rar15_context *ctx);
static dmc_unrar_return dmc_unrar_rar15_decode_short_match(dmc_unrar_rar15_context *ctx);
static dmc_unrar_return dmc_unrar_rar15_decode_long_match(dmc_unrar_rar15_context *ctx);

static dmc_unrar_return dmc_unrar_rar15_decompress(dmc_unrar_rar15_context *ctx) {
	dmc_unrar_return return_code;

	if (ctx->ctx->solid_offset == 0) {
		ctx->flag_bits = 0;
		ctx->stored_block = false;
		ctx->repeat_last_match_count = 0;
	}

	while (ctx->ctx->buffer_offset < ctx->ctx->buffer_size) {
		if (dmc_unrar_lzss_has_overhang(&ctx->ctx->lzss)) {
			ctx->ctx->buffer_offset = dmc_unrar_lzss_emit_copy_overhang(&ctx->ctx->lzss, ctx->ctx->buffer,
			                          ctx->ctx->buffer_size, ctx->ctx->buffer_offset, NULL);
			continue;
		}

		if (dmc_unrar_bs_has_error(&ctx->ctx->bs) || dmc_unrar_bs_eos(&ctx->ctx->bs))
			break;

		if (ctx->stored_block) {
			if ((return_code = dmc_unrar_rar15_decode_literal(ctx)) != DMC_UNRAR_OK)
				return return_code;

			continue;
		}

		{
			const bool expecting_match = ctx->match_weight > ctx->literal_weight;

			if (dmc_unrar_rar15_get_flag_bit(ctx, &return_code)) {
				if (return_code != DMC_UNRAR_OK)
					return return_code;

				/* The expected case. */

				if (expecting_match)
					return_code = dmc_unrar_rar15_decode_long_match(ctx);
				else
					return_code = dmc_unrar_rar15_decode_literal(ctx);

			} else if (dmc_unrar_rar15_get_flag_bit(ctx, &return_code)) {
				if (return_code != DMC_UNRAR_OK)
					return return_code;

				/* The unexpected case. */

				if (!expecting_match)
					return_code = dmc_unrar_rar15_decode_long_match(ctx);
				else
					return_code = dmc_unrar_rar15_decode_literal(ctx);

			} else {
				if (return_code != DMC_UNRAR_OK)
					return return_code;

				return_code = dmc_unrar_rar15_decode_short_match(ctx);
			}

			if (return_code != DMC_UNRAR_OK)
				return return_code;
		}
	}

	return dmc_unrar_bs_has_error(&ctx->ctx->bs) ? DMC_UNRAR_READ_FAIL : DMC_UNRAR_OK;
}

static int dmc_unrar_rar15_lookup_byte(unsigned int *table, unsigned int *reverse,
		unsigned int limit, size_t index) {

	unsigned int val = table[index];
	size_t new_index = reverse[val & 0xFF]++;

	if ((val & 0xFF) >= limit) {
		dmc_unrar_rar15_reset_table(table, reverse);

		val = table[index];
		new_index = reverse[val & 0xFF]++;
	}

	table[index] = table[new_index];
	table[new_index] = val + 1;

	return val >> 8;
}

static bool dmc_unrar_rar15_get_flag_bit(dmc_unrar_rar15_context *ctx, dmc_unrar_return *err) {
	*err = DMC_UNRAR_OK;

	if (ctx->flag_bits == 0) {
		const uint32_t index = dmc_unrar_huff_get_symbol(&ctx->huff_main2, &ctx->ctx->bs, err);
		if ((*err == DMC_UNRAR_OK) && (index == 256))
			*err = DMC_UNRAR_15_INVALID_FLAG_INDEX;

		if (*err != DMC_UNRAR_OK)
			return true;

		ctx->flags     = dmc_unrar_rar15_lookup_byte(ctx->flag_table, ctx->flag_reverse, 0xFF, index);
		ctx->flag_bits = 8;
	}

	ctx->flag_bits--;

	return (ctx->flags >> ctx->flag_bits) & 1;
}

static dmc_unrar_return dmc_unrar_rar15_decode_literal(dmc_unrar_rar15_context *ctx) {
	dmc_unrar_return return_code;
	uint32_t index;

	if      (ctx->running_average_literal < 0x0E00)
		index = dmc_unrar_huff_get_symbol(&ctx->huff_main0, &ctx->ctx->bs, &return_code);
	else if (ctx->running_average_literal < 0x3600)
		index = dmc_unrar_huff_get_symbol(&ctx->huff_main1, &ctx->ctx->bs, &return_code);
	else if (ctx->running_average_literal < 0x5E00)
		index = dmc_unrar_huff_get_symbol(&ctx->huff_main2, &ctx->ctx->bs, &return_code);
	else if (ctx->running_average_literal < 0x7600)
		index = dmc_unrar_huff_get_symbol(&ctx->huff_main3, &ctx->ctx->bs, &return_code);
	else
		index = dmc_unrar_huff_get_symbol(&ctx->huff_main4, &ctx->ctx->bs, &return_code);

	if (return_code != DMC_UNRAR_OK)
		return return_code;

	if (ctx->stored_block) {
		if (index == 0) {
			size_t length, offset;

			if (dmc_unrar_bs_read_bits(&ctx->ctx->bs, 1)) {
				ctx->stored_block = false;
				ctx->repeat_literal_count = 0;

				return dmc_unrar_bs_has_error(&ctx->ctx->bs) ? DMC_UNRAR_READ_FAIL : DMC_UNRAR_OK;
			}

			length = dmc_unrar_bs_read_bits(&ctx->ctx->bs, 1) ? 4 : 3;

			offset  = dmc_unrar_huff_get_symbol(&ctx->huff_main2, &ctx->ctx->bs, &return_code) << 5;
			offset |= dmc_unrar_bs_read_bits(&ctx->ctx->bs, 5);

			if (return_code != DMC_UNRAR_OK)
				return return_code;

			ctx->ctx->buffer_offset = dmc_unrar_lzss_emit_copy(&ctx->ctx->lzss, ctx->ctx->buffer,
			                          ctx->ctx->buffer_size, ctx->ctx->buffer_offset, offset, length, NULL);

			return dmc_unrar_bs_has_error(&ctx->ctx->bs) ? DMC_UNRAR_READ_FAIL : DMC_UNRAR_OK;

		} else
			index--;

	} else {
		index &= 0xFF;

		if ((ctx->repeat_literal_count++ >= 16) && (ctx->flag_bits == 0))
			ctx->stored_block = true;
	}

	ctx->running_average_literal += index;
	ctx->running_average_literal -= ctx->running_average_literal >> 8;

	ctx->literal_weight += 16;
	if (ctx->literal_weight > 0xFF) {
		ctx->literal_weight = 0x90;
		ctx->match_weight >>= 1;
	}

	{
		const uint8_t literal = dmc_unrar_rar15_lookup_byte(ctx->literal_table, ctx->literal_reverse,
		                                                    0xA1, index);

		ctx->ctx->buffer_offset = dmc_unrar_lzss_emit_literal(&ctx->ctx->lzss, ctx->ctx->buffer,
		                          ctx->ctx->buffer_size, ctx->ctx->buffer_offset, literal, NULL);
	}

	return dmc_unrar_bs_has_error(&ctx->ctx->bs) ? DMC_UNRAR_READ_FAIL : DMC_UNRAR_OK;
}

static dmc_unrar_return dmc_unrar_rar15_decode_short_match(dmc_unrar_rar15_context *ctx) {
	dmc_unrar_return return_code;
	uint32_t selector;

	ctx->repeat_literal_count = 0;

	if (ctx->repeat_last_match_count == 2) {
		if (dmc_unrar_bs_read_bits(&ctx->ctx->bs, 1)) {
			ctx->ctx->buffer_offset = dmc_unrar_lzss_emit_copy(&ctx->ctx->lzss, ctx->ctx->buffer,
			                          ctx->ctx->buffer_size, ctx->ctx->buffer_offset, ctx->last_offset,
			                          ctx->last_length, NULL);

			return dmc_unrar_bs_has_error(&ctx->ctx->bs) ? DMC_UNRAR_READ_FAIL : DMC_UNRAR_OK;

		} else
			ctx->repeat_last_match_count = 0;
	}

	if (ctx->running_average_selector < 37) {
		if (ctx->bug_fix_flag)
			selector = dmc_unrar_huff_get_symbol(&ctx->huff_short0, &ctx->ctx->bs, &return_code);
		else
			selector = dmc_unrar_huff_get_symbol(&ctx->huff_short1, &ctx->ctx->bs, &return_code);

	} else {
		if (ctx->bug_fix_flag)
			selector = dmc_unrar_huff_get_symbol(&ctx->huff_short2, &ctx->ctx->bs, &return_code);
		else
			selector = dmc_unrar_huff_get_symbol(&ctx->huff_short3, &ctx->ctx->bs, &return_code);
	}

	if (return_code != DMC_UNRAR_OK)
		return return_code;

	if (selector < 9) {
		size_t offset_index, offset, length;

		ctx->repeat_last_match_count = 0;

		ctx->running_average_selector += selector;
		ctx->running_average_selector -= ctx->running_average_selector >> 4;

		offset_index = dmc_unrar_huff_get_symbol(&ctx->huff_main2, &ctx->ctx->bs, &return_code) & 0xFF;
		if (return_code != DMC_UNRAR_OK)
			return return_code;

		offset = ctx->short_offset_table[offset_index];
		if (offset_index != 0) {
			ctx->short_offset_table[offset_index]     = ctx->short_offset_table[offset_index - 1];
			ctx->short_offset_table[offset_index - 1] = offset;
		}
		offset++;

		length = selector + 2;

		ctx->last_offset = ctx->old_offset[ctx->old_offset_index++ & 3] = offset;
		ctx->last_length = length;

		ctx->ctx->buffer_offset = dmc_unrar_lzss_emit_copy(&ctx->ctx->lzss, ctx->ctx->buffer,
		                          ctx->ctx->buffer_size, ctx->ctx->buffer_offset, offset, length, NULL);

		return dmc_unrar_bs_has_error(&ctx->ctx->bs) ? DMC_UNRAR_READ_FAIL : DMC_UNRAR_OK;
	}

	if (selector == 9) {
		ctx->repeat_last_match_count++;

		ctx->ctx->buffer_offset = dmc_unrar_lzss_emit_copy(&ctx->ctx->lzss, ctx->ctx->buffer,
		                          ctx->ctx->buffer_size, ctx->ctx->buffer_offset, ctx->last_offset,
		                          ctx->last_length, NULL);

		return dmc_unrar_bs_has_error(&ctx->ctx->bs) ? DMC_UNRAR_READ_FAIL : DMC_UNRAR_OK;
	}

	if (selector < 14) {
		size_t offset, length;

		ctx->repeat_last_match_count = 0;

		offset = ctx->old_offset[(ctx->old_offset_index - (selector - 9)) & 3];

		length = dmc_unrar_huff_get_symbol(&ctx->huff_lengths1, &ctx->ctx->bs, &return_code) + 2;
		if (return_code != DMC_UNRAR_OK)
			return return_code;

		if ((length == 0x101) && (selector == 10)) {
			ctx->bug_fix_flag = !ctx->bug_fix_flag;

			return dmc_unrar_bs_has_error(&ctx->ctx->bs) ? DMC_UNRAR_READ_FAIL : DMC_UNRAR_OK;
		}

		if (offset > 256)
			length++;

		if (offset >= ctx->maximum_offset)
			length++;

		ctx->last_offset = ctx->old_offset[ctx->old_offset_index++ & 3] = offset;
		ctx->last_length = length;

		ctx->ctx->buffer_offset = dmc_unrar_lzss_emit_copy(&ctx->ctx->lzss, ctx->ctx->buffer,
		                          ctx->ctx->buffer_size, ctx->ctx->buffer_offset, offset, length, NULL);

		return dmc_unrar_bs_has_error(&ctx->ctx->bs) ? DMC_UNRAR_READ_FAIL : DMC_UNRAR_OK;
	}

	ctx->repeat_last_match_count = 0;

	{
		const size_t length = dmc_unrar_huff_get_symbol(&ctx->huff_lengths2, &ctx->ctx->bs, &return_code) + 5;
		const size_t offset = dmc_unrar_bs_read_bits(&ctx->ctx->bs, 15) + 0x8000;

		if (return_code != DMC_UNRAR_OK)
			return return_code;

		ctx->last_offset = offset;
		ctx->last_length = length;

		ctx->ctx->buffer_offset = dmc_unrar_lzss_emit_copy(&ctx->ctx->lzss, ctx->ctx->buffer,
		                          ctx->ctx->buffer_size, ctx->ctx->buffer_offset, offset, length, NULL);
	}

	return dmc_unrar_bs_has_error(&ctx->ctx->bs) ? DMC_UNRAR_READ_FAIL : DMC_UNRAR_OK;
}

static dmc_unrar_return dmc_unrar_rar15_decode_long_match(dmc_unrar_rar15_context *ctx) {
	dmc_unrar_return return_code = DMC_UNRAR_OK;
	size_t raw_length, offset_index, offset, length;

	ctx->repeat_literal_count = 0;
	ctx->match_weight += 16;

	if (ctx->match_weight > 0xFF) {
		ctx->match_weight = 0x90;
		ctx->literal_weight >>= 1;
	}

	if      (ctx->running_average_length >= 122)
		raw_length = dmc_unrar_huff_get_symbol(&ctx->huff_lengths2, &ctx->ctx->bs, &return_code);
	else if (ctx->running_average_length >=  64)
		raw_length = dmc_unrar_huff_get_symbol(&ctx->huff_lengths1, &ctx->ctx->bs, &return_code);
	else {
		raw_length = 0;

		while ((raw_length < 8) && (dmc_unrar_bs_read_bits(&ctx->ctx->bs, 1) == 0))
			raw_length++;

		if (raw_length == 8)
			raw_length = dmc_unrar_bs_read_bits(&ctx->ctx->bs, 8);
	}

	if (return_code != DMC_UNRAR_OK)
		return return_code;

	if      (ctx->running_average_offset < 0x0700)
		offset_index = dmc_unrar_huff_get_symbol(&ctx->huff_main0, &ctx->ctx->bs, &return_code);
	else if (ctx->running_average_offset < 0x2900)
		offset_index = dmc_unrar_huff_get_symbol(&ctx->huff_main1, &ctx->ctx->bs, &return_code);
	else
		offset_index = dmc_unrar_huff_get_symbol(&ctx->huff_main2, &ctx->ctx->bs, &return_code);

	if ((return_code == DMC_UNRAR_OK) && (offset_index == 0x100))
		return_code = DMC_UNRAR_15_INVALID_LONG_MATCH_OFFSET_INDEX;

	if (return_code != DMC_UNRAR_OK)
		return return_code;

	offset  = dmc_unrar_rar15_lookup_byte(ctx->offset_table, ctx->offset_reverse, 0xFF, offset_index) << 7;
	offset |= dmc_unrar_bs_read_bits(&ctx->ctx->bs, 7);

	length = raw_length + 3;

	if (offset >= ctx->maximum_offset)
		length++;

	if (offset <= 256)
		length += 8;

	if ((ctx->running_average_below_maximum > 0xB0) ||
	    (ctx->running_average_literal >= 0x2A00 && ctx->running_average_length < 0x40))
		ctx->maximum_offset = 0x7F00;
	else
		ctx->maximum_offset = 0x2001;

	ctx->running_average_length += raw_length;
	ctx->running_average_length -= ctx->running_average_length >> 5;

	ctx->running_average_offset += offset_index;
	ctx->running_average_offset -= ctx->running_average_offset >> 8;

	if ((raw_length == 0) && (offset <= ctx->maximum_offset)) {
		ctx->running_average_below_maximum++;
		ctx->running_average_below_maximum -= ctx->running_average_below_maximum >> 8;

	} else if ((raw_length != 1) && (raw_length != 4)) {
		if (ctx->running_average_below_maximum > 0)
			ctx->running_average_below_maximum--;
	}

	ctx->last_offset = ctx->old_offset[ctx->old_offset_index++ & 3] = offset;
	ctx->last_length = length;

	ctx->ctx->buffer_offset = dmc_unrar_lzss_emit_copy(&ctx->ctx->lzss, ctx->ctx->buffer,
	                          ctx->ctx->buffer_size, ctx->ctx->buffer_offset, ctx->last_offset,
	                          ctx->last_length, NULL);

	return dmc_unrar_bs_has_error(&ctx->ctx->bs) ? DMC_UNRAR_READ_FAIL : DMC_UNRAR_OK;
}
/* '--- */

/* .--- Unpacking RAR 2.0/2.6 */
#define DMC_UNRAR_20_MAX_AUDIO_CHANNELS   4
#define DMC_UNRAR_20_AUDIO_CODE_COUNT   257

#define DMC_UNRAR_20_MAX_CODE_COUNT (DMC_UNRAR_20_MAX_AUDIO_CHANNELS * DMC_UNRAR_20_AUDIO_CODE_COUNT)

typedef struct dmc_unrar_rar20_audio_state_tag {
	size_t count;

	int last_delta;

	int weight[5];
	int delta[4];
	int error[11];

	int last_byte;

} dmc_unrar_rar20_audio_state;

typedef struct dmc_unrar_rar20_context_tag {
	dmc_unrar_rar_context *ctx;

	bool initialized;

	uint8_t length_table[DMC_UNRAR_20_MAX_CODE_COUNT];

	bool is_audio_block;

	size_t channel_count;
	size_t current_channel;
	int channel_delta;

	dmc_unrar_rar20_audio_state audio_state[DMC_UNRAR_20_MAX_AUDIO_CHANNELS];

	size_t last_offset;
	size_t last_length;

	size_t old_offset[4];
	size_t old_offset_index;

	dmc_unrar_huff huff_main;
	dmc_unrar_huff huff_offset;
	dmc_unrar_huff huff_length;
	dmc_unrar_huff huff_audio[DMC_UNRAR_20_MAX_AUDIO_CHANNELS];

} dmc_unrar_rar20_context;

static void dmc_unrar_rar20_destroy(dmc_unrar_rar_context *ctx);
static dmc_unrar_return dmc_unrar_rar20_unpack(dmc_unrar_rar_context *ctx);

static dmc_unrar_return dmc_unrar_rar20_create(dmc_unrar_rar_context *ctx,
		dmc_unrar_archive *archive, dmc_unrar_file_block *file) {

	dmc_unrar_return return_code;

	DMC_UNRAR_ASSERT(ctx && archive && file);

	if (dmc_unrar_rar_context_check(ctx, archive, file, &return_code))
		return DMC_UNRAR_OK;

	if (return_code != DMC_UNRAR_OK)
		return return_code;

	DMC_UNRAR_ASSERT(ctx->archive == archive);

	ctx->has_end_marker = false;

	ctx->internal_state = dmc_unrar_malloc(&archive->alloc, 1, sizeof(dmc_unrar_rar20_context));
	if (!ctx->internal_state)
		return DMC_UNRAR_ALLOC_FAIL;

	ctx->destroy = &dmc_unrar_rar20_destroy;
	ctx->unpack  = &dmc_unrar_rar20_unpack;

	{
		dmc_unrar_rar20_context *ictx = (dmc_unrar_rar20_context *)ctx->internal_state;
		DMC_UNRAR_CLEAR_OBJ(*ictx);

		ictx->ctx = ctx;
	}

	return DMC_UNRAR_OK;
}

static void dmc_unrar_rar20_free_codes(dmc_unrar_rar20_context *ctx);

static void dmc_unrar_rar20_destroy(dmc_unrar_rar_context *ctx) {
	if (!ctx || !ctx->internal_state)
		return;

	{
		dmc_unrar_rar20_context *ictx = (dmc_unrar_rar20_context *)ctx->internal_state;

		dmc_unrar_rar20_free_codes(ictx);

		if (ctx->archive)
			dmc_unrar_free(&ctx->archive->alloc, ictx);

		ctx->internal_state = NULL;
	}
}

static dmc_unrar_return dmc_unrar_rar20_decompress(dmc_unrar_rar20_context *ctx);

static dmc_unrar_return dmc_unrar_rar20_unpack(dmc_unrar_rar_context *ctx) {
	DMC_UNRAR_ASSERT(ctx && ctx->internal_state);

	return dmc_unrar_rar20_decompress((dmc_unrar_rar20_context *)ctx->internal_state);
}

static void dmc_unrar_rar20_free_codes(dmc_unrar_rar20_context *ctx) {
	size_t i;

	DMC_UNRAR_ASSERT(ctx);

	dmc_unrar_huff_destroy(&ctx->huff_main);
	dmc_unrar_huff_destroy(&ctx->huff_offset);
	dmc_unrar_huff_destroy(&ctx->huff_length);

	for (i = 0; i < DMC_UNRAR_20_MAX_AUDIO_CHANNELS; i++)
		dmc_unrar_huff_destroy(&ctx->huff_audio[i]);
}

static dmc_unrar_return dmc_unrar_rar20_read_codes(dmc_unrar_rar20_context *ctx);

static uint8_t dmc_unrar_rar20_decode_audio(dmc_unrar_rar20_audio_state *state,
	int *channel_delta, int cur_delta);

/** Number of RAR 2.0/2.6 main Huffman codes. */
#define DMC_UNRAR_20_MAIN_CODE_COUNT   298
/** Number of RAR 2.0/2.6 LZSS offset Huffman codes. */
#define DMC_UNRAR_20_OFFSET_CODE_COUNT  48
/** Number of RAR 2.0/2.6 LZSS length Huffman codes. */
#define DMC_UNRAR_20_LENGTH_CODE_COUNT  28
/** Number of RAR 2.0/2.6 small immediate codes. */
#define DMC_UNRAR_20_SMALL_CODE_COUNT    8

/** Max lengths of all RAR 2.0 huffman codes. */
#define DMC_UNRAR_20_CODE_LENGTH    15
/** Number of codes in the RAR 2.0 codebook code. */
#define DMC_UNRAR_20_PRE_CODE_COUNT 19

static const size_t DMC_UNRAR_20_LENGTH_BASES[DMC_UNRAR_20_LENGTH_CODE_COUNT] = {
	  0,   1,   2,   3,   4,   5,   6,   7,   8,  10,  12,  14,  16,  20,
	 24,  28,  32,  40,  48,  56,  64,  80,  96, 112, 128, 160, 192, 224
};
static const uint8_t DMC_UNRAR_20_LENGTH_BITS[DMC_UNRAR_20_LENGTH_CODE_COUNT] = {
	  0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   1,   1,   2,   2,
	  2,   2,   3,   3,   3,   3,   4,   4,   4,   4,   5,   5,   5,   5
};
static const size_t DMC_UNRAR_20_OFFSET_BASES[DMC_UNRAR_20_OFFSET_CODE_COUNT] = {
	     0,      1,      2,      3,      4,      6,      8,     12,     16,     24,     32,     48,
	    64,     96,    128,    192,    256,    384,    512,    768,   1024,   1536,   2048,   3072,
	  4096,   6144,   8192,  12288,  16384,  24576,  32768,  49152,  65536,  98304, 131072, 196608,
	262144, 327680, 393216, 458752, 524288, 589824, 655360, 720896, 786432, 851968, 917504, 983040
};
static const uint8_t DMC_UNRAR_20_OFFSET_BITS[DMC_UNRAR_20_OFFSET_CODE_COUNT] = {
	     0,      0,      0,      0,      1,      1,      2,      2,      3,      3,      4,      4,
	     5,      5,      6,      6,      7,      7,      8,      8,      9,      9,     10,     10,
	    11,     11,     12,     12,     13,     13,     14,     14,     15,     15,     16,     16,
	    16,     16,     16,     16,     16,     16,     16,     16,     16,     16,     16,     16
};
static const size_t DMC_UNRAR_20_SHORT_BASES[DMC_UNRAR_20_SMALL_CODE_COUNT] = {
	  0,   4,   8,  16,  32,  64, 128, 192
};
static const uint8_t DMC_UNRAR_20_SHORT_BITS[DMC_UNRAR_20_SMALL_CODE_COUNT] = {
	  2,   2,   3,   4,   5,   6,   6,   6
};

static dmc_unrar_return dmc_unrar_rar20_decompress(dmc_unrar_rar20_context *ctx) {
	dmc_unrar_return return_code = DMC_UNRAR_OK;

	DMC_UNRAR_ASSERT(ctx);

	if (!ctx->initialized)
		if ((return_code = dmc_unrar_rar20_read_codes(ctx)) != DMC_UNRAR_OK)
			return return_code;

	ctx->initialized = true;

	while (ctx->ctx->buffer_offset < ctx->ctx->buffer_size) {
		uint8_t literal;
		uint32_t symbol;
		size_t offset, length;

		if (dmc_unrar_lzss_has_overhang(&ctx->ctx->lzss)) {
			ctx->ctx->buffer_offset = dmc_unrar_lzss_emit_copy_overhang(&ctx->ctx->lzss, ctx->ctx->buffer,
			                          ctx->ctx->buffer_size, ctx->ctx->buffer_offset, NULL);
			continue;
		}

		if (dmc_unrar_bs_has_error(&ctx->ctx->bs) || dmc_unrar_bs_eos(&ctx->ctx->bs))
			break;

		if (ctx->is_audio_block) {
			symbol = dmc_unrar_huff_get_symbol(&ctx->huff_audio[ctx->current_channel], &ctx->ctx->bs, &return_code);
			if (return_code != DMC_UNRAR_OK)
				return return_code;

			if (symbol == 256) {
				/* New codebook. */

				if ((return_code = dmc_unrar_rar20_read_codes(ctx)) != DMC_UNRAR_OK)
					return return_code;

				continue;
			}

			literal = dmc_unrar_rar20_decode_audio(&ctx->audio_state[ctx->current_channel],
			                                       &ctx->channel_delta, symbol);

			ctx->current_channel = (ctx->current_channel + 1) % ctx->channel_count;

			ctx->ctx->buffer_offset = dmc_unrar_lzss_emit_literal(&ctx->ctx->lzss, ctx->ctx->buffer,
			                          ctx->ctx->buffer_size, ctx->ctx->buffer_offset, literal, NULL);

			continue;
		}

		symbol = dmc_unrar_huff_get_symbol(&ctx->huff_main, &ctx->ctx->bs, &return_code);
		if (return_code != DMC_UNRAR_OK)
			return return_code;

		if (symbol < 256) {
			/* Literal. */

			ctx->ctx->buffer_offset = dmc_unrar_lzss_emit_literal(&ctx->ctx->lzss, ctx->ctx->buffer,
			                          ctx->ctx->buffer_size, ctx->ctx->buffer_offset, symbol, NULL);
			continue;
		}

		if (symbol == 269) {
			/* New codebook. */

			if ((return_code = dmc_unrar_rar20_read_codes(ctx)) != DMC_UNRAR_OK)
				return return_code;

			continue;
		}

		/* Copy, back-reference. */

		if        (symbol == 256) {
			/* Reuse the last offset and length. */

			offset = ctx->last_offset;
			length = ctx->last_length;

		} else if (symbol <= 260) {
			/* Reuse a previous offset, but new length. */

			const uint32_t length_symbol = dmc_unrar_huff_get_symbol(&ctx->huff_length, &ctx->ctx->bs, &return_code);
			if (return_code != DMC_UNRAR_OK)
				return return_code;

			offset = ctx->old_offset[(ctx->old_offset_index - (symbol - 256)) & 3];
			length = DMC_UNRAR_20_LENGTH_BASES[length_symbol] + 2;

			length += dmc_unrar_bs_read_bits(&ctx->ctx->bs, DMC_UNRAR_20_LENGTH_BITS[length_symbol]);

			if (offset >= 0x40000) length++;
			if (offset >= 0x02000) length++;
			if (offset >= 0x00101) length++;

		} else if (symbol <= 268) {
			/* Duplicate a short, recent piece. */

			offset = DMC_UNRAR_20_SHORT_BASES[symbol - 261] + 1;
			length = 2;

			offset += dmc_unrar_bs_read_bits(&ctx->ctx->bs, DMC_UNRAR_20_SHORT_BITS[symbol - 261]);

		} else { /* symbol >= 270. */
			/* New offset, new length. */

			uint32_t offset_symbol;

			length  = DMC_UNRAR_20_LENGTH_BASES[symbol - 270] + 3;
			length += dmc_unrar_bs_read_bits(&ctx->ctx->bs, DMC_UNRAR_20_LENGTH_BITS[symbol - 270]);

			offset_symbol = dmc_unrar_huff_get_symbol(&ctx->huff_offset, &ctx->ctx->bs, &return_code);
			if (return_code != DMC_UNRAR_OK)
				return return_code;

			offset = DMC_UNRAR_20_OFFSET_BASES[offset_symbol] + 1;

			offset += dmc_unrar_bs_read_bits(&ctx->ctx->bs, DMC_UNRAR_20_OFFSET_BITS[offset_symbol]);

			if (offset >= 0x40000) length++;
			if (offset >= 0x02000) length++;
		}

		/* Remember the last offsets and length. */
		ctx->last_offset = ctx->old_offset[ctx->old_offset_index++ & 3] = offset;
		ctx->last_length = length;

		ctx->ctx->buffer_offset = dmc_unrar_lzss_emit_copy(&ctx->ctx->lzss, ctx->ctx->buffer,
		                          ctx->ctx->buffer_size, ctx->ctx->buffer_offset, offset, length, NULL);
	}

	return dmc_unrar_bs_has_error(&ctx->ctx->bs) ? DMC_UNRAR_READ_FAIL : DMC_UNRAR_OK;
}

static dmc_unrar_return dmc_unrar_rar20_read_codes(dmc_unrar_rar20_context *ctx) {
	dmc_unrar_return return_code = DMC_UNRAR_OK;

	dmc_unrar_huff huff_pre;
	uint8_t pre_lengths[DMC_UNRAR_20_PRE_CODE_COUNT];

	size_t count, i, j;

	ctx->is_audio_block = dmc_unrar_bs_read_bits(&ctx->ctx->bs, 1);

	if (dmc_unrar_bs_read_bits(&ctx->ctx->bs, 1) == 0)
		DMC_UNRAR_CLEAR_ARRAY(ctx->length_table);

	count = DMC_UNRAR_20_MAIN_CODE_COUNT + DMC_UNRAR_20_OFFSET_CODE_COUNT + DMC_UNRAR_20_LENGTH_CODE_COUNT;
	if (ctx->is_audio_block) {
		ctx->channel_count = dmc_unrar_bs_read_bits(&ctx->ctx->bs, 2) + 1;
		if (ctx->current_channel >= ctx->channel_count)
			ctx->current_channel = 0;

		DMC_UNRAR_ASSERT(ctx->channel_count <= DMC_UNRAR_20_MAX_AUDIO_CHANNELS);

		count = ctx->channel_count * DMC_UNRAR_20_AUDIO_CODE_COUNT;
	}

	for (i = 0; i < DMC_UNRAR_20_PRE_CODE_COUNT; i++)
		pre_lengths[i] = dmc_unrar_bs_read_bits(&ctx->ctx->bs, 4);

	return_code = dmc_unrar_huff_create_from_lengths(&ctx->ctx->archive->alloc, &huff_pre,
		pre_lengths, DMC_UNRAR_20_PRE_CODE_COUNT, DMC_UNRAR_20_CODE_LENGTH);

	if (return_code != DMC_UNRAR_OK)
		return return_code;

	for (i = 0; i < count; ) {
		const uint32_t val = dmc_unrar_huff_get_symbol(&huff_pre, &ctx->ctx->bs, &return_code);
		if (return_code != DMC_UNRAR_OK)
			goto end;

		if (val < 16) {
			ctx->length_table[i] = (ctx->length_table[i] + val) & 0x0F;
			i++;

		} else if (val == 16) {
			if (i == 0) {
				return_code = DMC_UNRAR_20_INVALID_LENGTH_TABLE_DATA;
				goto end;
			}

			{
				const size_t n = dmc_unrar_bs_read_bits(&ctx->ctx->bs, 2) + 3;
				for (j = 0; (j < n) && (i < count); j++, i++)
					ctx->length_table[i] = ctx->length_table[i - 1];
			}

		} else if (val == 17) {
			const size_t n = dmc_unrar_bs_read_bits(&ctx->ctx->bs, 3) + 3;
			for (j = 0; (j < n) && (i < count); j++, i++)
				ctx->length_table[i] = 0;

		} else {
			const size_t n = dmc_unrar_bs_read_bits(&ctx->ctx->bs, 7) + 11;
			for (j = 0; (j < n) && (i < count); j++, i++)
				ctx->length_table[i] = 0;
		}

	}

	dmc_unrar_rar20_free_codes(ctx);
	if (ctx->is_audio_block) {
		for (i = 0; i < ctx->channel_count; i++) {
			return_code = dmc_unrar_huff_create_from_lengths(&ctx->ctx->archive->alloc, &ctx->huff_audio[i],
				ctx->length_table + (i * DMC_UNRAR_20_AUDIO_CODE_COUNT), DMC_UNRAR_20_AUDIO_CODE_COUNT,
				DMC_UNRAR_20_CODE_LENGTH);

			if (return_code != DMC_UNRAR_OK)
				goto end;
		}

	} else {
		static const size_t main_code_offset   = 0;
		static const size_t offset_code_offset = DMC_UNRAR_20_MAIN_CODE_COUNT;
		static const size_t length_code_offset = DMC_UNRAR_20_MAIN_CODE_COUNT + DMC_UNRAR_20_OFFSET_CODE_COUNT;

		return_code = dmc_unrar_huff_create_from_lengths(&ctx->ctx->archive->alloc, &ctx->huff_main,
			ctx->length_table + main_code_offset, DMC_UNRAR_20_MAIN_CODE_COUNT, DMC_UNRAR_20_CODE_LENGTH);

		if (return_code != DMC_UNRAR_OK)
			goto end;

		return_code = dmc_unrar_huff_create_from_lengths(&ctx->ctx->archive->alloc, &ctx->huff_offset,
			ctx->length_table + offset_code_offset, DMC_UNRAR_20_OFFSET_CODE_COUNT, DMC_UNRAR_20_CODE_LENGTH);

		if (return_code != DMC_UNRAR_OK)
			goto end;

		return_code = dmc_unrar_huff_create_from_lengths(&ctx->ctx->archive->alloc, &ctx->huff_length,
			ctx->length_table + length_code_offset, DMC_UNRAR_20_LENGTH_CODE_COUNT, DMC_UNRAR_20_CODE_LENGTH);

		if (return_code != DMC_UNRAR_OK)
			goto end;
	}

end:
	dmc_unrar_huff_destroy(&huff_pre);
	return return_code;
}

static uint8_t dmc_unrar_rar20_decode_audio(dmc_unrar_rar20_audio_state *state,
		int *channel_delta, int cur_delta) {

	int pred_byte, cur_byte, pred_error;

	DMC_UNRAR_ASSERT(state && channel_delta);

	state->count++;

	state->delta[3] = state->delta[2];
	state->delta[2] = state->delta[1];
	state->delta[1] = state->last_delta - state->delta[0];
	state->delta[0] = state->last_delta;

	pred_byte = ((
		               8 * state->last_byte +
		state->weight[0] * state->delta[0] +
		state->weight[1] * state->delta[1] +
		state->weight[2] * state->delta[2] +
		state->weight[3] * state->delta[3] +
		state->weight[4] * *channel_delta
	) >> 3) & 0xFF;

	cur_byte = (pred_byte - cur_delta) & 0xFF;

	pred_error = ((int8_t)cur_delta) << 3;

	state->error[ 0] += DMC_UNRAR_ABS(pred_error);
	state->error[ 1] += DMC_UNRAR_ABS(pred_error - state->delta[0]);
	state->error[ 2] += DMC_UNRAR_ABS(pred_error + state->delta[0]);
	state->error[ 3] += DMC_UNRAR_ABS(pred_error - state->delta[1]);
	state->error[ 4] += DMC_UNRAR_ABS(pred_error + state->delta[1]);
	state->error[ 5] += DMC_UNRAR_ABS(pred_error - state->delta[2]);
	state->error[ 6] += DMC_UNRAR_ABS(pred_error + state->delta[2]);
	state->error[ 7] += DMC_UNRAR_ABS(pred_error - state->delta[3]);
	state->error[ 8] += DMC_UNRAR_ABS(pred_error + state->delta[3]);
	state->error[ 9] += DMC_UNRAR_ABS(pred_error - *channel_delta);
	state->error[10] += DMC_UNRAR_ABS(pred_error + *channel_delta);

	*channel_delta = state->last_delta = (int8_t)(cur_byte - state->last_byte);
	state->last_byte = cur_byte;

	if ((state->count & 0x1F) == 0) {
		int min_error = state->error[0];
		int min_index = 0;
		int i;

		for (i = 1; i < 11; i++) {
			if (state->error[i] < min_error) {
				min_error = state->error[i];
				min_index = i;
			}
		}

		DMC_UNRAR_CLEAR_ARRAY(state->error);

		switch (min_index) {
			case  1: if (state->weight[0] >=-16) state->weight[0]--; break;
			case  2: if (state->weight[0] <  16) state->weight[0]++; break;
			case  3: if (state->weight[1] >=-16) state->weight[1]--; break;
			case  4: if (state->weight[1] <  16) state->weight[1]++; break;
			case  5: if (state->weight[2] >=-16) state->weight[2]--; break;
			case  6: if (state->weight[2] <  16) state->weight[2]++; break;
			case  7: if (state->weight[3] >=-16) state->weight[3]--; break;
			case  8: if (state->weight[3] <  16) state->weight[3]++; break;
			case  9: if (state->weight[4] >=-16) state->weight[4]--; break;
			case 10: if (state->weight[4] <  16) state->weight[4]++; break;
		}
	}

	return cur_byte;
}
/* '--- */

/* .--- Unpacking RAR 2.9/3.6 */
/** Number of RAR 2.9/3.6 main Huffman codes. */
#define DMC_UNRAR_30_MAIN_CODE_COUNT 299
/** Number of RAR 2.9/3.6 LZSS offset Huffman codes. */
#define DMC_UNRAR_30_OFFSET_CODE_COUNT 60
/** Number of RAR 2.9/3.6 LZSS small offset Huffman codes. */
#define DMC_UNRAR_30_SMALL_CODE_COUNT 17
/** Number of RAR 2.9/3.6 LZSS length Huffman codes. */
#define DMC_UNRAR_30_LENGTH_CODE_COUNT 28

/** Number of codes in the RAR 2.9/3.6 codebook code. */
#define DMC_UNRAR_30_PRE_CODE_COUNT 20
/** Max lengths of all RAR 2.9/3.6 Huffman codes. */
#define DMC_UNRAR_30_CODE_LENGTH    15

/** Total number of RAR 2.9/3.6 Huffman codes. */
#define DMC_UNRAR_30_MAX_CODE_COUNT (DMC_UNRAR_30_MAIN_CODE_COUNT + DMC_UNRAR_30_OFFSET_CODE_COUNT + DMC_UNRAR_30_SMALL_CODE_COUNT + DMC_UNRAR_30_LENGTH_CODE_COUNT)

typedef struct dmc_unrar_rar30_context_tag {
	dmc_unrar_rar_context *ctx;

	bool start_new_table;

	size_t last_offset;
	size_t last_length;
	size_t old_offset[4];

	size_t last_small;
	size_t small_offset_repeat_count;

	bool is_ppmd_block;
	uint8_t ppmd_escape;
#if DMC_UNRAR_DISABLE_PPMD != 1
	dmc_unrar_ppmd ppmd;
#endif /* DMC_UNRAR_DISABLE_PPMD */

#if DMC_UNRAR_DISABLE_FILTERS != 1
	dmc_unrar_filters filters;
	uint8_t filter_buffer[DMC_UNRAR_FILTERS_BYTECODE_SIZE];
	size_t filter_output_length, filter_output_offset;
	size_t filter_overhang, filter_offset;
#endif /* DMC_UNRAR_DISABLE_FILTERS */

	uint8_t length_table[DMC_UNRAR_30_MAX_CODE_COUNT];

	dmc_unrar_huff huff_main;
	dmc_unrar_huff huff_offset;
	dmc_unrar_huff huff_small;
	dmc_unrar_huff huff_length;

} dmc_unrar_rar30_context;

static void dmc_unrar_rar30_destroy(dmc_unrar_rar_context *ctx);
static dmc_unrar_return dmc_unrar_rar30_unpack(dmc_unrar_rar_context *ctx);

static dmc_unrar_return dmc_unrar_rar30_create(dmc_unrar_rar_context *ctx,
		dmc_unrar_archive *archive, dmc_unrar_file_block *file) {

	dmc_unrar_return return_code;

	DMC_UNRAR_ASSERT(ctx && archive && file);

	if (dmc_unrar_rar_context_check(ctx, archive, file, &return_code))
		return DMC_UNRAR_OK;

	if (return_code != DMC_UNRAR_OK)
		return return_code;

	DMC_UNRAR_ASSERT(ctx->archive == archive);

	ctx->has_end_marker = true;

	ctx->internal_state = dmc_unrar_malloc(&archive->alloc, 1, sizeof(dmc_unrar_rar30_context));
	if (!ctx->internal_state)
		return DMC_UNRAR_ALLOC_FAIL;

	ctx->destroy = &dmc_unrar_rar30_destroy;
	ctx->unpack  = &dmc_unrar_rar30_unpack;

	{
		dmc_unrar_rar30_context *ictx = (dmc_unrar_rar30_context *)ctx->internal_state;
		DMC_UNRAR_CLEAR_OBJ(*ictx);

		ictx->ctx = ctx;

#if DMC_UNRAR_DISABLE_PPMD != 1
		{
			const dmc_unrar_return create_ppmd = dmc_unrar_ppmd_create(&archive->alloc, &ictx->ppmd);
			if (create_ppmd != DMC_UNRAR_OK)
				return create_ppmd;
		}
#endif /* DMC_UNRAR_DISABLE_PPMD */

#if DMC_UNRAR_DISABLE_FILTERS != 1
		{
			const dmc_unrar_return create_filters = dmc_unrar_filters_create(&archive->alloc, &ictx->filters);
			if (create_filters != DMC_UNRAR_OK)
				return create_filters;
		}
#endif /* DMC_UNRAR_DISABLE_FILTERS */

		ictx->start_new_table = true;
		ictx->ppmd_escape = 2;
	}

	return DMC_UNRAR_OK;
}

static void dmc_unrar_rar30_free_codes(dmc_unrar_rar30_context *ctx);

static void dmc_unrar_rar30_destroy(dmc_unrar_rar_context *ctx) {
	if (!ctx || !ctx->internal_state)
		return;

	{
		dmc_unrar_rar30_context *ictx = (dmc_unrar_rar30_context *)ctx->internal_state;

		dmc_unrar_rar30_free_codes(ictx);

#if DMC_UNRAR_DISABLE_PPMD != 1
		dmc_unrar_ppmd_destroy(&ictx->ppmd);
#endif
#if DMC_UNRAR_DISABLE_FILTERS != 1
		dmc_unrar_filters_destroy(&ictx->filters);
#endif

		if (ctx->archive)
			dmc_unrar_free(&ctx->archive->alloc, ictx);

		ctx->internal_state = NULL;
	}
}

static dmc_unrar_return dmc_unrar_rar30_decompress(dmc_unrar_rar30_context *ctx);

static dmc_unrar_return dmc_unrar_rar30_unpack(dmc_unrar_rar_context *ctx) {
	DMC_UNRAR_ASSERT(ctx && ctx->internal_state);

	return dmc_unrar_rar30_decompress((dmc_unrar_rar30_context *)ctx->internal_state);
}

static void dmc_unrar_rar30_free_codes(dmc_unrar_rar30_context *ctx) {
	if (!ctx)
		return;

	dmc_unrar_huff_destroy(&ctx->huff_main);
	dmc_unrar_huff_destroy(&ctx->huff_offset);
	dmc_unrar_huff_destroy(&ctx->huff_small);
	dmc_unrar_huff_destroy(&ctx->huff_length);
}

static dmc_unrar_return dmc_unrar_rar30_read_codes(dmc_unrar_rar30_context *ctx);

static dmc_unrar_return dmc_unrar_rar30_decode_huffman(dmc_unrar_rar30_context *ctx,
	uint8_t *buffer, size_t *buffer_offset, size_t buffer_size, size_t *running_output_count);
static dmc_unrar_return dmc_unrar_rar30_decode_ppmd(dmc_unrar_rar30_context *ctx,
	uint8_t *buffer, size_t *buffer_offset, size_t buffer_size, size_t *running_output_count);

static dmc_unrar_return dmc_unrar_rar30_decompress_block(dmc_unrar_rar30_context *ctx,
	uint8_t *buffer, size_t *buffer_offset, size_t buffer_size, size_t *running_output_count,
	bool stop_at_filter);

static dmc_unrar_return dmc_unrar_rar30_decompress(dmc_unrar_rar30_context *ctx) {
	dmc_unrar_return return_code;

	DMC_UNRAR_ASSERT(ctx);

	if (ctx->start_new_table)
		if ((return_code = dmc_unrar_rar30_read_codes(ctx)) != DMC_UNRAR_OK)
			return return_code;

	ctx->start_new_table = false;

#if DMC_UNRAR_DISABLE_FILTERS != 1
	while (ctx->ctx->buffer_offset < ctx->ctx->buffer_size) {
		const size_t current_offset = ctx->ctx->current_file_start + ctx->ctx->solid_offset + ctx->ctx->buffer_offset;

		if (ctx->filter_overhang > 0) {
			/* We still have filter output that goes to the decompression output buffer. */

			uint8_t *filter_memory = dmc_unrar_filters_get_memory(&ctx->filters);

			const size_t buffer_space = ctx->ctx->buffer_size - ctx->ctx->buffer_offset;
			const size_t copy_back = DMC_UNRAR_MIN(ctx->filter_overhang, buffer_space);

			if (ctx->ctx->buffer)
				memcpy(ctx->ctx->buffer + ctx->ctx->buffer_offset,
				       filter_memory + ctx->filter_output_offset, copy_back);

			ctx->ctx->buffer_offset   += copy_back;
			ctx->filter_output_offset += copy_back;

			ctx->filter_overhang      -= copy_back;
			ctx->filter_output_length -= copy_back;

			continue;
		}

		if (ctx->filter_output_length > 0) {
			/* We have filter output that's the input for the next filter.
			 * Move it to the front of the filter memory. */

			uint8_t *filter_memory = dmc_unrar_filters_get_memory(&ctx->filters);

			memmove(filter_memory, filter_memory + ctx->filter_output_offset, ctx->filter_output_length);

			ctx->filter_offset = ctx->filter_output_length;

			ctx->filter_output_offset = 0;
			ctx->filter_output_length = 0;

			ctx->filter_overhang = 0;
		}

		if (!dmc_unrar_lzss_has_overhang(&ctx->ctx->lzss) &&
			  (dmc_unrar_bs_eos(&ctx->ctx->bs) || ctx->ctx->start_new_file))
			break;

		if (current_offset < dmc_unrar_filters_get_first_offset(&ctx->filters)) {
			/* We're before the first filter position (or have no filter at all), so
			 * we can directly decode into the output buffer. */

			DMC_UNRAR_ASSERT(ctx->filter_offset == 0);

			return_code = dmc_unrar_rar30_decompress_block(ctx, ctx->ctx->buffer, &ctx->ctx->buffer_offset,
			              ctx->ctx->buffer_size, &ctx->ctx->output_offset, true);

			if (return_code != DMC_UNRAR_OK)
				return return_code;

			continue;
		}

		/* Now we should have filters and be at the first filter position. */

		DMC_UNRAR_ASSERT(!dmc_unrar_filters_empty(&ctx->filters));
		DMC_UNRAR_ASSERT(dmc_unrar_filters_get_first_length(&ctx->filters) > 0);
		DMC_UNRAR_ASSERT(current_offset == dmc_unrar_filters_get_first_offset(&ctx->filters));

		/* Decode into the filter memory. */

		{
			uint8_t *filter_memory = dmc_unrar_filters_get_memory(&ctx->filters);
			size_t filter_length = dmc_unrar_filters_get_first_length(&ctx->filters);

			DMC_UNRAR_ASSERT(filter_length < DMC_UNRAR_FILTERS_MEMORY_SIZE);

			return_code = dmc_unrar_rar30_decompress_block(ctx, filter_memory, &ctx->filter_offset,
			              filter_length, &ctx->ctx->output_offset, false);

			if (return_code != DMC_UNRAR_OK)
				return return_code;

			DMC_UNRAR_ASSERT(ctx->filter_offset == filter_length);
		}

		/* We have a filter and its memory data now. */

		{
			size_t next_filter = 0;

			/* Run the filter. */

			return_code = dmc_unrar_filters_run(&ctx->filters, current_offset, ctx->ctx->current_file_start,
			                                    &ctx->filter_output_offset,  &ctx->filter_output_length);

			if (return_code != DMC_UNRAR_OK)
				return return_code;

			/* Check when the next filter starts. Up to there, the current filter output
			 * goes to the output buffer. The rest is input for the next filter. */

			next_filter = dmc_unrar_filters_get_first_offset(&ctx->filters);

			ctx->filter_overhang = DMC_UNRAR_MIN(ctx->filter_output_length, next_filter);
			ctx->filter_offset   = 0;
		}
	}

	return DMC_UNRAR_OK;

#else /* DMC_UNRAR_DISABLE_FILTERS */
	return dmc_unrar_rar30_decompress_block(ctx, ctx->ctx->buffer, &ctx->ctx->buffer_offset,
	                                        ctx->ctx->buffer_size, &ctx->ctx->output_offset, false);
#endif /* DMC_UNRAR_DISABLE_FILTERS */
}

static dmc_unrar_return dmc_unrar_rar30_decompress_block(dmc_unrar_rar30_context *ctx,
		uint8_t *buffer, size_t *buffer_offset, size_t buffer_size, size_t *running_output_count,
		bool stop_at_filter) {

	dmc_unrar_return return_code;

	DMC_UNRAR_ASSERT(ctx && buffer_offset);
	(void)stop_at_filter;

	while (*buffer_offset < buffer_size) {
		if (dmc_unrar_bs_has_error(&ctx->ctx->bs))
			break;

#if DMC_UNRAR_DISABLE_FILTERS != 1
		if (stop_at_filter) {
			const size_t filter_pos = dmc_unrar_filters_get_first_offset(&ctx->filters) -
			                          ctx->ctx->current_file_start - ctx->ctx->solid_offset;

			buffer_size = DMC_UNRAR_MIN(buffer_size, filter_pos);
		}

		if (*buffer_offset >= buffer_size)
			break;
#endif /* DMC_UNRAR_DISABLE_FILTERS */

		if (dmc_unrar_lzss_has_overhang(&ctx->ctx->lzss)) {
			*buffer_offset = dmc_unrar_lzss_emit_copy_overhang(&ctx->ctx->lzss, buffer,
			                 buffer_size, *buffer_offset, running_output_count);
			continue;
		}

		if (dmc_unrar_bs_eos(&ctx->ctx->bs) || ctx->ctx->start_new_file)
			break;

		if (ctx->is_ppmd_block) {
			return_code = dmc_unrar_rar30_decode_ppmd(ctx, buffer, buffer_offset, buffer_size,
			                                          running_output_count);
			if (return_code != DMC_UNRAR_OK)
				return return_code;

			continue;
		}

		return_code = dmc_unrar_rar30_decode_huffman(ctx, buffer, buffer_offset, buffer_size,
		                                             running_output_count);
		if (return_code != DMC_UNRAR_OK)
			return return_code;
	}

	return dmc_unrar_bs_has_error(&ctx->ctx->bs) ? DMC_UNRAR_READ_FAIL : DMC_UNRAR_OK;
}

static dmc_unrar_return dmc_unrar_rar30_init_ppmd(dmc_unrar_rar30_context *ctx) {
#if DMC_UNRAR_DISABLE_PPMD != 1
	const uint8_t flags = dmc_unrar_bs_read_bits(&ctx->ctx->bs, 7);

	size_t heap_size_mb = SIZE_MAX;
	if (flags & 0x20)
		heap_size_mb = dmc_unrar_bs_read_bits(&ctx->ctx->bs, 8) + 1;

	if (flags & 0x40)
		ctx->ppmd_escape = dmc_unrar_bs_read_bits(&ctx->ctx->bs, 8);

	if (flags & 0x20) {
		size_t max_order = (flags & 0x1F) + 1;
		if (max_order > 16)
			max_order = 16 + (max_order - 16) * 3;

		if (max_order == 1)
			return DMC_UNRAR_PPMD_INVALID_MAXORDER;

		DMC_UNRAR_ASSERT(heap_size_mb != SIZE_MAX);

		return dmc_unrar_ppmd_start(&ctx->ppmd, &ctx->ctx->bs, heap_size_mb, max_order);
	}

	return dmc_unrar_ppmd_restart(&ctx->ppmd, &ctx->ctx->bs);

#else /* DMC_UNRAR_DISABLE_PPMD */
	(void)ctx;
	return DMC_UNRAR_30_DISABLED_FEATURE_PPMD;
#endif /* DMC_UNRAR_DISABLE_PPMD */
}

#define DMC_UNRAR_30_MAIN_CODE_OFFSET    (0)
#define DMC_UNRAR_30_OFFSET_CODE_OFFSET  (DMC_UNRAR_30_MAIN_CODE_OFFSET   + DMC_UNRAR_30_MAIN_CODE_COUNT)
#define DMC_UNRAR_30_SMALL_CODE_OFFSET   (DMC_UNRAR_30_OFFSET_CODE_OFFSET + DMC_UNRAR_30_OFFSET_CODE_COUNT)
#define DMC_UNRAR_30_LENGTH_CODE_OFFSET  (DMC_UNRAR_30_SMALL_CODE_OFFSET  + DMC_UNRAR_30_SMALL_CODE_COUNT)

static dmc_unrar_return dmc_unrar_rar30_init_huffman(dmc_unrar_rar30_context *ctx) {
	dmc_unrar_return return_code = DMC_UNRAR_OK;

	dmc_unrar_huff huff_pre;
	uint8_t pre_lengths[DMC_UNRAR_30_PRE_CODE_COUNT];
	size_t i, j;

	ctx->last_small = 0;
	ctx->small_offset_repeat_count = 0;

	if (dmc_unrar_bs_read_bits(&ctx->ctx->bs, 1) == 0)
		DMC_UNRAR_CLEAR_ARRAY(ctx->length_table);

	for (i = 0; i < DMC_UNRAR_30_PRE_CODE_COUNT; ) {
		const size_t length = dmc_unrar_bs_read_bits(&ctx->ctx->bs, 4);
		if (length == DMC_UNRAR_30_CODE_LENGTH) {
			const size_t count = dmc_unrar_bs_read_bits(&ctx->ctx->bs, 4) + 2;

			if (count == 2)
				pre_lengths[i++] = DMC_UNRAR_30_CODE_LENGTH;
			else
				for (j = 0; j <count && i < DMC_UNRAR_30_PRE_CODE_COUNT; j++)
					pre_lengths[i++] = 0;

		} else
			pre_lengths[i++] = length;
	}

	return_code = dmc_unrar_huff_create_from_lengths(&ctx->ctx->archive->alloc, &huff_pre,
		pre_lengths, DMC_UNRAR_30_PRE_CODE_COUNT, DMC_UNRAR_30_CODE_LENGTH);

	if (return_code != DMC_UNRAR_OK)
		return return_code;

	for (i = 0; i < DMC_UNRAR_30_MAX_CODE_COUNT; ) {
		size_t n;

		const uint32_t val = dmc_unrar_huff_get_symbol(&huff_pre, &ctx->ctx->bs, &return_code);
		if (return_code != DMC_UNRAR_OK)
			goto end;

		if (val < 16) {
			ctx->length_table[i] = (ctx->length_table[i] + val) & 0x0F;
			i++;

			continue;
		}

		if (val < 18) {
			if (i == 0) {
				return_code = DMC_UNRAR_30_INVALID_LENGTH_TABLE_DATA;
				goto end;
			}

			if (val == 16)
				n = dmc_unrar_bs_read_bits(&ctx->ctx->bs, 3) +  3;
			else
				n = dmc_unrar_bs_read_bits(&ctx->ctx->bs, 7) + 11;

			for (j = 0; j < n && i < DMC_UNRAR_30_MAX_CODE_COUNT; j++, i++)
				ctx->length_table[i] = ctx->length_table[i - 1];

			continue;
		}

		if (val == 18)
			n = dmc_unrar_bs_read_bits(&ctx->ctx->bs, 3) +  3;
		else
			n = dmc_unrar_bs_read_bits(&ctx->ctx->bs, 7) + 11;

		for (j = 0; j < n && i < DMC_UNRAR_30_MAX_CODE_COUNT; j++, i++)
			ctx->length_table[i] = 0;
	}

	return_code = dmc_unrar_huff_create_from_lengths(&ctx->ctx->archive->alloc, &ctx->huff_main,
		ctx->length_table + DMC_UNRAR_30_MAIN_CODE_OFFSET, DMC_UNRAR_30_MAIN_CODE_COUNT,
		DMC_UNRAR_30_CODE_LENGTH);

	if (return_code != DMC_UNRAR_OK)
		goto end;

	return_code = dmc_unrar_huff_create_from_lengths(&ctx->ctx->archive->alloc, &ctx->huff_offset,
		ctx->length_table + DMC_UNRAR_30_OFFSET_CODE_OFFSET, DMC_UNRAR_30_OFFSET_CODE_COUNT,
		DMC_UNRAR_30_CODE_LENGTH);

	if (return_code != DMC_UNRAR_OK)
		goto end;

	return_code = dmc_unrar_huff_create_from_lengths(&ctx->ctx->archive->alloc, &ctx->huff_small,
		ctx->length_table + DMC_UNRAR_30_SMALL_CODE_OFFSET, DMC_UNRAR_30_SMALL_CODE_COUNT,
		DMC_UNRAR_30_CODE_LENGTH);

	if (return_code != DMC_UNRAR_OK)
		goto end;

	return_code = dmc_unrar_huff_create_from_lengths(&ctx->ctx->archive->alloc, &ctx->huff_length,
		ctx->length_table + DMC_UNRAR_30_LENGTH_CODE_OFFSET, DMC_UNRAR_30_LENGTH_CODE_COUNT,
		DMC_UNRAR_30_CODE_LENGTH);

	if (return_code != DMC_UNRAR_OK)
		goto end;

end:
	dmc_unrar_huff_destroy(&huff_pre);
	return return_code;
}

static dmc_unrar_return dmc_unrar_rar30_read_codes(dmc_unrar_rar30_context *ctx) {
	dmc_unrar_rar30_free_codes(ctx);

	if (!dmc_unrar_bs_skip_to_byte_boundary(&ctx->ctx->bs))
		return DMC_UNRAR_SEEK_FAIL;

	ctx->is_ppmd_block = dmc_unrar_bs_read_bits(&ctx->ctx->bs, 1);

	if (ctx->is_ppmd_block)
		return dmc_unrar_rar30_init_ppmd(ctx);

	return dmc_unrar_rar30_init_huffman(ctx);
}

static bool dmc_unrar_rar30_decode_copy_param(dmc_unrar_rar30_context *ctx, uint32_t symbol,
	size_t *offset, size_t *length, dmc_unrar_return *return_code);

static dmc_unrar_return dmc_unrar_rar30_read_filter_from_input(dmc_unrar_rar30_context *ctx);

#if DMC_UNRAR_DISABLE_PPMD != 1
static dmc_unrar_return dmc_unrar_rar30_read_filter_from_ppmd(dmc_unrar_rar30_context *ctx);
#endif /* DMC_UNRAR_DISABLE_PPMD */

static dmc_unrar_return dmc_unrar_rar30_decode_huffman(dmc_unrar_rar30_context *ctx,
		uint8_t *buffer, size_t *buffer_offset, size_t buffer_size, size_t *running_output_count) {

	dmc_unrar_return return_code = DMC_UNRAR_OK;

	const uint32_t symbol = dmc_unrar_huff_get_symbol(&ctx->huff_main, &ctx->ctx->bs, &return_code);
	if (return_code != DMC_UNRAR_OK)
		return return_code;

	if (symbol < 256) {
		/* Literal. */

		*buffer_offset = dmc_unrar_lzss_emit_literal(&ctx->ctx->lzss, buffer,
		                 buffer_size, *buffer_offset, symbol, running_output_count);

		return DMC_UNRAR_OK;
	}

	if (symbol == 256) {
		/* New solid part or new code book. */

		const bool new_file = !dmc_unrar_bs_read_bits(&ctx->ctx->bs, 1);

		if (new_file) {
			ctx->ctx->start_new_file = true;
			ctx->start_new_table = dmc_unrar_bs_read_bits(&ctx->ctx->bs, 1);
		} else {
			if ((return_code = dmc_unrar_rar30_read_codes(ctx)) != DMC_UNRAR_OK)
				return return_code;
		}

		return DMC_UNRAR_OK;
	}

	if (symbol == 257)
		return dmc_unrar_rar30_read_filter_from_input(ctx);

	{
		size_t offset, length;
		if (!dmc_unrar_rar30_decode_copy_param(ctx, symbol, &offset, &length, &return_code))
			return return_code;

		if (return_code != DMC_UNRAR_OK)
			return return_code;

		ctx->last_offset = offset;
		ctx->last_length = length;

		*buffer_offset = dmc_unrar_lzss_emit_copy(&ctx->ctx->lzss, buffer,
		                 buffer_size, *buffer_offset, offset, length, running_output_count);
	}

	return return_code;
}

static bool dmc_unrar_rar30_decode_copy_param(dmc_unrar_rar30_context *ctx, uint32_t symbol,
		size_t *offset, size_t *length, dmc_unrar_return *return_code) {

	static const size_t DMC_UNRAR_30_LENGTH_BASES[28] = {
		  0,   1,   2,   3,   4,   5,   6,   7,   8,  10,  12,  14,  16,  20,
		 24,  28,  32,  40,  48,  56,  64,  80,  96, 112, 128, 160, 192, 224
	};
	static const uint8_t DMC_UNRAR_30_LENGTH_BITS[28] = {
		  0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   1,   1,   2,   2,
		  2,   2,   3,   3,   3,   3,   4,   4,   4,   4,   5,   5,   5,   5
	};

	static const size_t DMC_UNRAR_30_OFFSET_BASES[60] = {
		      0,       1,       2,       3,       4,       6,       8,      12,      16,      24,
		     32,      48,      64,      96,     128,     192,     256,     384,     512,     768,
		   1024,    1536,    2048,    3072,    4096,    6144,    8192,   12288,   16384,   24576,
		  32768,   49152,   65536,   98304,  131072,  196608,  262144,  327680,  393216,  458752,
		 524288,  589824,  655360,  720896,  786432,  851968,  917504,  983040, 1048576, 1310720,
		1572864, 1835008, 2097152, 2359296, 2621440, 2883584, 3145728, 3407872, 3670016, 3932160
	};
	static const uint8_t DMC_UNRAR_30_OFFSET_BITS[60] = {
		      0,       0,       0,       0,       1,       1,       2,       2,       3,       3,
		      4,       4,       5,       5,       6,       6,       7,       7,       8,       8,
		      9,       9,      10,      10,      11,      11,      12,      12,      13,      13,
		     14,      14,      15,      15,      16,      16,      16,      16,      16,      16,
		     16,      16,      16,      16,      16,      16,      16,      16,      18,      18,
		     18,      18,      18,      18,      18,      18,      18,      18,      18,      18
	};

	static const size_t DMC_UNRAR_30_SHORT_BASES[8] = {
		  0,   4,   8,  16,  32,  64, 128, 192
	};
	static const uint8_t DMC_UNRAR_30_SHORT_BITS[8] = {
		  2,   2,   3,   4,   5,   6,   6,   6
	};

	size_t offset_index, length_index, i;

	DMC_UNRAR_ASSERT(return_code);
	*return_code = DMC_UNRAR_OK;

	if (symbol == 258) {
		if (ctx->last_length == 0)
			return false;

		*offset = ctx->last_offset;
		*length = ctx->last_length;

		return true;
	}

	if (symbol <= 262) {
		offset_index = symbol - 259;
		*offset = ctx->old_offset[offset_index];

		length_index = dmc_unrar_huff_get_symbol(&ctx->huff_length, &ctx->ctx->bs, return_code);
		if (*return_code != DMC_UNRAR_OK)
			return false;

		*length  = DMC_UNRAR_30_LENGTH_BASES[length_index] + 2;
		*length += dmc_unrar_bs_read_bits(&ctx->ctx->bs, DMC_UNRAR_30_LENGTH_BITS[length_index]);

		for (i = offset_index; i > 0; i--)
			ctx->old_offset[i] = ctx->old_offset[i - 1];

		ctx->old_offset[0] = *offset;

		return true;
	}

	if (symbol <= 270) {
		*length  = 2;
		*offset  = DMC_UNRAR_30_SHORT_BASES[symbol - 263] + 1;
		*offset += dmc_unrar_bs_read_bits(&ctx->ctx->bs, DMC_UNRAR_30_SHORT_BITS[symbol - 263]);

		for (i = 3; i > 0; i--)
			ctx->old_offset[i] = ctx->old_offset[i - 1];

		ctx->old_offset[0] = *offset;

		return true;
	}

	/* symbol >= 271. */

	*length  = DMC_UNRAR_30_LENGTH_BASES[symbol - 271] + 3;
	*length += dmc_unrar_bs_read_bits(&ctx->ctx->bs, DMC_UNRAR_30_LENGTH_BITS[symbol - 271]);

	offset_index = dmc_unrar_huff_get_symbol(&ctx->huff_offset, &ctx->ctx->bs, return_code);
	if (*return_code != DMC_UNRAR_OK)
		return false;

	*offset = DMC_UNRAR_30_OFFSET_BASES[offset_index] + 1;

	if (DMC_UNRAR_30_OFFSET_BITS[offset_index] > 0) {
		if (offset_index > 9) {
			if (DMC_UNRAR_30_OFFSET_BITS[offset_index] > 4)
				*offset += dmc_unrar_bs_read_bits(&ctx->ctx->bs, DMC_UNRAR_30_OFFSET_BITS[offset_index] - 4) << 4;

			if (ctx->small_offset_repeat_count > 0) {
				ctx->small_offset_repeat_count--;
				*offset += ctx->last_small;
			} else {
				const size_t small_offset_symbol = dmc_unrar_huff_get_symbol(&ctx->huff_small, &ctx->ctx->bs, return_code);
				if (*return_code != DMC_UNRAR_OK)
					return false;

				if (small_offset_symbol == 16) {
					ctx->small_offset_repeat_count = 15;
					*offset += ctx->last_small;
				} else {
					*offset += small_offset_symbol;
					ctx->last_small = small_offset_symbol;
				}
			}
		} else
			*offset += dmc_unrar_bs_read_bits(&ctx->ctx->bs, DMC_UNRAR_30_OFFSET_BITS[offset_index]);
	}

	if (*offset >= 0x40000)
		(*length)++;

	if (*offset >= 0x2000)
		(*length)++;

	for (i = 3; i > 0; i--)
		ctx->old_offset[i] = ctx->old_offset[i - 1];

	ctx->old_offset[0] = *offset;

	if (dmc_unrar_bs_has_error(&ctx->ctx->bs))
		*return_code = DMC_UNRAR_READ_FAIL;

	return true;
}

static dmc_unrar_return dmc_unrar_rar30_decode_ppmd(dmc_unrar_rar30_context *ctx,
		uint8_t *buffer, size_t *buffer_offset, size_t buffer_size, size_t *running_output_count) {

#if DMC_UNRAR_DISABLE_PPMD != 1
	dmc_unrar_return return_code = DMC_UNRAR_OK;
	uint8_t symbol, code;
	size_t offset, length;

	symbol = dmc_unrar_ppmd_get_byte(&ctx->ppmd);

	if (symbol != ctx->ppmd_escape) {
		/* Literal. */

		*buffer_offset = dmc_unrar_lzss_emit_literal(&ctx->ctx->lzss, buffer,
		                 buffer_size, *buffer_offset, symbol, running_output_count);

		return DMC_UNRAR_OK;
	}

	code = dmc_unrar_ppmd_get_byte(&ctx->ppmd);

	switch (code) {
		case 0:
			/* New code book. */

			if ((return_code = dmc_unrar_rar30_read_codes(ctx)) != DMC_UNRAR_OK)
				return return_code;

			break;

		case 2:
			/* New solid part. */

			ctx->ctx->start_new_file = true;
			ctx->start_new_table = true;
			break;

		case 3:
			/* RARVM filter. */
			return dmc_unrar_rar30_read_filter_from_ppmd(ctx);

		case 4:
			{
				/* LZSS copy. */

				offset  = dmc_unrar_ppmd_get_byte(&ctx->ppmd) << 16;
				offset |= dmc_unrar_ppmd_get_byte(&ctx->ppmd) <<  8;
				offset |= dmc_unrar_ppmd_get_byte(&ctx->ppmd);
				offset += 2;

				length = dmc_unrar_ppmd_get_byte(&ctx->ppmd) + 32;

				*buffer_offset = dmc_unrar_lzss_emit_copy(&ctx->ctx->lzss, buffer,
				                 buffer_size, *buffer_offset, offset, length, running_output_count);
			}
			break;

		case 5:
			{
				/* LZSS copy, one byte repeatedly. */

				length = dmc_unrar_ppmd_get_byte(&ctx->ppmd) + 4;

				*buffer_offset = dmc_unrar_lzss_emit_copy(&ctx->ctx->lzss, buffer,
				                 buffer_size, *buffer_offset, 1, length, running_output_count);
			}
			break;

		default:
			/* The literal is the escape value. */

			*buffer_offset = dmc_unrar_lzss_emit_literal(&ctx->ctx->lzss, buffer,
			                 buffer_size, *buffer_offset, symbol, running_output_count);
			break;
	}

	return DMC_UNRAR_OK;
#else /* DMC_UNRAR_DISABLE_PPMD */
	(void)ctx; (void)buffer; (void)buffer_offset; (void)buffer_size; (void)running_output_count;
	return DMC_UNRAR_30_DISABLED_FEATURE_PPMD;
#endif /* DMC_UNRAR_DISABLE_PPMD */
}

static dmc_unrar_return dmc_unrar_rar30_read_filter_from_input(dmc_unrar_rar30_context *ctx) {
#if DMC_UNRAR_DISABLE_FILTERS != 1
	dmc_unrar_return result = DMC_UNRAR_OK;
	const uint8_t flags = dmc_unrar_bs_read_bits(&ctx->ctx->bs, 8);
	size_t i;

	size_t length = (flags & 7) + 1;
	if      (length == 7)
		length = dmc_unrar_bs_read_bits(&ctx->ctx->bs,  8) + 7;
	else if (length == 8)
		length = dmc_unrar_bs_read_bits(&ctx->ctx->bs, 16);

	/* If the length is larger than any of the filter we recognize, abort here.
	 * This also lets us use a small static buffer to read the filter in, instead
	 * of allocating a buffer for each filter. */
	if (length > DMC_UNRAR_FILTERS_BYTECODE_SIZE)
		return DMC_UNRAR_FILTERS_UNKNOWN;

	for (i = 0; i < length; i++) {
		ctx->filter_buffer[i] = dmc_unrar_bs_read_bits(&ctx->ctx->bs, 8);
		if (dmc_unrar_bs_has_error(&ctx->ctx->bs)) {
			result = DMC_UNRAR_READ_FAIL;
			break;
		}
	}

	if (result == DMC_UNRAR_OK)
		result = dmc_unrar_filters_rar4_parse(&ctx->filters, ctx->filter_buffer, length,
		                                      flags, ctx->ctx->output_offset);

	return result;

#else /* DMC_UNRAR_DISABLE_FILTERS */
	(void)ctx;
	return DMC_UNRAR_30_DISABLED_FEATURE_FILTERS;
#endif /* DMC_UNRAR_DISABLE_FILTERS */
}

#if DMC_UNRAR_DISABLE_PPMD != 1
static dmc_unrar_return dmc_unrar_rar30_read_filter_from_ppmd(dmc_unrar_rar30_context *ctx) {
#if DMC_UNRAR_DISABLE_FILTERS != 1
	dmc_unrar_return result = DMC_UNRAR_OK;
	const uint8_t flags = dmc_unrar_ppmd_get_byte(&ctx->ppmd);
	size_t i;

	size_t length = (flags & 7) + 1;
	if      (length == 7)
		length = dmc_unrar_ppmd_get_byte(&ctx->ppmd) + 7;
	else if (length == 8) {
		length  = dmc_unrar_ppmd_get_byte(&ctx->ppmd) << 8;
		length |= dmc_unrar_ppmd_get_byte(&ctx->ppmd);
	}

	/* If the length is larger than any of the filter we recognize, abort here.
	 * This also lets us use a small static buffer to read the filter in, instead
	 * of allocating a buffer for each filter. */
	if (length > DMC_UNRAR_FILTERS_BYTECODE_SIZE)
		return DMC_UNRAR_FILTERS_UNKNOWN;

	for (i = 0; i < length; i++) {
		ctx->filter_buffer[i] = dmc_unrar_ppmd_get_byte(&ctx->ppmd);
		if (dmc_unrar_bs_has_error(&ctx->ctx->bs)) {
			result = DMC_UNRAR_READ_FAIL;
			break;
		}
	}

	if (result == DMC_UNRAR_OK)
		result = dmc_unrar_filters_rar4_parse(&ctx->filters, ctx->filter_buffer, length,
		                                      flags, ctx->ctx->output_offset);

	return result;

#else /* DMC_UNRAR_DISABLE_FILTERS */
	(void)ctx;
	return DMC_UNRAR_30_DISABLED_FEATURE_FILTERS;
#endif /* DMC_UNRAR_DISABLE_FILTERS */
}
#endif /* DMC_UNRAR_DISABLE_PPMD */
/* '--- */

/* .--- Unpacking RAR 5.0 */
/** Number of RAR 5.0 main Huffman codes. */
#define DMC_UNRAR_50_MAIN_CODE_COUNT 306
/** Number of RAR 5.0 LZSS offset Huffman codes. */
#define DMC_UNRAR_50_OFFSET_CODE_COUNT 64
/** Number of RAR 5.0 LZSS small offset Huffman codes. */
#define DMC_UNRAR_50_SMALL_CODE_COUNT 16
/** Number of RAR 5.0 LZSS length Huffman codes. */
#define DMC_UNRAR_50_LENGTH_CODE_COUNT 44

/** Number of codes in the RAR 5.0 codebook code. */
#define DMC_UNRAR_50_PRE_CODE_COUNT 20
/** Max lengths of all RAR 5.0 Huffman codes. */
#define DMC_UNRAR_50_CODE_LENGTH    15

/** Total number of RAR 5.0 Huffman codes. */
#define DMC_UNRAR_50_MAX_CODE_COUNT (DMC_UNRAR_50_MAIN_CODE_COUNT + DMC_UNRAR_50_OFFSET_CODE_COUNT + DMC_UNRAR_50_SMALL_CODE_COUNT + DMC_UNRAR_50_LENGTH_CODE_COUNT)

typedef struct dmc_unrar_rar50_context_tag {
	dmc_unrar_rar_context *ctx;

	size_t last_length;
	size_t old_offset[4];

	size_t block_end_bits;
	bool is_last_block;

#if DMC_UNRAR_DISABLE_FILTERS != 1
	dmc_unrar_filters filters;
	uint8_t filter_buffer[DMC_UNRAR_FILTERS_BYTECODE_SIZE];
	size_t filter_output_length, filter_output_offset;
	size_t filter_overhang, filter_offset;
#endif /* DMC_UNRAR_DISABLE_FILTERS */

	uint8_t length_table[DMC_UNRAR_50_MAX_CODE_COUNT];

	dmc_unrar_huff huff_main;
	dmc_unrar_huff huff_offset;
	dmc_unrar_huff huff_small;
	dmc_unrar_huff huff_length;

} dmc_unrar_rar50_context;

static void dmc_unrar_rar50_destroy(dmc_unrar_rar_context *ctx);
static dmc_unrar_return dmc_unrar_rar50_unpack(dmc_unrar_rar_context *ctx);

static dmc_unrar_return dmc_unrar_rar50_create(dmc_unrar_rar_context *ctx,
		dmc_unrar_archive *archive, dmc_unrar_file_block *file) {

	dmc_unrar_return return_code;

	DMC_UNRAR_ASSERT(ctx && archive && file);

	if (dmc_unrar_rar_context_check(ctx, archive, file, &return_code))
		return DMC_UNRAR_OK;

	DMC_UNRAR_ASSERT(ctx->archive == archive);

	if (return_code != DMC_UNRAR_OK)
		return return_code;

	ctx->has_end_marker = true;

	ctx->internal_state = dmc_unrar_malloc(&archive->alloc, 1, sizeof(dmc_unrar_rar50_context));
	if (!ctx->internal_state)
		return DMC_UNRAR_ALLOC_FAIL;

	ctx->destroy = &dmc_unrar_rar50_destroy;
	ctx->unpack  = &dmc_unrar_rar50_unpack;

	{
		dmc_unrar_rar50_context *ictx = (dmc_unrar_rar50_context *)ctx->internal_state;
		DMC_UNRAR_CLEAR_OBJ(*ictx);

		ictx->ctx = ctx;

#if DMC_UNRAR_DISABLE_FILTERS != 1
		{
			const dmc_unrar_return create_filters = dmc_unrar_filters_create(&archive->alloc, &ictx->filters);
			if (create_filters != DMC_UNRAR_OK)
				return create_filters;
		}
#endif /* DMC_UNRAR_DISABLE_FILTERS */
	}

	return DMC_UNRAR_OK;
}

static void dmc_unrar_rar50_free_codes(dmc_unrar_rar50_context *ctx);

static void dmc_unrar_rar50_destroy(dmc_unrar_rar_context *ctx) {
	if (!ctx || !ctx->internal_state)
		return;

	{
		dmc_unrar_rar50_context *ictx = (dmc_unrar_rar50_context *)ctx->internal_state;

		dmc_unrar_rar50_free_codes(ictx);

#if DMC_UNRAR_DISABLE_FILTERS != 1
		dmc_unrar_filters_destroy(&ictx->filters);
#endif

		if (ctx->archive)
			dmc_unrar_free(&ctx->archive->alloc, ictx);

		ctx->internal_state = NULL;
	}
}

static dmc_unrar_return dmc_unrar_rar50_decompress(dmc_unrar_rar50_context *ctx);

static dmc_unrar_return dmc_unrar_rar50_unpack(dmc_unrar_rar_context *ctx) {
	DMC_UNRAR_ASSERT(ctx && ctx->internal_state);

	return dmc_unrar_rar50_decompress((dmc_unrar_rar50_context *)ctx->internal_state);
}

static void dmc_unrar_rar50_free_codes(dmc_unrar_rar50_context *ctx) {
	if (!ctx)
		return;

	dmc_unrar_huff_destroy(&ctx->huff_main);
	dmc_unrar_huff_destroy(&ctx->huff_offset);
	dmc_unrar_huff_destroy(&ctx->huff_small);
	dmc_unrar_huff_destroy(&ctx->huff_length);
}

static dmc_unrar_return dmc_unrar_rar50_read_block_header(dmc_unrar_rar50_context *ctx);

static dmc_unrar_return dmc_unrar_rar50_decode_huffman(dmc_unrar_rar50_context *ctx,
	uint8_t *buffer, size_t *buffer_offset, size_t buffer_size, size_t *running_output_count);

static dmc_unrar_return dmc_unrar_rar50_decompress_block(dmc_unrar_rar50_context *ctx,
	uint8_t *buffer, size_t *buffer_offset, size_t buffer_size, size_t *running_output_count,
	bool stop_at_filter);

static dmc_unrar_return dmc_unrar_rar50_decompress(dmc_unrar_rar50_context *ctx) {
	dmc_unrar_return return_code;

	DMC_UNRAR_ASSERT(ctx);

	while ((ctx->ctx->current_file_start * 8 + ctx->ctx->bs.offset_bits) >= ctx->block_end_bits) {
		if ((return_code = dmc_unrar_rar50_read_block_header(ctx)) != DMC_UNRAR_OK)
			return return_code;

		if (ctx->is_last_block)
			break;
	}

#if DMC_UNRAR_DISABLE_FILTERS != 1
	while (ctx->ctx->buffer_offset < ctx->ctx->buffer_size) {
		const size_t current_offset = ctx->ctx->current_file_start + ctx->ctx->solid_offset + ctx->ctx->buffer_offset;

		if (ctx->filter_overhang > 0) {
			/* We still have filter output that goes to the decompression output buffer. */

			uint8_t *filter_memory = dmc_unrar_filters_get_memory(&ctx->filters);

			const size_t buffer_space = ctx->ctx->buffer_size - ctx->ctx->buffer_offset;
			const size_t copy_back = DMC_UNRAR_MIN(ctx->filter_overhang, buffer_space);

			if (ctx->ctx->buffer)
				memcpy(ctx->ctx->buffer + ctx->ctx->buffer_offset,
				       filter_memory + ctx->filter_output_offset, copy_back);

			ctx->ctx->buffer_offset   += copy_back;
			ctx->filter_output_offset += copy_back;

			ctx->filter_overhang      -= copy_back;
			ctx->filter_output_length -= copy_back;

			continue;
		}

		if (ctx->filter_output_length > 0) {
			/* We have filter output that's the input for the next filter.
			 * Move it to the front of the filter memory. */

			uint8_t *filter_memory = dmc_unrar_filters_get_memory(&ctx->filters);

			memmove(filter_memory, filter_memory + ctx->filter_output_offset, ctx->filter_output_length);

			ctx->filter_offset = ctx->filter_output_length;

			ctx->filter_output_offset = 0;
			ctx->filter_output_length = 0;

			ctx->filter_overhang = 0;
		}

		if (!dmc_unrar_lzss_has_overhang(&ctx->ctx->lzss) &&
			  (dmc_unrar_bs_eos(&ctx->ctx->bs) || ctx->ctx->start_new_file))
			break;

		if (current_offset < dmc_unrar_filters_get_first_offset(&ctx->filters)) {
			/* We're before the first filter position (or have no filter at all), so
			 * we can directly decode into the output buffer. */

			DMC_UNRAR_ASSERT(ctx->filter_offset == 0);

			return_code = dmc_unrar_rar50_decompress_block(ctx, ctx->ctx->buffer, &ctx->ctx->buffer_offset,
			              ctx->ctx->buffer_size, &ctx->ctx->output_offset, true);

			if (return_code != DMC_UNRAR_OK)
				return return_code;

			continue;
		}

		/* Now we should have filters and be at the first filter position. */

		DMC_UNRAR_ASSERT(!dmc_unrar_filters_empty(&ctx->filters));
		DMC_UNRAR_ASSERT(dmc_unrar_filters_get_first_length(&ctx->filters) > 0);
		DMC_UNRAR_ASSERT(current_offset == dmc_unrar_filters_get_first_offset(&ctx->filters));

		/* Decode into the filter memory. */

		{
			uint8_t *filter_memory = dmc_unrar_filters_get_memory(&ctx->filters);
			size_t filter_length = dmc_unrar_filters_get_first_length(&ctx->filters);

			DMC_UNRAR_ASSERT(filter_length < DMC_UNRAR_FILTERS_MEMORY_SIZE);

			return_code = dmc_unrar_rar50_decompress_block(ctx, filter_memory, &ctx->filter_offset,
			              filter_length, &ctx->ctx->output_offset, false);

			if (return_code != DMC_UNRAR_OK)
				return return_code;

			DMC_UNRAR_ASSERT(ctx->filter_offset == filter_length);
		}

		/* We have a filter and its memory data now. */

		{
			size_t next_filter = 0;

			/* Run the filter. */

			return_code = dmc_unrar_filters_run(&ctx->filters, current_offset, ctx->ctx->current_file_start,
			                                    &ctx->filter_output_offset,  &ctx->filter_output_length);

			if (return_code != DMC_UNRAR_OK)
				return return_code;

			/* Check when the next filter starts. Up to there, the current filter output
			 * goes to the output buffer. The rest is input for the next filter. */

			next_filter = dmc_unrar_filters_get_first_offset(&ctx->filters);

			ctx->filter_overhang = DMC_UNRAR_MIN(ctx->filter_output_length, next_filter);
			ctx->filter_offset   = 0;
		}
	}

	return DMC_UNRAR_OK;

#else /* DMC_UNRAR_DISABLE_FILTERS */
	return dmc_unrar_rar50_decompress_block(ctx, ctx->ctx->buffer, &ctx->ctx->buffer_offset,
	                                        ctx->ctx->buffer_size, &ctx->ctx->output_offset, false);
#endif /* DMC_UNRAR_DISABLE_FILTERS */
}

static dmc_unrar_return dmc_unrar_rar50_decompress_block(dmc_unrar_rar50_context *ctx,
		uint8_t *buffer, size_t *buffer_offset, size_t buffer_size, size_t *running_output_count,
		bool stop_at_filter) {

	dmc_unrar_return return_code;

	DMC_UNRAR_ASSERT(ctx && buffer_offset);
	(void)stop_at_filter;

	while (*buffer_offset < buffer_size) {
		if (dmc_unrar_bs_has_error(&ctx->ctx->bs))
			break;

#if DMC_UNRAR_DISABLE_FILTERS != 1
		if (stop_at_filter) {
			const size_t filter_pos = dmc_unrar_filters_get_first_offset(&ctx->filters) -
			                          ctx->ctx->current_file_start - ctx->ctx->solid_offset;

			buffer_size = DMC_UNRAR_MIN(buffer_size, filter_pos);
		}

		if (*buffer_offset >= buffer_size)
			break;
#endif /* DMC_UNRAR_DISABLE_FILTERS */

		if (dmc_unrar_lzss_has_overhang(&ctx->ctx->lzss)) {
			*buffer_offset = dmc_unrar_lzss_emit_copy_overhang(&ctx->ctx->lzss, buffer,
			                 buffer_size, *buffer_offset, running_output_count);
			continue;
		}

		if (dmc_unrar_bs_eos(&ctx->ctx->bs) || ctx->ctx->start_new_file)
			break;

		if ((ctx->ctx->current_file_start * 8 + ctx->ctx->bs.offset_bits) >= ctx->block_end_bits) {
			while ((ctx->ctx->current_file_start * 8 + ctx->ctx->bs.offset_bits) >= ctx->block_end_bits) {
				if (ctx->is_last_block) {
					ctx->ctx->start_new_file = true;
					break;
				}

				if ((return_code = dmc_unrar_rar50_read_block_header(ctx)) != DMC_UNRAR_OK)
					return return_code;
			}

			continue;
		}

		return_code = dmc_unrar_rar50_decode_huffman(ctx, buffer, buffer_offset, buffer_size,
		                                             running_output_count);
		if (return_code != DMC_UNRAR_OK)
			return return_code;
	}

	return dmc_unrar_bs_has_error(&ctx->ctx->bs) ? DMC_UNRAR_READ_FAIL : DMC_UNRAR_OK;
}

static dmc_unrar_return dmc_unrar_rar50_read_codes(dmc_unrar_rar50_context *ctx);

static dmc_unrar_return dmc_unrar_rar50_read_block_header(dmc_unrar_rar50_context *ctx) {
	DMC_UNRAR_ASSERT(ctx);

	if (!dmc_unrar_bs_skip_to_byte_boundary(&ctx->ctx->bs))
		return DMC_UNRAR_SEEK_FAIL;

	if (dmc_unrar_bs_eos(&ctx->ctx->bs)) {
		ctx->is_last_block = true;
		return DMC_UNRAR_OK;
	}

	{
		uint8_t calculated_checksum, size_count, block_size_bits;
		size_t block_size = 0, i;

		const uint8_t flags    = dmc_unrar_bs_read_bits(&ctx->ctx->bs, 8);
		const uint8_t checksum = dmc_unrar_bs_read_bits(&ctx->ctx->bs, 8);

		/* TODO: The Unarchive throws an exception when size_count is 4,
		 * with the comment "What to do here?". The size_count is the
		 * number of bytes to read for the block size (in bytes), and I
		 * don't really see what the problem would be with reading a
		 * full 32-bit value for the size of the block... */

		size_count = ((flags >> 3) & 3) + 1;

		/* Number of bits the block size is long beyond a full byte. */
		block_size_bits = (flags & 7) + 1;

		calculated_checksum = 0x5A ^ flags;

		for (i = 0; i < size_count; i++) {
			const uint8_t value = dmc_unrar_bs_read_bits(&ctx->ctx->bs, 8);

			block_size += value << (i * 8);

			calculated_checksum ^= value;
		}

		if (calculated_checksum != checksum)
			return DMC_UNRAR_50_BLOCK_CHECKSUM_NO_MATCH;

		{
			const size_t current_offset = ctx->ctx->current_file_start * 8 + ctx->ctx->bs.offset_bits;

			ctx->block_end_bits = current_offset + block_size * 8 + block_size_bits - 8;
		}

		ctx->is_last_block = (flags & 0x40) != 0;

		if (flags & 0x80)
			return dmc_unrar_rar50_read_codes(ctx);
	}

	return dmc_unrar_bs_has_error(&ctx->ctx->bs) ? DMC_UNRAR_READ_FAIL : DMC_UNRAR_OK;
}

#define DMC_UNRAR_50_MAIN_CODE_OFFSET   (0)
#define DMC_UNRAR_50_OFFSET_CODE_OFFSET (DMC_UNRAR_50_MAIN_CODE_OFFSET   + DMC_UNRAR_50_MAIN_CODE_COUNT)
#define DMC_UNRAR_50_SMALL_CODE_OFFSET  (DMC_UNRAR_50_OFFSET_CODE_OFFSET + DMC_UNRAR_50_OFFSET_CODE_COUNT)
#define DMC_UNRAR_50_LENGTH_CODE_OFFSET (DMC_UNRAR_50_SMALL_CODE_OFFSET  + DMC_UNRAR_50_SMALL_CODE_COUNT)

static dmc_unrar_return dmc_unrar_rar50_read_codes(dmc_unrar_rar50_context *ctx) {
	dmc_unrar_return return_code = DMC_UNRAR_OK;

	dmc_unrar_huff huff_pre;
	uint8_t pre_lengths[DMC_UNRAR_50_PRE_CODE_COUNT];
	size_t i, j;

	dmc_unrar_rar50_free_codes(ctx);

	for (i = 0; i < DMC_UNRAR_50_PRE_CODE_COUNT; ) {
		uint8_t length = dmc_unrar_bs_read_bits(&ctx->ctx->bs, 4);

		if (length == 15) {
			size_t count = dmc_unrar_bs_read_bits(&ctx->ctx->bs, 4) + 2;

			if (count != 2) {
				for (j = 0; j < count && i < 20; j++)
					pre_lengths[i++] = 0;

			} else
				pre_lengths[i++] = 15;


		} else
			pre_lengths[i++] = length;
	}

	return_code = dmc_unrar_huff_create_from_lengths(&ctx->ctx->archive->alloc, &huff_pre,
		pre_lengths, DMC_UNRAR_50_PRE_CODE_COUNT, DMC_UNRAR_50_CODE_LENGTH);

	if (return_code != DMC_UNRAR_OK)
		return return_code;

	for (i = 0; i < DMC_UNRAR_50_MAX_CODE_COUNT; ) {
		size_t n;

		const uint32_t val = dmc_unrar_huff_get_symbol(&huff_pre, &ctx->ctx->bs, &return_code);
		if (return_code != DMC_UNRAR_OK)
			goto end;

		if (val < 16) {
			ctx->length_table[i++] = val;

			continue;
		}

		if (val < 18) {
			if (i == 0) {
				return_code = DMC_UNRAR_50_INVALID_LENGTH_TABLE_DATA;
				goto end;
			}

			if (val == 16)
				n = dmc_unrar_bs_read_bits(&ctx->ctx->bs, 3) +  3;
			else
				n = dmc_unrar_bs_read_bits(&ctx->ctx->bs, 7) + 11;

			for (j = 0; j < n && i < DMC_UNRAR_50_MAX_CODE_COUNT; j++, i++)
				ctx->length_table[i] = ctx->length_table[i - 1];

			continue;
		}

		if (val == 18)
			n = dmc_unrar_bs_read_bits(&ctx->ctx->bs, 3) +  3;
		else
			n = dmc_unrar_bs_read_bits(&ctx->ctx->bs, 7) + 11;

		for (j = 0; j < n && i < DMC_UNRAR_50_MAX_CODE_COUNT; j++, i++)
			ctx->length_table[i] = 0;
	}

	return_code = dmc_unrar_huff_create_from_lengths(&ctx->ctx->archive->alloc, &ctx->huff_main,
		ctx->length_table + DMC_UNRAR_50_MAIN_CODE_OFFSET, DMC_UNRAR_50_MAIN_CODE_COUNT,
		DMC_UNRAR_50_CODE_LENGTH);

	if (return_code != DMC_UNRAR_OK)
		goto end;

	return_code = dmc_unrar_huff_create_from_lengths(&ctx->ctx->archive->alloc, &ctx->huff_offset,
		ctx->length_table + DMC_UNRAR_50_OFFSET_CODE_OFFSET, DMC_UNRAR_50_OFFSET_CODE_COUNT,
		DMC_UNRAR_50_CODE_LENGTH);

	if (return_code != DMC_UNRAR_OK)
		goto end;

	return_code = dmc_unrar_huff_create_from_lengths(&ctx->ctx->archive->alloc, &ctx->huff_small,
		ctx->length_table + DMC_UNRAR_50_SMALL_CODE_OFFSET, DMC_UNRAR_50_SMALL_CODE_COUNT,
		DMC_UNRAR_50_CODE_LENGTH);

	if (return_code != DMC_UNRAR_OK)
		goto end;

	return_code = dmc_unrar_huff_create_from_lengths(&ctx->ctx->archive->alloc, &ctx->huff_length,
		ctx->length_table + DMC_UNRAR_50_LENGTH_CODE_OFFSET, DMC_UNRAR_50_LENGTH_CODE_COUNT,
		DMC_UNRAR_50_CODE_LENGTH);

	if (return_code != DMC_UNRAR_OK)
		goto end;

end:
	dmc_unrar_huff_destroy(&huff_pre);
	return return_code;
}

static bool dmc_unrar_rar50_decode_copy_param(dmc_unrar_rar50_context *ctx, uint32_t symbol,
	size_t *offset, size_t *length, dmc_unrar_return *return_code);

static dmc_unrar_return dmc_unrar_rar50_read_filter_from_input(dmc_unrar_rar50_context *ctx);

static dmc_unrar_return dmc_unrar_rar50_decode_huffman(dmc_unrar_rar50_context *ctx,
	uint8_t *buffer, size_t *buffer_offset, size_t buffer_size, size_t *running_output_count) {

	dmc_unrar_return return_code = DMC_UNRAR_OK;

	const uint32_t symbol = dmc_unrar_huff_get_symbol(&ctx->huff_main, &ctx->ctx->bs, &return_code);
	if (return_code != DMC_UNRAR_OK)
		return return_code;

	if (symbol < 256) {
		/* Literal. */

		*buffer_offset = dmc_unrar_lzss_emit_literal(&ctx->ctx->lzss, buffer,
		                 buffer_size, *buffer_offset, symbol, running_output_count);

		return DMC_UNRAR_OK;
	}

	if (symbol == 256)
		return dmc_unrar_rar50_read_filter_from_input(ctx);

	{
		size_t offset, length;
		if (!dmc_unrar_rar50_decode_copy_param(ctx, symbol, &offset, &length, &return_code))
			return return_code;

		if (return_code != DMC_UNRAR_OK)
			return return_code;

		ctx->last_length = length;

		*buffer_offset = dmc_unrar_lzss_emit_copy(&ctx->ctx->lzss, buffer,
		                 buffer_size, *buffer_offset, offset, length, running_output_count);
	}

	return DMC_UNRAR_OK;
}

static size_t dmc_unrar_rar50_read_length_with_symbol(dmc_unrar_rar50_context *ctx, uint32_t symbol,
	dmc_unrar_return *return_code) {

	size_t length;

	DMC_UNRAR_ASSERT(return_code);
	*return_code = DMC_UNRAR_OK;

	if (symbol < 8)
		return symbol + 2;

	{
		size_t length_bits = symbol / 4 - 1;

		length  = ((4 + (symbol & 3)) << length_bits) + 2;
		length += dmc_unrar_bs_read_bits(&ctx->ctx->bs, length_bits);
	}

	if (dmc_unrar_bs_has_error(&ctx->ctx->bs))
		*return_code = DMC_UNRAR_READ_FAIL;

	return length;
}

static bool dmc_unrar_rar50_decode_copy_param(dmc_unrar_rar50_context *ctx, uint32_t symbol,
	size_t *offset, size_t *length, dmc_unrar_return *return_code) {

	size_t offset_symbol, length_symbol, i;

	DMC_UNRAR_ASSERT(return_code);
	*return_code = DMC_UNRAR_OK;

	if (symbol == 257) {
		if (ctx->last_length == 0)
			return false;

		*offset = ctx->old_offset[0];
		*length = ctx->last_length;

		return true;
	}

	if (symbol < 262) {
		offset_symbol = symbol - 258;
		*offset = ctx->old_offset[offset_symbol];

		length_symbol = dmc_unrar_huff_get_symbol(&ctx->huff_length, &ctx->ctx->bs, return_code);
		if (*return_code != DMC_UNRAR_OK)
			return false;

		*length = dmc_unrar_rar50_read_length_with_symbol(ctx, length_symbol, return_code);
		if (*return_code != DMC_UNRAR_OK)
			return false;

		for (i = offset_symbol; i > 0; i--)
			ctx->old_offset[i] = ctx->old_offset[i - 1];

		ctx->old_offset[0] = *offset;

		return true;
	}

	/* symbol >= 262. */

	*length = dmc_unrar_rar50_read_length_with_symbol(ctx, symbol - 262, return_code);
	if (*return_code != DMC_UNRAR_OK)
		return false;

	offset_symbol = dmc_unrar_huff_get_symbol(&ctx->huff_offset, &ctx->ctx->bs, return_code);
	if (*return_code != DMC_UNRAR_OK)
		return false;

	if (offset_symbol >= 4) {
		uint32_t offset_bits = (offset_symbol / 2) - 1;
		uint32_t small_offset;

		if (offset_bits >= 4) {
			if (offset_bits > 4)
				small_offset = dmc_unrar_bs_read_bits(&ctx->ctx->bs, offset_bits - 4) << 4;
			else
				small_offset = 0;

			small_offset += dmc_unrar_huff_get_symbol(&ctx->huff_small, &ctx->ctx->bs, return_code);
			if (*return_code != DMC_UNRAR_OK)
				return false;

		} else
			small_offset = dmc_unrar_bs_read_bits(&ctx->ctx->bs, offset_bits);

		*offset = ((2 + (offset_symbol & 1)) << offset_bits) + small_offset + 1;

	} else
		*offset = offset_symbol + 1;

	if (*offset > 0x40000)
		(*length)++;
	if (*offset > 0x02000)
		(*length)++;
	if (*offset > 0x00100)
		(*length)++;

	for (i = 3; i > 0; i--)
		ctx->old_offset[i] = ctx->old_offset[i - 1];

	ctx->old_offset[0] = *offset;

	if (dmc_unrar_bs_has_error(&ctx->ctx->bs))
		*return_code = DMC_UNRAR_READ_FAIL;

	return true;
}

static dmc_unrar_return dmc_unrar_rar50_read_filter_from_input(dmc_unrar_rar50_context *ctx) {
#if DMC_UNRAR_DISABLE_FILTERS != 1
	return dmc_unrar_filters_rar5_parse(&ctx->filters, &ctx->ctx->bs, ctx->ctx->output_offset);
#else /* DMC_UNRAR_DISABLE_FILTERS */
	(void)ctx;
	return DMC_UNRAR_50_DISABLED_FEATURE_FILTERS;
#endif /* DMC_UNRAR_DISABLE_FILTERS */
}
/* '--- */

/* .--- Bitstream implementation, based on dr_flac's */

/* This uses a 32- or 64-bit bit-shifted cache - as bits are read, the cache is
 * shifted such that the first valid bit is sitting on the most significant bit.
 *
 * It uses the notion of an L1 and L2 cache (borrowed from CPU architecture),
 * where the L1 cache is a 32- or 64-bit unsigned integer (depending on whether
 * or not a 32- or 64-bit build is being compiled) and the L2 is an array of
 * "cache lines", with each cache line being the same size as the L1. The L2 is
 * a buffer of about 4KB and is where data from func_read() is read into.
 */

#define DMC_UNRAR_BS_L1_SIZE_BYTES(bs)                   (sizeof((bs)->cache))
#define DMC_UNRAR_BS_L1_SIZE_BITS(bs)                    (sizeof((bs)->cache)*8)
#define DMC_UNRAR_BS_L1_BITS_REMAINING(bs)               (DMC_UNRAR_BS_L1_SIZE_BITS(bs) - ((bs)->consumed_bits))

#if DMC_UNRAR_64BIT == 1
	#define DMC_UNRAR_BS_L1_SELECTION_MASK(_bit_count)     (~(((uint64_t)((int64_t)-1)) >> (_bit_count)))
#else
	#define DMC_UNRAR_BS_L1_SELECTION_MASK(_bit_count)     (~(((uint32_t)-1) >> (_bit_count)))
#endif

#define DMC_UNRAR_BS_L1_SELECTION_SHIFT(bs, _bit_count)  (DMC_UNRAR_BS_L1_SIZE_BITS(bs) - (_bit_count))
#define DMC_UNRAR_BS_L1_SELECT(bs, _bit_count)           (((bs)->cache) & DMC_UNRAR_BS_L1_SELECTION_MASK(_bit_count))
#define DMC_UNRAR_BS_L1_SELECT_AND_SHIFT(bs, _bit_count) (DMC_UNRAR_BS_L1_SELECT((bs), _bit_count) >> DMC_UNRAR_BS_L1_SELECTION_SHIFT((bs), _bit_count))
#define DMC_UNRAR_BS_L2_SIZE_BYTES(bs)                   (sizeof((bs)->cache_l2))
#define DMC_UNRAR_BS_L2_LINE_COUNT(bs)                   (DMC_UNRAR_BS_L2_SIZE_BYTES(bs) / sizeof((bs)->cache_l2[0]))
#define DMC_UNRAR_BS_L2_LINES_REMAINING(bs)              (DMC_UNRAR_BS_L2_LINE_COUNT(bs) - (bs)->next_l2_line)

#ifdef _MSC_VER
#include <intrin.h> /* For _byteswap_ulong and _byteswap_uint64. */
#endif

#if defined(__linux__) && (DMC_UNRAR_DISABLE_BE32TOH_BE64TOH != 1)
#include <endian.h> /* For be32toh and be64toh. */
#endif

static bool dmc_unrar_bs_is_little_endian(void) {
	int n = 1;
	return (*(char*)&n) == 1;
}

#if DMC_UNRAR_32BIT == 1
static uint32_t dmc_unrar_bs_swap_endian_uint32(uint32_t n) {
#ifdef _MSC_VER
	return _byteswap_ulong(n);
#elif defined(__GNUC__) && ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3))
	return __builtin_bswap32(n);
#else
	return ((n & 0xFF000000) >> 24) |
	       ((n & 0x00FF0000) >>  8) |
	       ((n & 0x0000FF00) <<  8) |
	       ((n & 0x000000FF) << 24);
#endif
}

static uint32_t dmc_unrar_bs_be2host_32(uint32_t n) {
	(void)&dmc_unrar_bs_is_little_endian; (void)&dmc_unrar_bs_swap_endian_uint32;

#if defined(__linux__) && (DMC_UNRAR_DISABLE_BE32TOH_BE64TOH != 1)
	return be32toh(n);
#else
	if (dmc_unrar_bs_is_little_endian()) {
		return dmc_unrar_bs_swap_endian_uint32(n);
	}

	return n;
#endif
}

#define dmc_unrar_bs_be2host_cache_line dmc_unrar_bs_be2host_32
#endif /* DMC_UNRAR_32BIT */

#if DMC_UNRAR_64BIT == 1
static uint64_t dmc_unrar_bs_swap_endian_uint64(uint64_t n) {
#ifdef _MSC_VER
	return _byteswap_uint64(n);
#elif defined(__GNUC__) && ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3))
	return __builtin_bswap64(n);
#else
	return ((n & 0xFF00000000000000ULL) >> 56) |
	       ((n & 0x00FF000000000000ULL) >> 40) |
	       ((n & 0x0000FF0000000000ULL) >> 24) |
	       ((n & 0x000000FF00000000ULL) >>  8) |
	       ((n & 0x00000000FF000000ULL) <<  8) |
	       ((n & 0x0000000000FF0000ULL) << 24) |
	       ((n & 0x000000000000FF00ULL) << 40) |
	       ((n & 0x00000000000000FFULL) << 56);
#endif
}

static uint64_t dmc_unrar_bs_be2host_64(uint64_t n) {
	(void)&dmc_unrar_bs_is_little_endian; (void)&dmc_unrar_bs_swap_endian_uint64;

#if defined(__linux__) && (DMC_UNRAR_DISABLE_BE32TOH_BE64TOH != 1)
	return be64toh(n);
#else
	if (dmc_unrar_bs_is_little_endian()) {
		return dmc_unrar_bs_swap_endian_uint64(n);
	}

	return n;
#endif
}

#define dmc_unrar_bs_be2host_cache_line dmc_unrar_bs_be2host_64
#endif /* DMC_UNRAR_64BIT */

static bool dmc_unrar_bs_init_from_io(dmc_unrar_bs *bs, dmc_unrar_io *io, uint64_t size) {
	if (!bs || !io || !io->func_read || !io->func_seek)
		return false;

	DMC_UNRAR_CLEAR_OBJ(*bs);

	bs->io = *io;

	bs->io.offset = 0;
	bs->io.size   = size;

	/* Trigger a data retrieval right at the start. */
	bs->next_l2_line  = sizeof(bs->cache_l2) / sizeof(bs->cache_l2[0]);
	bs->consumed_bits = sizeof(bs->cache) * 8;
	return true;
}

static bool dmc_unrar_bs_refill_l2_cache_from_client(dmc_unrar_bs *bs) {
	size_t bytes_read, aligned_l1_line_count;

	if (bs->unaligned_byte_count > 0)
		/* If we have any unaligned bytes it means there's no more aligned bytes left in the client.. */
		return false;

	bytes_read = dmc_unrar_archive_read(&bs->io, bs->cache_l2, DMC_UNRAR_BS_L2_SIZE_BYTES(bs));

	bs->next_l2_line = 0;
	if (bytes_read == DMC_UNRAR_BS_L2_SIZE_BYTES(bs))
		return true;

	/* If we get here it means we were unable to retrieve enough data to fill the
	 * entire L2 cache. It probably means we've just reached the end of the file.
	 * We need to move the valid data down to the end of the buffer and adjust
	 * the index of the next line accordingly. Also keep in mind that the L2
	 * cache must be aligned to the size of the L1 so we'll need to seek
	 * backwards by any misaligned bytes. */

	aligned_l1_line_count = bytes_read / DMC_UNRAR_BS_L1_SIZE_BYTES(bs);

	/* We need to keep track of any unaligned bytes for later use.. */
	bs->unaligned_byte_count = bytes_read - (aligned_l1_line_count * DMC_UNRAR_BS_L1_SIZE_BYTES(bs));
	if (bs->unaligned_byte_count > 0)
		bs->unaligned_cache = bs->cache_l2[aligned_l1_line_count];

	if (aligned_l1_line_count > 0) {
		size_t i, offset = DMC_UNRAR_BS_L2_LINE_COUNT(bs) - aligned_l1_line_count;
		for (i = aligned_l1_line_count; i > 0; --i) {
			bs->cache_l2[i-1 + offset] = bs->cache_l2[i-1];
		}

		bs->next_l2_line = offset;
		return true;
	}

	/* If we get into this branch it means we weren't able to load any L1-aligned data.. */
	bs->next_l2_line = DMC_UNRAR_BS_L2_LINE_COUNT(bs);
	return false;
}

static bool dmc_unrar_bs_reload_l1_cache_from_l2(dmc_unrar_bs *bs) {
	/* Fast path. Try loading straight from L2.. */
	if (bs->next_l2_line < DMC_UNRAR_BS_L2_LINE_COUNT(bs)) {
		bs->cache = bs->cache_l2[bs->next_l2_line++];
		return true;
	}

	/* If we get here it means we've run out of data in the L2 cache.
	 * We'll need to fetch more from the client, if there's any left. */

	if (!dmc_unrar_bs_refill_l2_cache_from_client(bs))
		return false;

	DMC_UNRAR_ASSERT(bs->next_l2_line < DMC_UNRAR_BS_L2_LINE_COUNT(bs));

	bs->cache = bs->cache_l2[bs->next_l2_line++];
	return true;
}

static bool dmc_unrar_bs_reload_cache(dmc_unrar_bs *bs) {
	size_t bytes_read;

	/* Fast path. Try just moving the next value in the L2 cache to the L1 cache.. */
	if (dmc_unrar_bs_reload_l1_cache_from_l2(bs)) {
		bs->cache = dmc_unrar_bs_be2host_cache_line(bs->cache);
		bs->consumed_bits = 0;
		return true;
	}

	/* Slow path.. */

	/* If we get here it means we have failed to load the L1 cache from the L2.
	 * Likely we've just reached the end of the stream and the last few bytes
	 * did not meet the alignment requirements for the L2 cache. In this case
	 * we need to fall back to a slower path and read the data from the
	 * unaligned cache. */
	bytes_read = bs->unaligned_byte_count;
	if (bytes_read == 0)
		return false;

	DMC_UNRAR_ASSERT(bytes_read < DMC_UNRAR_BS_L1_SIZE_BYTES(bs));
	bs->consumed_bits = (DMC_UNRAR_BS_L1_SIZE_BYTES(bs) - bytes_read) * 8;

	bs->cache = dmc_unrar_bs_be2host_cache_line(bs->unaligned_cache);

	/* Make sure the consumed bits are always set to zero.
	 * Other parts of the bitstreamer depend on this property. */
	bs->cache &= DMC_UNRAR_BS_L1_SELECTION_MASK(DMC_UNRAR_BS_L1_SIZE_BITS(bs) - bs->consumed_bits);

	bs->unaligned_byte_count = 0;
	return true;
}

static bool dmc_unrar_bs_seek_bits(dmc_unrar_bs *bs, size_t bits_to_seek) {
	size_t whole_bytes_remaining;

	if (bits_to_seek <= DMC_UNRAR_BS_L1_BITS_REMAINING(bs)) {
		bs->offset_bits += bits_to_seek;

		bs->consumed_bits += bits_to_seek;
		bs->cache <<= bits_to_seek;
		return true;
	}

	/* It straddles the cached data.
	 * This function isn't called too frequently so I'm favouring simplicity here. */

	bits_to_seek -= DMC_UNRAR_BS_L1_BITS_REMAINING(bs);
	bs->offset_bits += DMC_UNRAR_BS_L1_BITS_REMAINING(bs);
	bs->consumed_bits += DMC_UNRAR_BS_L1_BITS_REMAINING(bs);
	bs->cache = 0;

	whole_bytes_remaining = bits_to_seek / 8;
	if (whole_bytes_remaining > 0) {
		/* The next bytes to seek will be located in the L2 cache.
		 * The problem is that the L2 cache is not byte aligned, but rather
		 * DMC_UNRAR_BS_L1_SIZE_BYTES aligned (usually 4 or 8). If, for example,
		 * the number of bytes to seek is 3, we'll need to handle it in a special way. */

		size_t whole_cache_lines_remaining = whole_bytes_remaining / DMC_UNRAR_BS_L1_SIZE_BYTES(bs);
		if (whole_cache_lines_remaining < DMC_UNRAR_BS_L2_LINES_REMAINING(bs)) {
			whole_bytes_remaining -= whole_cache_lines_remaining * DMC_UNRAR_BS_L1_SIZE_BYTES(bs);
			bits_to_seek -= whole_cache_lines_remaining * DMC_UNRAR_BS_L1_SIZE_BITS(bs);
			bs->offset_bits += whole_cache_lines_remaining * DMC_UNRAR_BS_L1_SIZE_BITS(bs);
			bs->next_l2_line += whole_cache_lines_remaining;

		} else {
			whole_bytes_remaining -= DMC_UNRAR_BS_L2_LINES_REMAINING(bs) * DMC_UNRAR_BS_L1_SIZE_BYTES(bs);
			bits_to_seek -= DMC_UNRAR_BS_L2_LINES_REMAINING(bs) * DMC_UNRAR_BS_L1_SIZE_BITS(bs);
			bs->offset_bits += DMC_UNRAR_BS_L2_LINES_REMAINING(bs) * DMC_UNRAR_BS_L1_SIZE_BITS(bs);
			bs->next_l2_line += DMC_UNRAR_BS_L2_LINES_REMAINING(bs);

			if (whole_bytes_remaining > 0 && bs->unaligned_byte_count == 0) {
				if (!(dmc_unrar_archive_seek(&bs->io, bs->io.offset + whole_bytes_remaining)))
					return false;

				bits_to_seek -= whole_bytes_remaining * 8;
				bs->offset_bits += whole_bytes_remaining * 8;
			}
		}
	}

	if (bits_to_seek > 0) {
		if (!dmc_unrar_bs_reload_cache(bs))
			return false;

		return dmc_unrar_bs_seek_bits(bs, bits_to_seek);
	}

	return true;
}

static bool dmc_unrar_bs_peek_uint32(dmc_unrar_bs *bs, unsigned int bit_count,
		uint32_t *result_out) {

	DMC_UNRAR_ASSERT(bs != NULL);
	DMC_UNRAR_ASSERT(result_out != NULL);
	DMC_UNRAR_ASSERT(bit_count > 0);
	DMC_UNRAR_ASSERT(bit_count <= 32);

	if (bs->consumed_bits == DMC_UNRAR_BS_L1_SIZE_BITS(bs))
		if (!dmc_unrar_bs_reload_cache(bs))
			return false;

	if (bit_count <= DMC_UNRAR_BS_L1_BITS_REMAINING(bs)) {
		if (bit_count < DMC_UNRAR_BS_L1_SIZE_BITS(bs))
			*result_out = DMC_UNRAR_BS_L1_SELECT_AND_SHIFT(bs, bit_count);
		else
			*result_out = (uint32_t)bs->cache;

		return true;
	}

	/* It straddles the cached. Peek in 2 parts. */

	{
		size_t bit_count_hi = DMC_UNRAR_BS_L1_BITS_REMAINING(bs);
		size_t bit_count_lo = bit_count - bit_count_hi;
		uint32_t result_hi = DMC_UNRAR_BS_L1_SELECT_AND_SHIFT(bs, bit_count_hi);
		dmc_unrar_bs_cache_t result_lo;

		/* Do we need to refill the L2 cache first?. */
		if (bs->next_l2_line >= DMC_UNRAR_BS_L2_LINE_COUNT(bs))
			dmc_unrar_bs_refill_l2_cache_from_client(bs);


		if (bs->next_l2_line < DMC_UNRAR_BS_L2_LINE_COUNT(bs)) {
			/* We can get the remainder from the L2 cache. */

			result_lo = dmc_unrar_bs_be2host_cache_line(bs->cache_l2[bs->next_l2_line]);

		} else {
			/* L2 cache is empty, so the client is empty. */

			/* Do we have enough data in the unaligned cache? If not, give up. */
			if ((bs->unaligned_byte_count * 8) < bit_count_lo)
				return false;

			/* If we do, we get the remainder from there. */
			result_lo = dmc_unrar_bs_be2host_cache_line(bs->unaligned_cache);
		}

		result_lo = (result_lo & DMC_UNRAR_BS_L1_SELECTION_MASK(bit_count_lo)) >>
		            (DMC_UNRAR_BS_L1_SIZE_BITS(bs) - (bit_count_lo));

		*result_out = (result_hi << bit_count_lo) | result_lo;
	}

	return true;
}

static bool dmc_unrar_bs_read_uint32(dmc_unrar_bs *bs, unsigned int bit_count,
		uint32_t *result_out) {

	if (bit_count == 0) {
		*result_out = 0;
		return true;
	}

	bs->offset_bits += bit_count;

	DMC_UNRAR_ASSERT(bs != NULL);
	DMC_UNRAR_ASSERT(result_out != NULL);
	DMC_UNRAR_ASSERT(bit_count <= 32);

	if (bs->consumed_bits == DMC_UNRAR_BS_L1_SIZE_BITS(bs))
		if (!dmc_unrar_bs_reload_cache(bs))
			return false;

	if (bit_count <= DMC_UNRAR_BS_L1_BITS_REMAINING(bs)) {
		if (bit_count < DMC_UNRAR_BS_L1_SIZE_BITS(bs)) {
			*result_out = DMC_UNRAR_BS_L1_SELECT_AND_SHIFT(bs, bit_count);
			bs->consumed_bits += bit_count;
			bs->cache <<= bit_count;
		} else {
			*result_out = (uint32_t)bs->cache;
			bs->consumed_bits = DMC_UNRAR_BS_L1_SIZE_BITS(bs);
			bs->cache = 0;
		}

		return true;
	}

	{
		/* It straddles the cached data. It will never cover more than the next chunk.
		 * We just read the number in two parts and combine them. */

		size_t bit_count_hi = DMC_UNRAR_BS_L1_BITS_REMAINING(bs);
		size_t bit_count_lo = bit_count - bit_count_hi;
		uint32_t result_hi = DMC_UNRAR_BS_L1_SELECT_AND_SHIFT(bs, bit_count_hi);

		if (!dmc_unrar_bs_reload_cache(bs))
			return false;

		*result_out = (result_hi << bit_count_lo) | DMC_UNRAR_BS_L1_SELECT_AND_SHIFT(bs, bit_count_lo);
		bs->consumed_bits += bit_count_lo;
		bs->cache <<= bit_count_lo;
	}

	return true;
}

static uint32_t dmc_unrar_bs_read_bits(dmc_unrar_bs *bs, size_t n) {
	uint32_t data = 0;
	if (bs->error || !dmc_unrar_bs_read_uint32(bs, n, &data)) {
		bs->error = true;
		return 0;
	}

	return data;
}

static uint32_t dmc_unrar_bs_peek_bits(dmc_unrar_bs *bs, size_t n) {
	uint32_t data = 0;
	if (bs->error || !dmc_unrar_bs_peek_uint32(bs, n, &data)) {
		bs->error = true;
		return 0;
	}

	return data;
}

static bool dmc_unrar_bs_skip_bits(dmc_unrar_bs *bs, size_t n) {
	if (bs->error || !dmc_unrar_bs_seek_bits(bs, n)) {
		bs->error = true;
		return false;
	}

	return true;
}

static bool dmc_unrar_bs_skip_to_byte_boundary(dmc_unrar_bs *bs) {
	return dmc_unrar_bs_skip_bits(bs, (8 - (bs->offset_bits % 8)) % 8);
}

static bool dmc_unrar_bs_has_error(dmc_unrar_bs *bs) {
	return bs->error;
}

static bool dmc_unrar_bs_eos(dmc_unrar_bs *bs) {
	/* We still have data left in the client. */
	if (bs->io.offset < bs->io.size)
		return false;

	/* We still have data left in the L2 cache, either unaligned or aligned. */
	if ((bs->unaligned_byte_count > 0) || DMC_UNRAR_BS_L2_LINES_REMAINING(bs) > 0)
		return false;

	/* We still have data left in the L1 cache. */
	if (DMC_UNRAR_BS_L1_BITS_REMAINING(bs) > 0)
		return false;

	/* No data left, end of stream reached. */
	return true;
}

static bool dmc_unrar_bs_has_at_least(dmc_unrar_bs *bs, size_t n) {
	size_t count1, count2;
	uint64_t client_bytes;

	DMC_UNRAR_ASSERT(n <= 32);

	if (DMC_UNRAR_BS_L2_LINES_REMAINING(bs) > 0)
		return true;

	client_bytes = bs->io.size - bs->io.offset;
	if (client_bytes >= 4)
		return true;

	count1 = DMC_UNRAR_BS_L1_BITS_REMAINING(bs);
	count2 = (client_bytes + bs->unaligned_byte_count) * 8;

	return (count1 + count2) >= n;
}
/* '--- */

/* .--- Huffman implementation */
static dmc_unrar_return dmc_unrar_huff_create_tree(dmc_unrar_huff *huff, uint8_t max_length,
	size_t code_count, const uint32_t *codes, const uint8_t *lengths, const uint32_t *symbols);
static dmc_unrar_return dmc_unrar_huff_create_tree_from_lengths(dmc_unrar_huff *huff,
	const uint8_t *lengths, size_t code_count, uint8_t max_length);
static dmc_unrar_return dmc_unrar_huff_create_table(dmc_unrar_huff *huff, uint8_t max_length);

static uint8_t dmc_unrar_huff_find_max_length(uint8_t max_length, const uint8_t *lengths,
		size_t lengths_count) {

	size_t i;

	if (max_length == 0)
		for (i = 0; i < lengths_count; i++)
			max_length = DMC_UNRAR_MAX(max_length, lengths[i]);

	if ((max_length == 0) || (lengths_count == 0))
		return 0;

	return max_length;
}

static dmc_unrar_return dmc_unrar_huff_create(dmc_unrar_alloc *alloc, dmc_unrar_huff *huff,
	uint8_t max_length, size_t code_count, const uint32_t *codes, const uint8_t *lengths,
	const uint32_t *symbols) {

	dmc_unrar_return return_code = DMC_UNRAR_OK;

	DMC_UNRAR_ASSERT(alloc && huff);
	DMC_UNRAR_ASSERT(codes && lengths && symbols);
	DMC_UNRAR_ASSERT(code_count > 0);

	max_length = dmc_unrar_huff_find_max_length(max_length, lengths, code_count);

	DMC_UNRAR_ASSERT(max_length > 0 && max_length <= 20);

	DMC_UNRAR_CLEAR_OBJ(*huff);

	huff->alloc = alloc;

	return_code = dmc_unrar_huff_create_tree(huff, max_length, code_count, codes, lengths, symbols);
	if (return_code != DMC_UNRAR_OK)
		goto fail;

	return_code = dmc_unrar_huff_create_table(huff, max_length);
	if (return_code != DMC_UNRAR_OK)
		goto fail;

	return DMC_UNRAR_OK;

fail:
	dmc_unrar_huff_destroy(huff);
	return return_code;
}

static dmc_unrar_return dmc_unrar_huff_create_from_lengths(dmc_unrar_alloc *alloc,
		dmc_unrar_huff *huff, const uint8_t *lengths, size_t code_count, uint8_t max_length) {

	dmc_unrar_return return_code = DMC_UNRAR_OK;

	DMC_UNRAR_ASSERT(alloc && huff && lengths);
	DMC_UNRAR_ASSERT(code_count > 0);
	DMC_UNRAR_ASSERT(max_length > 0 && max_length <= 20);

	DMC_UNRAR_CLEAR_OBJ(*huff);

	huff->alloc = alloc;

	return_code = dmc_unrar_huff_create_tree_from_lengths(huff, lengths, code_count, max_length);
	if (return_code != DMC_UNRAR_OK)
		goto fail;

	return_code = dmc_unrar_huff_create_table(huff, max_length);
	if (return_code != DMC_UNRAR_OK)
		goto fail;

	return DMC_UNRAR_OK;

fail:
	dmc_unrar_huff_destroy(huff);
	return return_code;
}

static void dmc_unrar_huff_destroy(dmc_unrar_huff *huff) {
	if (!huff)
		return;

	if (!huff->alloc) {
		DMC_UNRAR_CLEAR_OBJ(*huff);
		return;
	}

	dmc_unrar_free(huff->alloc, huff->tree);
	dmc_unrar_free(huff->alloc, huff->table);

	DMC_UNRAR_CLEAR_OBJ(*huff);
}

typedef enum {
	DMC_UNRAR_HUFF_TREE_BRANCH_LEFT  = 0,
	DMC_UNRAR_HUFF_TREE_BRANCH_RIGHT = 1
} dmc_unrar_huff_tree_branch;

static uint32_t *dmc_unrar_huff_tree_node_get(dmc_unrar_huff *huff, size_t index) {
	return &huff->tree[index * 2];
}

static bool dmc_unrar_huff_tree_node_is_invalid(uint32_t node_value) {
	return node_value == 0xFFFFFFFF;
}

static bool dmc_unrar_huff_tree_node_is_open_branch(const uint32_t *node,
		dmc_unrar_huff_tree_branch branch) {

	return dmc_unrar_huff_tree_node_is_invalid(node[branch]);
}

static bool dmc_unrar_huff_tree_node_is_leaf(const uint32_t *node) {
	return (node[DMC_UNRAR_HUFF_TREE_BRANCH_LEFT] == node[DMC_UNRAR_HUFF_TREE_BRANCH_RIGHT]) &&
	       !dmc_unrar_huff_tree_node_is_invalid(node[DMC_UNRAR_HUFF_TREE_BRANCH_LEFT]);
}

static bool dmc_unrar_huff_tree_node_is_empty(const uint32_t *node) {
	return dmc_unrar_huff_tree_node_is_invalid(node[DMC_UNRAR_HUFF_TREE_BRANCH_LEFT]) &&
	       dmc_unrar_huff_tree_node_is_invalid(node[DMC_UNRAR_HUFF_TREE_BRANCH_RIGHT]);
}

static void dmc_unrar_huff_tree_node_set_leaf(uint32_t *node, uint32_t value) {
	node[DMC_UNRAR_HUFF_TREE_BRANCH_LEFT] = node[DMC_UNRAR_HUFF_TREE_BRANCH_RIGHT] = value;
}

static void dmc_unrar_huff_tree_node_set_empty(uint32_t *node) {
	dmc_unrar_huff_tree_node_set_leaf(node, 0xFFFFFFFF);
}

static void dmc_unrar_huff_tree_node_set_branch(uint32_t *node, dmc_unrar_huff_tree_branch branch,
		size_t index) {

	node[branch] = index;
}

static size_t dmc_unrar_huff_tree_node_new(dmc_unrar_huff *huff) {
	dmc_unrar_huff_tree_node_set_empty(dmc_unrar_huff_tree_node_get(huff, huff->node_count));

	return huff->node_count++;
}

static uint32_t *dmc_unrar_huff_tree_node_follow_branch(dmc_unrar_huff *huff,
		uint32_t *node, dmc_unrar_huff_tree_branch branch) {

	uint32_t value;

	DMC_UNRAR_ASSERT(!dmc_unrar_huff_tree_node_is_leaf(node));

	value = node[branch];
	DMC_UNRAR_ASSERT(!dmc_unrar_huff_tree_node_is_invalid(value));

	return dmc_unrar_huff_tree_node_get(huff, value);
}

static uint32_t dmc_unrar_huff_tree_node_get_leaf_value(const uint32_t *node) {
	DMC_UNRAR_ASSERT(dmc_unrar_huff_tree_node_is_leaf(node));

	return node[DMC_UNRAR_HUFF_TREE_BRANCH_LEFT];
}

static dmc_unrar_return dmc_unrar_huff_tree_node_add(dmc_unrar_huff *huff,
		uint32_t code, uint8_t length, uint32_t symbol) {

	if (symbol & 0xF8000000)
		return DMC_UNRAR_HUFF_RESERVED_SYMBOL;

	{
		uint32_t *node = dmc_unrar_huff_tree_node_get(huff, 0);
		int bit_pos;

		for (bit_pos = length - 1; bit_pos >= 0; bit_pos--) {
			dmc_unrar_huff_tree_branch branch = (dmc_unrar_huff_tree_branch)((code >> bit_pos) & 1);

			if (dmc_unrar_huff_tree_node_is_leaf(node))
				return DMC_UNRAR_HUFF_PREFIX_PRESENT;

			if (dmc_unrar_huff_tree_node_is_open_branch(node, branch))
				dmc_unrar_huff_tree_node_set_branch(node, branch, dmc_unrar_huff_tree_node_new(huff));

			node = dmc_unrar_huff_tree_node_follow_branch(huff, node, branch);
		}

		if (!dmc_unrar_huff_tree_node_is_empty(node))
			return DMC_UNRAR_HUFF_PREFIX_PRESENT;

		dmc_unrar_huff_tree_node_set_leaf(node, symbol);
	}

	return DMC_UNRAR_OK;
}

static dmc_unrar_return dmc_unrar_huff_create_tree(dmc_unrar_huff *huff, uint8_t max_length,
		size_t code_count, const uint32_t *codes, const uint8_t *lengths, const uint32_t *symbols) {

	const size_t tree_size = (1 << (max_length + 1)) * 2;
	size_t i;

	huff->tree = (uint32_t *)dmc_unrar_malloc(huff->alloc, tree_size, sizeof(uint32_t));
	if (!huff->tree)
		return DMC_UNRAR_ALLOC_FAIL;

	huff->node_count = 0;

	/* Root node. */
	dmc_unrar_huff_tree_node_new(huff);

	for (i = 0; i < code_count; i++) {
		if ((lengths[i] != 0) && (lengths[i] <= max_length)) {
			const dmc_unrar_return added = dmc_unrar_huff_tree_node_add(huff, codes[i], lengths[i], symbols[i]);
			if (added != DMC_UNRAR_OK)
				return added;
		}
	}

	return DMC_UNRAR_OK;
}

static dmc_unrar_return dmc_unrar_huff_create_tree_from_lengths(dmc_unrar_huff *huff,
		const uint8_t *lengths, size_t code_count, uint8_t max_length) {

	uint8_t length;
	uint32_t code = 0;
	size_t codes_left = code_count, i;

	const size_t tree_size = (1 << (max_length + 1)) * 2;

	huff->tree = (uint32_t *)dmc_unrar_malloc(huff->alloc, tree_size, sizeof(uint32_t));
	if (!huff->tree)
		return DMC_UNRAR_ALLOC_FAIL;

	huff->node_count = 0;

	/* Root node. */
	dmc_unrar_huff_tree_node_new(huff);

	for (length = 1; length <= max_length; length++) {
		for (i = 0; i < code_count; i++) {
			dmc_unrar_return added;

			if (lengths[i] != length)
				continue;

			added = dmc_unrar_huff_tree_node_add(huff, code, length, i);
			if (added != DMC_UNRAR_OK)
				return added;

			code++;
			if (--codes_left == 0)
				break;
		}

		code <<= 1;
	}

	return DMC_UNRAR_OK;
}

static void dmc_unrar_huff_table_create(dmc_unrar_huff *huff, size_t node, uint32_t *table,
		size_t depth, size_t max_depth) {

	size_t cur_table_size = 1 << (max_depth - depth);

	if (dmc_unrar_huff_tree_node_is_invalid(node)) {
		size_t i;

		for (i = 0; i < cur_table_size; i++)
			table[i] = 0xFFFFFFFF;

		return;
	}

	{
		const uint32_t *node_pointer = dmc_unrar_huff_tree_node_get(huff, node);

		if (dmc_unrar_huff_tree_node_is_leaf(node_pointer)) {
			const uint32_t value = (depth << 27) | dmc_unrar_huff_tree_node_get_leaf_value(node_pointer);
			size_t i;

			for (i = 0; i < cur_table_size; i++)
				table[i] = value;

			return;
		}

		if (depth == max_depth) {
			table[0] = ((max_depth + 1) << 27) | node;
			return;
		}

		dmc_unrar_huff_table_create(huff, node_pointer[0], table, depth + 1, max_depth);
		dmc_unrar_huff_table_create(huff, node_pointer[1], table + cur_table_size / 2, depth + 1, max_depth);
	}
}

static dmc_unrar_return dmc_unrar_huff_create_table(dmc_unrar_huff *huff, uint8_t max_length) {
	huff->table_size = DMC_UNRAR_MIN(max_length, DMC_UNRAR_HUFF_MAX_TABLE_DEPTH);

	huff->table = (uint32_t *)dmc_unrar_malloc(huff->alloc, 1 << huff->table_size, sizeof(uint32_t));
	if (!huff->table)
		return DMC_UNRAR_ALLOC_FAIL;

	dmc_unrar_huff_table_create(huff, 0, huff->table, 0, huff->table_size);

	return DMC_UNRAR_OK;
}

static uint32_t dmc_unrar_huff_get_symbol(dmc_unrar_huff *huff, dmc_unrar_bs *bs,
		dmc_unrar_return *err) {

	size_t start_node = 0;
	uint32_t *node;

	*err = DMC_UNRAR_OK;

	/* If we want to use the table, we need to make sure we have at least
	 * as many bits left in the bitstream as the table is deep. Otherwise,
	 * we're going to peek past the end of the bitstream. */

	if (dmc_unrar_bs_has_at_least(bs, huff->table_size)) {
		size_t length;
		uint32_t value;

		const uint32_t table_entry = huff->table[dmc_unrar_bs_peek_bits(bs, huff->table_size)];
		if      (dmc_unrar_bs_has_error(bs))
			*err = DMC_UNRAR_READ_FAIL;
		else if (table_entry == 0xFFFFFFFF)
			*err = DMC_UNRAR_HUFF_INVALID_CODE;

		if (*err != DMC_UNRAR_OK)
			return 0xFFFFFFFF;

		length = table_entry >> 27;
		value  = table_entry & 0x07FFFFFF;

		if (length <= huff->table_size) {
			dmc_unrar_bs_skip_bits(bs, length);
			return value;
		}

		dmc_unrar_bs_skip_bits(bs, huff->table_size);
		start_node = value;
	}

	node = dmc_unrar_huff_tree_node_get(huff, start_node);

	while (!dmc_unrar_huff_tree_node_is_leaf(node)) {
		dmc_unrar_huff_tree_branch branch = (dmc_unrar_huff_tree_branch)dmc_unrar_bs_read_bits(bs, 1);

		if (dmc_unrar_huff_tree_node_is_open_branch(node, branch)) {
			*err = DMC_UNRAR_HUFF_INVALID_CODE;
			return 0xFFFFFFFF;
		}

		node = dmc_unrar_huff_tree_node_follow_branch(huff, node, branch);
	}

	if (dmc_unrar_bs_has_error(bs)) {
		*err = DMC_UNRAR_READ_FAIL;
		return 0xFFFFFFFF;
	}

	return dmc_unrar_huff_tree_node_get_leaf_value(node);
}
/* '--- */

/* .--- LZSS implementation */
static bool dmc_unrar_is_power_2(size_t x) {
	return x && !(x & (x - 1));
}

static dmc_unrar_return dmc_unrar_lzss_create(dmc_unrar_alloc *alloc, dmc_unrar_lzss *lzss,
		size_t window_size) {

	DMC_UNRAR_ASSERT(alloc && lzss);
	DMC_UNRAR_ASSERT(window_size && dmc_unrar_is_power_2(window_size));

	DMC_UNRAR_CLEAR_OBJ(*lzss);

	lzss->alloc = alloc;

	lzss->window_size = window_size;
	lzss->window_mask = window_size - 1;

	lzss->window = (uint8_t *)dmc_unrar_malloc(alloc, window_size, sizeof(uint8_t));
	if (!lzss->window)
		return DMC_UNRAR_ALLOC_FAIL;

	return DMC_UNRAR_OK;
}

static void dmc_unrar_lzss_destroy(dmc_unrar_lzss *lzss) {
	if (!lzss)
		return;

	if (lzss->alloc)
		dmc_unrar_free(lzss->alloc, lzss->window);

	DMC_UNRAR_CLEAR_OBJ(*lzss);
}

static size_t dmc_unrar_lzss_emit_literal_no_buffer(dmc_unrar_lzss *lzss,
		size_t buffer_size, size_t buffer_offset, uint8_t literal, size_t *running_output_count) {

	DMC_UNRAR_ASSERT(lzss);
	DMC_UNRAR_ASSERT(buffer_offset < buffer_size);

	buffer_offset++;

	lzss->window[lzss->window_offset++ & lzss->window_mask] = literal;

	if (running_output_count)
		(*running_output_count)++;

	return buffer_offset;
}

static size_t dmc_unrar_lzss_emit_literal(dmc_unrar_lzss *lzss, uint8_t *buffer,
		size_t buffer_size, size_t buffer_offset, uint8_t literal, size_t *running_output_count) {

	if (!buffer)
		return dmc_unrar_lzss_emit_literal_no_buffer(lzss, buffer_size,
			buffer_offset, literal, running_output_count);

	DMC_UNRAR_ASSERT(lzss);
	DMC_UNRAR_ASSERT(buffer_offset < buffer_size);

	buffer[buffer_offset++] = literal;
	lzss->window[lzss->window_offset++ & lzss->window_mask] = literal;

	if (running_output_count)
		(*running_output_count)++;

	return buffer_offset;
}

static size_t dmc_unrar_lzss_emit_copy(dmc_unrar_lzss *lzss, uint8_t *buffer,
		size_t buffer_size, size_t buffer_offset, size_t copy_offset, size_t copy_size,
		size_t *running_output_count) {

	DMC_UNRAR_ASSERT(lzss);
	DMC_UNRAR_ASSERT(copy_offset <= lzss->window_offset);

	/* Convert relative offset into absolute output buffer offset. */
	copy_offset = lzss->window_offset - copy_offset;

	if (buffer) {
		while ((buffer_offset < buffer_size) && (copy_size-- > 0)) {
			const uint8_t data = lzss->window[copy_offset++ & lzss->window_mask];

			buffer_offset = dmc_unrar_lzss_emit_literal(lzss, buffer, buffer_size,
				buffer_offset, data, running_output_count);
		}
	} else {
		while ((buffer_offset < buffer_size) && (copy_size-- > 0)) {
			const uint8_t data = lzss->window[copy_offset++ & lzss->window_mask];

			buffer_offset = dmc_unrar_lzss_emit_literal_no_buffer(lzss,
				buffer_size, buffer_offset, data, running_output_count);
		}
	}

	if ((buffer_offset == buffer_size) && (copy_size > 0)) {
		/** Remember the overhang. */

		lzss->copy_offset = lzss->window_offset - copy_offset;
		lzss->copy_size   = copy_size;

	} else {
		lzss->copy_offset = 0;
		lzss->copy_size   = 0;
	}

	return buffer_offset;
}

static bool dmc_unrar_lzss_has_overhang(dmc_unrar_lzss *lzss) {
	DMC_UNRAR_ASSERT(lzss);

	return lzss->copy_size > 0;
}

static size_t dmc_unrar_lzss_emit_copy_overhang(dmc_unrar_lzss *lzss, uint8_t *buffer,
	size_t buffer_size, size_t buffer_offset, size_t *running_output_count) {

	DMC_UNRAR_ASSERT(lzss);

	if (lzss->copy_size == 0)
		return buffer_offset;

	return dmc_unrar_lzss_emit_copy(lzss, buffer, buffer_size, buffer_offset,
	                                lzss->copy_offset, lzss->copy_size, running_output_count);
}

/* '--- */

/* .--- PPMd implementation */
#if DMC_UNRAR_DISABLE_PPMD != 1
/* This is a PPMd, variant H, decoder, based on the code found in The
 * Unarchiver, based on Igor Pavlov's implementation in 7-zip, based
 * on Dmitry Shkarin's PPMd algorithm. The Unarchiver is released
 * under the terms of the LGPLv2.1 or later. */

#define DMC_UNRAR_PPMD_MAX_O       255
#define DMC_UNRAR_PPMD_INT_BITS      7
#define DMC_UNRAR_PPMD_PERIOD_BITS   7
#define DMC_UNRAR_PPMD_TOTAL_BITS  (DMC_UNRAR_PPMD_INT_BITS + DMC_UNRAR_PPMD_PERIOD_BITS)

#define DMC_UNRAR_PPMD_MAX_FREQ    124
#define DMC_UNRAR_PPMD_INTERVAL    (1 << DMC_UNRAR_PPMD_INT_BITS)
#define DMC_UNRAR_PPMD_BIN_SCALE   (1 << DMC_UNRAR_PPMD_TOTAL_BITS)

#define DMC_UNRAR_PPMD_N1 4
#define DMC_UNRAR_PPMD_N2 4
#define DMC_UNRAR_PPMD_N3 4
#define DMC_UNRAR_PPMD_N4 ((128 + 3-1 * DMC_UNRAR_PPMD_N1-2 * DMC_UNRAR_PPMD_N2-3 * DMC_UNRAR_PPMD_N3)/4)

#define DMC_UNRAR_PPMD_UNIT_SIZE 12
#define DMC_UNRAR_PPMD_N_INDICES (DMC_UNRAR_PPMD_N1 + DMC_UNRAR_PPMD_N2 + DMC_UNRAR_PPMD_N3 + DMC_UNRAR_PPMD_N4)

#define DMC_UNRAR_PPMD_GET_MEAN(SUMM, SHIFT, ROUND) ((SUMM+(1<<(SHIFT-ROUND)))>>(SHIFT))

struct dmc_unrar_ppmd_suballocator_tag;
typedef struct dmc_unrar_ppmd_suballocator_tag dmc_unrar_ppmd_suballocator;

struct dmc_unrar_ppmd_suballocator_tag {
	void (*init)(dmc_unrar_ppmd_suballocator *self);
	uint32_t (*alloc_context)(dmc_unrar_ppmd_suballocator *self);
	uint32_t (*alloc_units)(dmc_unrar_ppmd_suballocator *self, int num);  /* 1 unit == 12 bytes, nu <= 128. */
	uint32_t (*expand_units)(dmc_unrar_ppmd_suballocator *self, uint32_t old_offs, int old_num);
	uint32_t (*shrink_units)(dmc_unrar_ppmd_suballocator *self, uint32_t old_offs, int old_num, int new_num);
	void (*free_units)(dmc_unrar_ppmd_suballocator *self, uint32_t offs, int num);
};

#pragma pack(push, 1)
typedef struct dmc_unrar_ppmd_memory_block_h_tag {
	uint16_t stamp, nu;
	uint32_t next, prev;
} dmc_unrar_ppmd_memory_block_h;
#pragma pack(pop)

struct dmc_unrar_ppmd_allocator_node_h_tag;
typedef struct dmc_unrar_ppmd_allocator_node_h_tag dmc_unrar_ppmd_allocator_node_h;

struct dmc_unrar_ppmd_allocator_node_h_tag {
	dmc_unrar_ppmd_allocator_node_h *next;
};

struct dmc_unrar_ppmd_suballocator_h_tag {
	dmc_unrar_ppmd_suballocator core;

	uint32_t suballocator_size;
	uint8_t index2units[38], units2index[128], glue_count;
	uint8_t *text, *units_start, *low_unit, *high_unit;

	dmc_unrar_ppmd_allocator_node_h free_list[38];

	dmc_unrar_ppmd_memory_block_h sentinel;
	uint8_t heap_start[1];
};

typedef int dmc_unrar_ppmd_read_func(void *context);

typedef struct dmc_unrar_ppmd_range_coder_tag {
	dmc_unrar_ppmd_read_func *read_func;
	void *input_context;

	uint32_t low, code, range, bottom;
	bool use_low;

} dmc_unrar_ppmd_range_coder;

#pragma pack(push, 1)
/** SEE-contexts for PPMd-contexts with masked symbols .*/
typedef struct dmc_unrar_ppmd_see_context_tag {
	uint16_t summ;
	uint8_t shift, count;
} dmc_unrar_ppmd_see_context;

typedef struct dmc_unrar_ppmd_state_tag {
	uint8_t symbol, freq;
	uint32_t successor;
} dmc_unrar_ppmd_state;

typedef struct dmc_unrar_ppmd_context_tag {
	uint8_t  last_state_index, flags;
	uint16_t summ_freq;
	uint32_t states;
	uint32_t suffix;
} dmc_unrar_ppmd_context;
#pragma pack(pop)

struct dmc_unrar_ppmd_core_model_tag;
typedef struct dmc_unrar_ppmd_core_model_tag dmc_unrar_ppmd_core_model;

struct dmc_unrar_ppmd_core_model_tag {
	dmc_unrar_ppmd_suballocator *alloc;

	dmc_unrar_ppmd_range_coder coder;
	uint32_t scale;

	dmc_unrar_ppmd_state *found_state; /* found next state transition. */
	int order_fall, init_esc, run_length, init_rl;
	uint8_t char_mask[256];
	uint8_t last_mask_index, esc_count, prev_success;

	void (*rescale_ppmd_context)(dmc_unrar_ppmd_context *self, dmc_unrar_ppmd_core_model *model);
};

struct dmc_unrar_ppmd_model_h_tag {
	dmc_unrar_ppmd_core_model core;

	dmc_unrar_ppmd_suballocator_h *alloc;

	dmc_unrar_ppmd_context *min_context, *max_context;
	int max_order, hi_bits_flag;
	bool seven_zip;

	dmc_unrar_ppmd_see_context see_context[25][16], dummy_see_context;
	uint8_t ns2bs_index[256], hb2flag[256], ns2index[256];
	uint16_t bin_summ[128][64]; /* binary SEE-contexts. */
};


static dmc_unrar_ppmd_suballocator_h *dmc_unrar_ppmd_suballocator_h_create(dmc_unrar_alloc *alloc,
	int size);
static void dmc_unrar_ppmd_suballocator_h_destroy(dmc_unrar_alloc *alloc,
	dmc_unrar_ppmd_suballocator_h *self);

static void dmc_unrar_ppmd_range_coder_init(dmc_unrar_ppmd_range_coder *self,
	dmc_unrar_ppmd_read_func *read_func, void *input_context, bool use_low, int bottom);

static uint32_t dmc_unrar_ppmd_range_coder_current_count(dmc_unrar_ppmd_range_coder *self,
	uint32_t scale);
static void dmc_unrar_ppmd_range_coder_remove_sub_range(dmc_unrar_ppmd_range_coder *self,
	uint32_t low_count, uint32_t high_count);

static int dmc_unrar_ppmd_range_coder_next_weighted_bit(dmc_unrar_ppmd_range_coder *self,
	int weight, int size);

static int dmc_unrar_ppmd_range_coder_next_weighted_bit2(dmc_unrar_ppmd_range_coder *self,
	int weight, int shift);

static void dmc_unrar_ppmd_range_coder_normalize(dmc_unrar_ppmd_range_coder *self);

static dmc_unrar_ppmd_see_context dmc_unrar_ppmd_see_create(int init_val, int count);
static unsigned int dmc_unrar_ppmd_see_get_mean(dmc_unrar_ppmd_see_context *self);
static void dmc_unrar_ppmd_see_update(dmc_unrar_ppmd_see_context *self);

static dmc_unrar_ppmd_context *dmc_unrar_ppmd_state_successor(dmc_unrar_ppmd_state *self,
	dmc_unrar_ppmd_core_model *model);
static void dmc_unrar_ppmd_state_set_successor_pointer(dmc_unrar_ppmd_state *self,
	dmc_unrar_ppmd_context *new_successor, dmc_unrar_ppmd_core_model *model);
static dmc_unrar_ppmd_state *dmc_unrar_ppmd_context_states(dmc_unrar_ppmd_context *self,
	dmc_unrar_ppmd_core_model *model);
static void dmc_unrar_ppmd_context_set_states_pointer(dmc_unrar_ppmd_context *self,
	dmc_unrar_ppmd_state *new_states, dmc_unrar_ppmd_core_model *model);
static dmc_unrar_ppmd_context *dmc_unrar_ppmd_context_suffix(dmc_unrar_ppmd_context *self,
	dmc_unrar_ppmd_core_model *model);
static void dmc_unrar_ppmd_context_set_suffix_pointer(dmc_unrar_ppmd_context *self,
	dmc_unrar_ppmd_context *new_suffix, dmc_unrar_ppmd_core_model *model);
static dmc_unrar_ppmd_state *dmc_unrar_ppmd_context_one_state(dmc_unrar_ppmd_context *self);

static dmc_unrar_ppmd_context *dmc_unrar_ppmd_new_context(dmc_unrar_ppmd_core_model *model);
static dmc_unrar_ppmd_context *dmc_unrar_ppmd_new_context_as_child_of(dmc_unrar_ppmd_core_model *model,
	dmc_unrar_ppmd_context *suffix_context, dmc_unrar_ppmd_state *suffix_state,
	dmc_unrar_ppmd_state *first_state);

static void dmc_unrar_ppmd_decode_bin_symbol(dmc_unrar_ppmd_context *self,
	dmc_unrar_ppmd_core_model *model, uint16_t *bs, int freq_limit, bool alt_next_bit);
static int dmc_unrar_ppmd_decode_symbol1(dmc_unrar_ppmd_context *self,
	dmc_unrar_ppmd_core_model *model, bool greater_or_equal);
static void dmc_unrar_ppmd_context_update1(dmc_unrar_ppmd_context *self,
	dmc_unrar_ppmd_core_model *model, dmc_unrar_ppmd_state *state);
static void dmc_unrar_ppmd_decode_symbol2(dmc_unrar_ppmd_context *self,
	dmc_unrar_ppmd_core_model *model, dmc_unrar_ppmd_see_context *see);
static void dmc_unrar_ppmd_context_update2(dmc_unrar_ppmd_context *self,
	dmc_unrar_ppmd_core_model *model, dmc_unrar_ppmd_state *state);
static void dmc_unrar_ppmd_rescale_context(dmc_unrar_ppmd_context *self,
		dmc_unrar_ppmd_core_model *model);

static void dmc_unrar_ppmd_clear_model_mask(dmc_unrar_ppmd_core_model *self);

static void dmc_unrar_ppmd_start_model_h(dmc_unrar_ppmd_model_h *self,
	dmc_unrar_ppmd_read_func *read_func, void *input_context, dmc_unrar_ppmd_suballocator_h *alloc,
	int max_order, bool seven_zip);

static void dmc_unrar_ppmd_range_coder_restart(dmc_unrar_ppmd_model_h *self,
	dmc_unrar_ppmd_read_func *read_func, void *input_context, bool seven_zip);

static int dmc_unrar_ppmd_next_byte(dmc_unrar_ppmd_model_h *self);

static void dmc_unrar_ppmd_suballoc_h_node_insert(dmc_unrar_ppmd_suballocator_h *self,
	void *p, int index);
static void *dmc_unrar_ppmd_suballoc_h_node_remove(dmc_unrar_ppmd_suballocator_h *self,
	int index);
static unsigned int dmc_unrar_ppmd_suballoc_h_i2b(dmc_unrar_ppmd_suballocator_h *self,
	int index);
static void dmc_unrar_ppmd_suballoc_h_block_split(dmc_unrar_ppmd_suballocator_h *self,
	void *pv, int old_index, int new_index);

static void dmc_unrar_ppmd_suballoc_h_init(dmc_unrar_ppmd_suballocator_h *self);
static uint32_t dmc_unrar_ppmd_suballoc_h_alloc_context(dmc_unrar_ppmd_suballocator_h *self);
static uint32_t dmc_unrar_ppmd_suballoc_h_alloc_units(dmc_unrar_ppmd_suballocator_h *self,
	int num);
static uint32_t dmc_unrar_ppmd_suballoc_h_alloc_units_internal(dmc_unrar_ppmd_suballocator_h *self,
	int index);
static uint32_t dmc_unrar_ppmd_suballoc_h_expand_units(dmc_unrar_ppmd_suballocator_h *self,
	uint32_t old_offs, int old_num);
static uint32_t dmc_unrar_ppmd_suballoc_h_shrink_units(dmc_unrar_ppmd_suballocator_h *self,
	uint32_t old_offs, int old_num, int new_num);
static void dmc_unrar_ppmd_suballoc_h_free_units(dmc_unrar_ppmd_suballocator_h *self,
	uint32_t offs, int num);
static void dmc_unrar_ppmd_suballoc_h_glue_free_blocks(dmc_unrar_ppmd_suballocator_h *self);

static void dmc_unrar_ppmd_model_restart(dmc_unrar_ppmd_model_h *self);
static void dmc_unrar_ppmd_model_update(dmc_unrar_ppmd_model_h *self);

static dmc_unrar_ppmd_context *dmc_unrar_ppmd_create_successors(dmc_unrar_ppmd_model_h *self,
	bool skip, dmc_unrar_ppmd_state *state);

static void dmc_unrar_ppmd_decode_bin_symbol_h(dmc_unrar_ppmd_context *self,
	dmc_unrar_ppmd_model_h *model);
static void dmc_unrar_ppmd_decode_symbol1_h(dmc_unrar_ppmd_context *self,
	dmc_unrar_ppmd_model_h *model);
static void dmc_unrar_ppmd_decode_symbol2_h(dmc_unrar_ppmd_context *self,
	dmc_unrar_ppmd_model_h *model);


static void *dmc_unrar_ppmd_offset_to_pointer(void *base, uint32_t offset) {
	if (!offset)
		return NULL;

	return ((uint8_t *)base) + offset;
}

static uint32_t dmc_unrar_ppmd_pointer_to_offset(void *base, void *pointer) {
	if (!pointer)
		return 0;

	return (uint32_t)(((uintptr_t)pointer) - (uintptr_t)base);
}


static void dmc_unrar_ppmd_init_suballocator(dmc_unrar_ppmd_suballocator *self) {
	self->init(self);
}

static uint32_t dmc_unrar_ppmd_alloc_context(dmc_unrar_ppmd_suballocator *self) {
	return self->alloc_context(self);
}

static uint32_t dmc_unrar_ppmd_alloc_units(dmc_unrar_ppmd_suballocator *self, int num) {
	return self->alloc_units(self, num);
}

static uint32_t dmc_unrar_ppmd_expand_units(dmc_unrar_ppmd_suballocator *self,
                                            uint32_t old_offs, int old_num) {

	return self->expand_units(self, old_offs, old_num);
}

static uint32_t dmc_unrar_ppmd_shrink_units(dmc_unrar_ppmd_suballocator *self,
                                            uint32_t old_offs, int old_num, int new_num) {

	return self->shrink_units(self, old_offs, old_num, new_num);
}

static void dmc_unrar_ppmd_free_units(dmc_unrar_ppmd_suballocator *self, uint32_t offs, int num) {
	self->free_units(self, offs, num);
}


static void dmc_unrar_ppmd_suballoc_h_block_insert_after(dmc_unrar_ppmd_suballocator_h *self,
		dmc_unrar_ppmd_memory_block_h *block, dmc_unrar_ppmd_memory_block_h *preceeding) {

	dmc_unrar_ppmd_memory_block_h *following = (dmc_unrar_ppmd_memory_block_h *)
		dmc_unrar_ppmd_offset_to_pointer(self, preceeding->next);

	block->prev      = dmc_unrar_ppmd_pointer_to_offset(self, preceeding);
	block->next      = dmc_unrar_ppmd_pointer_to_offset(self, following);
	preceeding->next = dmc_unrar_ppmd_pointer_to_offset(self, block);
	following->prev  = dmc_unrar_ppmd_pointer_to_offset(self, block);
}

static void dmc_unrar_ppmd_suballoc_h_block_remove(dmc_unrar_ppmd_suballocator_h *self,
		dmc_unrar_ppmd_memory_block_h *block) {

	dmc_unrar_ppmd_memory_block_h *preceeding = (dmc_unrar_ppmd_memory_block_h *)
		dmc_unrar_ppmd_offset_to_pointer(self, block->prev);
	dmc_unrar_ppmd_memory_block_h *following = (dmc_unrar_ppmd_memory_block_h *)
		dmc_unrar_ppmd_offset_to_pointer(self, block->next);

	preceeding->next = dmc_unrar_ppmd_pointer_to_offset(self, following);
	following->prev  = dmc_unrar_ppmd_pointer_to_offset(self, preceeding);
}


static dmc_unrar_ppmd_suballocator_h *dmc_unrar_ppmd_suballocator_h_create(dmc_unrar_alloc *alloc,
		int size) {

	dmc_unrar_ppmd_suballocator_h *self = (dmc_unrar_ppmd_suballocator_h *)
		dmc_unrar_malloc(alloc, 1, sizeof(dmc_unrar_ppmd_suballocator_h) + size);

	if (!self)
		return NULL;

	self->core.init = (void (*)(dmc_unrar_ppmd_suballocator*))dmc_unrar_ppmd_suballoc_h_init;
	self->core.alloc_context = (uint32_t (*)(dmc_unrar_ppmd_suballocator *))dmc_unrar_ppmd_suballoc_h_alloc_context;
	self->core.alloc_units = (uint32_t (*)(dmc_unrar_ppmd_suballocator *, int))dmc_unrar_ppmd_suballoc_h_alloc_units;
	self->core.expand_units = (uint32_t (*)(dmc_unrar_ppmd_suballocator *, uint32_t, int))dmc_unrar_ppmd_suballoc_h_expand_units;
	self->core.shrink_units = (uint32_t (*)(dmc_unrar_ppmd_suballocator *, uint32_t, int, int))dmc_unrar_ppmd_suballoc_h_shrink_units;
	self->core.free_units = (void (*)(dmc_unrar_ppmd_suballocator*, uint32_t, int))dmc_unrar_ppmd_suballoc_h_free_units;

	self->suballocator_size = size;

	return self;
}

static void dmc_unrar_ppmd_suballocator_h_destroy(dmc_unrar_alloc *alloc,
		dmc_unrar_ppmd_suballocator_h *self) {

	dmc_unrar_free(alloc, self);
}


static void dmc_unrar_ppmd_suballoc_h_init(dmc_unrar_ppmd_suballocator_h *self) {
	unsigned int diff;
	int i, k;

	memset(self->free_list, 0, sizeof(self->free_list));

	self->text = self->heap_start;
	self->high_unit = self->heap_start + self->suballocator_size;

	diff = DMC_UNRAR_PPMD_UNIT_SIZE * (self->suballocator_size / 8 / DMC_UNRAR_PPMD_UNIT_SIZE * 7);
	self->low_unit = self->units_start = self->high_unit-diff;
	self->glue_count = 0;

	for (i = 0; i < DMC_UNRAR_PPMD_N1; i++)
		self->index2units[i] = 1 + i;

	for (i = 0; i < DMC_UNRAR_PPMD_N2; i++)
		self->index2units[DMC_UNRAR_PPMD_N1 + i] = 2 + DMC_UNRAR_PPMD_N1 + i * 2;

	for (i = 0; i < DMC_UNRAR_PPMD_N3; i++)
		self->index2units[DMC_UNRAR_PPMD_N1 + DMC_UNRAR_PPMD_N2 + i] =
			3 + DMC_UNRAR_PPMD_N1 + 2 * DMC_UNRAR_PPMD_N2 + i * 3;

	for (i = 0; i < DMC_UNRAR_PPMD_N4; i++)
		self->index2units[DMC_UNRAR_PPMD_N1 + DMC_UNRAR_PPMD_N2 + DMC_UNRAR_PPMD_N3 + i] =
			4 + DMC_UNRAR_PPMD_N1 + 2 * DMC_UNRAR_PPMD_N2 + 3 * DMC_UNRAR_PPMD_N3 + i * 4;

	for (k = 0, i = 0; k < 128; k++) {
		if (self->index2units[i] < (k + 1))
			i++;

		self->units2index[k] = i;
	}
}

static uint32_t dmc_unrar_ppmd_suballoc_h_alloc_context(dmc_unrar_ppmd_suballocator_h *self) {
	if (self->high_unit != self->low_unit) {
		self->high_unit -= DMC_UNRAR_PPMD_UNIT_SIZE;
		return dmc_unrar_ppmd_pointer_to_offset(self, self->high_unit);
	}

	if (self->free_list->next)
		return dmc_unrar_ppmd_pointer_to_offset(self, dmc_unrar_ppmd_suballoc_h_node_remove(self, 0));

	return dmc_unrar_ppmd_suballoc_h_alloc_units_internal(self, 0);
}

static uint32_t dmc_unrar_ppmd_suballoc_h_alloc_units(dmc_unrar_ppmd_suballocator_h *self,
		int num) {

	void *units;
	int index = self->units2index[num - 1];

	if (self->free_list[index].next)
		return dmc_unrar_ppmd_pointer_to_offset(self, dmc_unrar_ppmd_suballoc_h_node_remove(self, index));

	units = self->low_unit;
	self->low_unit += dmc_unrar_ppmd_suballoc_h_i2b(self, index);
	if (self->low_unit <= self->high_unit)
		return dmc_unrar_ppmd_pointer_to_offset(self, units);

	self->low_unit -= dmc_unrar_ppmd_suballoc_h_i2b(self, index);

	return dmc_unrar_ppmd_suballoc_h_alloc_units_internal(self, index);
}

static uint32_t dmc_unrar_ppmd_suballoc_h_alloc_units_internal(dmc_unrar_ppmd_suballocator_h *self,
		int index) {

	int i;

	if (self->glue_count == 0) {
		self->glue_count = 255;

		dmc_unrar_ppmd_suballoc_h_glue_free_blocks(self);
		if (self->free_list[index].next)
			return dmc_unrar_ppmd_pointer_to_offset(self, dmc_unrar_ppmd_suballoc_h_node_remove(self, index));
	}

	for (i = index + 1; i < DMC_UNRAR_PPMD_N_INDICES; i++) {
		if (self->free_list[i].next) {
			void *units = dmc_unrar_ppmd_suballoc_h_node_remove(self, i);

			dmc_unrar_ppmd_suballoc_h_block_split(self, units, i, index);
			return dmc_unrar_ppmd_pointer_to_offset(self, units);
		}
	}

	self->glue_count--;

	i = dmc_unrar_ppmd_suballoc_h_i2b(self, index);
	if (self->units_start-self->text > i) {
		self->units_start -= i;

		return dmc_unrar_ppmd_pointer_to_offset(self, self->units_start);
	}

	return 0;
}

static uint32_t dmc_unrar_ppmd_suballoc_h_expand_units(dmc_unrar_ppmd_suballocator_h *self,
		uint32_t old_offs, int old_num) {

	void *old_ptr = dmc_unrar_ppmd_offset_to_pointer(self, old_offs);
	int old_index = self->units2index[old_num - 1];
	int new_index = self->units2index[old_num];
	uint32_t offs;

	if (old_index == new_index)
		return old_offs;

	offs = dmc_unrar_ppmd_suballoc_h_alloc_units(self, old_num + 1);
	if (offs) {
		memcpy(dmc_unrar_ppmd_offset_to_pointer(self, offs), old_ptr, old_num * DMC_UNRAR_PPMD_UNIT_SIZE);
		dmc_unrar_ppmd_suballoc_h_node_insert(self, old_ptr, old_index);
	}

	return offs;
}

static uint32_t dmc_unrar_ppmd_suballoc_h_shrink_units(dmc_unrar_ppmd_suballocator_h *self,
		uint32_t old_offs, int old_num, int new_num) {

	void *old_ptr = dmc_unrar_ppmd_offset_to_pointer(self, old_offs);
	int old_index = self->units2index[old_num - 1];
	int new_index = self->units2index[new_num - 1];

	if (old_index == new_index)
		return old_offs;

	if (self->free_list[new_index].next) {
		void *ptr = dmc_unrar_ppmd_suballoc_h_node_remove(self, new_index);

		memcpy(ptr, old_ptr, new_num * DMC_UNRAR_PPMD_UNIT_SIZE);
		dmc_unrar_ppmd_suballoc_h_node_insert(self, old_ptr, old_index);

		return dmc_unrar_ppmd_pointer_to_offset(self, ptr);
	}

	dmc_unrar_ppmd_suballoc_h_block_split(self, old_ptr, old_index, new_index);
	return old_offs;
}

static void dmc_unrar_ppmd_suballoc_h_free_units(dmc_unrar_ppmd_suballocator_h *self,
		uint32_t offs, int num) {

	dmc_unrar_ppmd_suballoc_h_node_insert(self, dmc_unrar_ppmd_offset_to_pointer(self, offs),
	                                      self->units2index[num - 1]);
}

static void dmc_unrar_ppmd_suballoc_h_glue_free_blocks(dmc_unrar_ppmd_suballocator_h *self) {
	dmc_unrar_ppmd_memory_block_h *p = NULL;
	int i, k, sz;

	if (self->low_unit != self->high_unit)
		*self->low_unit = 0;

	self->sentinel.next = self->sentinel.prev = dmc_unrar_ppmd_pointer_to_offset(self, &self->sentinel);
	for (i = 0; i < DMC_UNRAR_PPMD_N_INDICES; i++) {
		while (self->free_list[i].next) {
			p = (dmc_unrar_ppmd_memory_block_h *) dmc_unrar_ppmd_suballoc_h_node_remove(self, i);

			dmc_unrar_ppmd_suballoc_h_block_insert_after(self, p, &self->sentinel);
			p->stamp = 0xFFFF;
			p->nu = self->index2units[i];
		}
	}

	for (p = (dmc_unrar_ppmd_memory_block_h *)dmc_unrar_ppmd_offset_to_pointer(self, self->sentinel.next);
	     p != &self->sentinel;
	     p = (dmc_unrar_ppmd_memory_block_h *)dmc_unrar_ppmd_offset_to_pointer(self, p->next)) {

		for (;;) {
			dmc_unrar_ppmd_memory_block_h *p1 = p + p->nu;

			if (p1->stamp != 0xFFFF)
				break;
			if ((p->nu + p1->nu) >= 0x10000)
				break;

			dmc_unrar_ppmd_suballoc_h_block_remove(self, p1);
			p->nu += p1->nu;
		}
	}

	for (;;) {
		p = (dmc_unrar_ppmd_memory_block_h *)dmc_unrar_ppmd_offset_to_pointer(self, self->sentinel.next);

		if (p == &self->sentinel)
			break;

		dmc_unrar_ppmd_suballoc_h_block_remove(self, p);

		sz = p->nu;
		while (sz > 128) {
			dmc_unrar_ppmd_suballoc_h_node_insert(self, p, DMC_UNRAR_PPMD_N_INDICES - 1);
			sz -= 128;
			p  += 128;
		}

		i = self->units2index[sz-1];
		if (self->index2units[i]!=sz) {
			i--;

			k = sz-self->index2units[i];
			dmc_unrar_ppmd_suballoc_h_node_insert(self, p + (sz - k), k - 1);
		}

		dmc_unrar_ppmd_suballoc_h_node_insert(self, p, i);
	}
}

static void dmc_unrar_ppmd_suballoc_h_node_insert(dmc_unrar_ppmd_suballocator_h *self, void *p, int index) {
	((dmc_unrar_ppmd_allocator_node_h *)p)->next = self->free_list[index].next;
	self->free_list[index].next = (dmc_unrar_ppmd_allocator_node_h*)p;
}

static void *dmc_unrar_ppmd_suballoc_h_node_remove(dmc_unrar_ppmd_suballocator_h *self, int index) {
	dmc_unrar_ppmd_allocator_node_h *node = self->free_list[index].next;
	self->free_list[index].next = (dmc_unrar_ppmd_allocator_node_h *)node->next;

	return node;
}

static unsigned int dmc_unrar_ppmd_suballoc_h_i2b(dmc_unrar_ppmd_suballocator_h *self, int index) {
	return DMC_UNRAR_PPMD_UNIT_SIZE * self->index2units[index];
}

static void dmc_unrar_ppmd_suballoc_h_block_split(dmc_unrar_ppmd_suballocator_h *self, void *pv,
		int old_index, int new_index) {

	uint8_t *p = ((uint8_t *)pv) + dmc_unrar_ppmd_suballoc_h_i2b(self, new_index);

	int diff = self->index2units[old_index] - self->index2units[new_index];
	int i = self->units2index[diff - 1];

	if (self->index2units[i] != diff) {
		dmc_unrar_ppmd_suballoc_h_node_insert(self, p, i - 1);

		p += dmc_unrar_ppmd_suballoc_h_i2b(self, i - 1);
		diff -= self->index2units[i - 1];
	}

	dmc_unrar_ppmd_suballoc_h_node_insert(self, p, self->units2index[diff - 1]);
}

static void dmc_unrar_ppmd_range_coder_init(dmc_unrar_ppmd_range_coder *self,
		dmc_unrar_ppmd_read_func *read_func, void *input_context, bool use_low, int bottom) {

	int i;

	self->read_func     = read_func;
	self->input_context = input_context;

	self->low   = 0;
	self->code  = 0;
	self->range = 0xFFFFFFFF;

	self->use_low = use_low;
	self->bottom = bottom;

	for (i = 0; i < 4; i++)
		self->code = (self->code << 8) | read_func(input_context);
}

static uint32_t dmc_unrar_ppmd_range_coder_current_count(dmc_unrar_ppmd_range_coder *self, uint32_t scale) {
	self->range /= scale;
	return (self->code-self->low) / self->range;
}

static void dmc_unrar_ppmd_range_coder_remove_sub_range(dmc_unrar_ppmd_range_coder *self,
		uint32_t low_count, uint32_t high_count) {

	if (self->use_low)
		self->low  += self->range * low_count;
	else
		self->code -= self->range * low_count;

	self->range *= high_count-low_count;

	dmc_unrar_ppmd_range_coder_normalize(self);
}

static int dmc_unrar_ppmd_range_coder_next_weighted_bit(dmc_unrar_ppmd_range_coder *self,
		int weight, int size) {

	uint32_t val = dmc_unrar_ppmd_range_coder_current_count(self, size);

	if ((int64_t)val < weight) {
		dmc_unrar_ppmd_range_coder_remove_sub_range(self, 0, weight);
		return 0;
	}

	dmc_unrar_ppmd_range_coder_remove_sub_range(self, weight, size);
	return 1;
}

static int dmc_unrar_ppmd_range_coder_next_weighted_bit2(dmc_unrar_ppmd_range_coder *self,
	int weight, int shift) {

	uint32_t threshold = (self->range >> shift) * weight;

	int bit;
	if (self->code < threshold) { /* <= ?. */
		bit = 0;
		self->range  = threshold;
	} else {
		bit = 1;
		self->range -= threshold;
		self->code  -= threshold;
	}

	dmc_unrar_ppmd_range_coder_normalize(self);

	return bit;
}

static void dmc_unrar_ppmd_range_coder_normalize(dmc_unrar_ppmd_range_coder *self) {
	int byte_c;

	for (;;) {
		if ((self->low ^ (self->low + self->range)) >= 0x1000000) {
			if (self->range >= self->bottom)
				break;

			self->range = -self->low & (self->bottom - 1);
		}

		byte_c = self->read_func(self->input_context);

		self->code = (self->code << 8) | byte_c;
		self->range <<= 8;
		self->low <<= 8;
	}
}

static dmc_unrar_ppmd_see_context dmc_unrar_ppmd_see_create(int init_val, int count) {
	dmc_unrar_ppmd_see_context self;

	self.shift = DMC_UNRAR_PPMD_PERIOD_BITS - 4;
	self.summ  = init_val << self.shift;
	self.count = count;

	return self;
}

static unsigned int dmc_unrar_ppmd_see_get_mean(dmc_unrar_ppmd_see_context *self) {
	unsigned int retval = self->summ >> self->shift;

	self->summ -= retval;

	if (retval == 0)
		return 1;

	return retval;
}

static void dmc_unrar_ppmd_see_update(dmc_unrar_ppmd_see_context *self) {
	if (self->shift >= DMC_UNRAR_PPMD_PERIOD_BITS)
		return;

	self->count--;
	if (self->count == 0) {
		self->summ *= 2;
		self->count = 3 << self->shift;
		self->shift++;
	}
}

static dmc_unrar_ppmd_context *dmc_unrar_ppmd_state_successor(dmc_unrar_ppmd_state *self,
	dmc_unrar_ppmd_core_model *model) {

	return (dmc_unrar_ppmd_context *)dmc_unrar_ppmd_offset_to_pointer(model->alloc, self->successor);
}

static void dmc_unrar_ppmd_state_set_successor_pointer(dmc_unrar_ppmd_state *self,
		dmc_unrar_ppmd_context *new_successor, dmc_unrar_ppmd_core_model *model) {

	self->successor = dmc_unrar_ppmd_pointer_to_offset(model->alloc, new_successor);
}

static dmc_unrar_ppmd_state *dmc_unrar_ppmd_context_states(dmc_unrar_ppmd_context *self,
		dmc_unrar_ppmd_core_model *model) {

	return (dmc_unrar_ppmd_state *)dmc_unrar_ppmd_offset_to_pointer(model->alloc, self->states);
}

static void dmc_unrar_ppmd_context_set_states_pointer(dmc_unrar_ppmd_context *self,
		dmc_unrar_ppmd_state *new_states, dmc_unrar_ppmd_core_model *model) {

	self->states = dmc_unrar_ppmd_pointer_to_offset(model->alloc, new_states);
}

static dmc_unrar_ppmd_context *dmc_unrar_ppmd_context_suffix(dmc_unrar_ppmd_context *self,
		dmc_unrar_ppmd_core_model *model) {

	return (dmc_unrar_ppmd_context *)dmc_unrar_ppmd_offset_to_pointer(model->alloc, self->suffix);
}

static void dmc_unrar_ppmd_context_set_suffix_pointer(dmc_unrar_ppmd_context *self,
		dmc_unrar_ppmd_context *new_suffix, dmc_unrar_ppmd_core_model *model) {

	self->suffix = dmc_unrar_ppmd_pointer_to_offset(model->alloc, new_suffix);
}

static dmc_unrar_ppmd_state *dmc_unrar_ppmd_context_one_state(dmc_unrar_ppmd_context *self) {
	return (dmc_unrar_ppmd_state *)&self->summ_freq;
}

static dmc_unrar_ppmd_context *dmc_unrar_ppmd_new_context(dmc_unrar_ppmd_core_model *model) {
	dmc_unrar_ppmd_context *context = (dmc_unrar_ppmd_context *)
		dmc_unrar_ppmd_offset_to_pointer(model->alloc, dmc_unrar_ppmd_alloc_context(model->alloc));

	if (context) {
		context->last_state_index = 0;
		context->flags            = 0;
		context->suffix           = 0;
	}

	return context;
}

static dmc_unrar_ppmd_context *dmc_unrar_ppmd_new_context_as_child_of(dmc_unrar_ppmd_core_model *model,
		dmc_unrar_ppmd_context *suffix_context, dmc_unrar_ppmd_state *suffix_state,
		dmc_unrar_ppmd_state *first_state) {

	dmc_unrar_ppmd_context *context = (dmc_unrar_ppmd_context *)
		dmc_unrar_ppmd_offset_to_pointer(model->alloc, dmc_unrar_ppmd_alloc_context(model->alloc));

	if (context) {
		context->last_state_index = 0;
		context->flags            = 0;

		dmc_unrar_ppmd_context_set_suffix_pointer(context, suffix_context, model);
		dmc_unrar_ppmd_state_set_successor_pointer(suffix_state, context, model);

		if (first_state)
			*(dmc_unrar_ppmd_context_one_state(context)) = *first_state;
	}

	return context;
}

static void dmc_unrar_ppmd_decode_bin_symbol(dmc_unrar_ppmd_context *self,
		dmc_unrar_ppmd_core_model *model, uint16_t *bs, int freq_limit, bool alt_next_bit) {

	/* Tabulated escapes for exponential symbol distribution. */
	static const uint8_t exp_escape[16] = { 25, 14, 9, 7, 5, 5, 4, 4, 4, 3, 3, 3, 2, 2, 2, 2 };

	dmc_unrar_ppmd_state *rs = dmc_unrar_ppmd_context_one_state(self);

	int bit;
	if (alt_next_bit)
		bit = dmc_unrar_ppmd_range_coder_next_weighted_bit2(&model->coder, *bs, DMC_UNRAR_PPMD_TOTAL_BITS);
	else
		bit = dmc_unrar_ppmd_range_coder_next_weighted_bit(&model->coder, *bs, 1 << DMC_UNRAR_PPMD_TOTAL_BITS);

	if (bit == 0) {
		model->prev_success = 1;
		model->run_length++;
		model->found_state = rs;

		if (rs->freq < freq_limit)
			rs->freq++;

		*bs += DMC_UNRAR_PPMD_INTERVAL - DMC_UNRAR_PPMD_GET_MEAN(*bs, DMC_UNRAR_PPMD_PERIOD_BITS, 2);

	} else {
		model->prev_success    = 0;
		model->found_state     = NULL;
		model->last_mask_index = 0;

		model->char_mask[rs->symbol] = model->esc_count;

		*bs -= DMC_UNRAR_PPMD_GET_MEAN(*bs, DMC_UNRAR_PPMD_PERIOD_BITS, 2);
		model->init_esc = exp_escape[*bs >> 10];
	}
}

static int dmc_unrar_ppmd_decode_symbol1(dmc_unrar_ppmd_context *self,
		dmc_unrar_ppmd_core_model *model, bool greater_or_equal) {

	dmc_unrar_ppmd_state *states;
	int i, first_count, high_count, count, adder, last_symbol;

	model->scale = self->summ_freq;

	states = dmc_unrar_ppmd_context_states(self, model);
	first_count = states[0].freq;
	count = dmc_unrar_ppmd_range_coder_current_count(&model->coder, model->scale);
	adder = greater_or_equal ? 1 : 0;

	if (count < first_count) {
		dmc_unrar_ppmd_range_coder_remove_sub_range(&model->coder, 0, first_count);

		if ((2 * first_count + adder) > (int64_t)model->scale) {
			model->prev_success = 1;
			model->run_length++;
		}
		else
			model->prev_success = 0;

		model->found_state = &states[0];
		states[0].freq = first_count + 4;
		self->summ_freq += 4;

		if ((first_count + 4) > (int)DMC_UNRAR_PPMD_MAX_FREQ)
			model->rescale_ppmd_context(self, model);

		return -1;
	}

	high_count = first_count;
	model->prev_success = 0;

	for (i = 1; i <= self->last_state_index; i++) {
		high_count += states[i].freq;

		if (high_count > count) {
			dmc_unrar_ppmd_range_coder_remove_sub_range(&model->coder, high_count - states[i].freq, high_count);
			dmc_unrar_ppmd_context_update1(self, model, &states[i]);
			return -1;
		}
	}

	if (!model->found_state)
		return -1;

	last_symbol = model->found_state->symbol;

	dmc_unrar_ppmd_range_coder_remove_sub_range(&model->coder, high_count, model->scale);
	model->last_mask_index = self->last_state_index;
	model->found_state = NULL;

	for (i = 0; i <= self->last_state_index; i++)
		model->char_mask[states[i].symbol] = model->esc_count;

	return last_symbol;
}

static void dmc_unrar_ppmd_context_update1(dmc_unrar_ppmd_context *self,
		dmc_unrar_ppmd_core_model *model, dmc_unrar_ppmd_state *state) {

	state->freq     += 4;
	self->summ_freq += 4;

	if (state[0].freq > state[-1].freq) {
		dmc_unrar_ppmd_state tmp = state[0];
		state[0] = state[-1];
		state[-1] = tmp;

		model->found_state = &state[-1];
		if (state[-1].freq > DMC_UNRAR_PPMD_MAX_FREQ)
			model->rescale_ppmd_context(self, model);

	} else
		model->found_state = state;
}

static void dmc_unrar_ppmd_decode_symbol2(dmc_unrar_ppmd_context *self,
		dmc_unrar_ppmd_core_model *model, dmc_unrar_ppmd_see_context *see) {

	int n = self->last_state_index-model->last_mask_index, total = 0, i, count;
	dmc_unrar_ppmd_state *ps[256];

	dmc_unrar_ppmd_state *state = dmc_unrar_ppmd_context_states(self, model);
	for (i = 0; i < n; i++) {
		while (model->char_mask[state->symbol] == model->esc_count)
			state++;

		total += state->freq;
		ps[i] = state++;
	}

	model->scale += total;
	count = dmc_unrar_ppmd_range_coder_current_count(&model->coder, model->scale);

	if (count < total) {
		int high_count = ps[0]->freq;

		i = 0;
		while (high_count <= count && (i + 1) < n)
			high_count += ps[++i]->freq;

		dmc_unrar_ppmd_range_coder_remove_sub_range(&model->coder, high_count - ps[i]->freq, high_count);
		dmc_unrar_ppmd_see_update(see);
		dmc_unrar_ppmd_context_update2(self, model, ps[i]);

	} else {
		dmc_unrar_ppmd_range_coder_remove_sub_range(&model->coder, total, model->scale);
		model->last_mask_index = self->last_state_index;
		see->summ += model->scale;

		for (i = 0; i < n; i++)
			model->char_mask[ps[i]->symbol] = model->esc_count;
	}
}

static void dmc_unrar_ppmd_context_update2(dmc_unrar_ppmd_context *self,
		dmc_unrar_ppmd_core_model *model, dmc_unrar_ppmd_state *state) {

	model->found_state = state;
	state->freq       += 4;
	self->summ_freq   += 4;

	if (state->freq > DMC_UNRAR_PPMD_MAX_FREQ)
		model->rescale_ppmd_context(self, model);

	model->esc_count++;
	model->run_length = model->init_rl;
}

static void dmc_unrar_ppmd_rescale_context(dmc_unrar_ppmd_context *self, dmc_unrar_ppmd_core_model *model) {
	dmc_unrar_ppmd_state *states = dmc_unrar_ppmd_context_states(self, model);
	int i, esc_freq, adder, n = self->last_state_index + 1;

	/* Bump frequency of found state. */
	model->found_state->freq += 4;

	/* Divide all frequencies and sort list. */
	esc_freq = self->summ_freq + 4;
	adder = ((model->order_fall == 0) ? 0 : 1);
	self->summ_freq = 0;

	for (i = 0; i < n; i++) {
		esc_freq -= states[i].freq;
		states[i].freq = (states[i].freq + adder) >> 1;
		self->summ_freq += states[i].freq;

		/* Keep states sorted by decreasing frequency. */
		if ((i > 0) && (states[i].freq > states[i - 1].freq)) {
			/* If not sorted, move current state upwards until list is sorted. */
			dmc_unrar_ppmd_state tmp = states[i];

			int j = i - 1;
			while ((j > 0) && (tmp.freq > states[j - 1].freq))
				j--;

			memmove(&states[j + 1], &states[j], sizeof(dmc_unrar_ppmd_state) * (i -  j));
			states[j] = tmp;
		}
	}

	/* TODO: add better sorting stage here.. */

	/* Drop states whose frequency has fallen to 0. */
	if (states[n-1].freq == 0) {
		int num_zeros = 1;
		while ((num_zeros < n) && (states[n - 1 - num_zeros].freq == 0))
			num_zeros++;

		esc_freq += num_zeros;

		self->last_state_index -= num_zeros;
		if (self->last_state_index == 0) {
			dmc_unrar_ppmd_state tmp = states[0];

			do {
				tmp.freq = (tmp.freq + 1) >> 1;
				esc_freq >>= 1;
			} while (esc_freq>1);

			dmc_unrar_ppmd_free_units(model->alloc, self->states, (n + 1) >> 1);
			model->found_state = dmc_unrar_ppmd_context_one_state(self);
			*model->found_state = tmp;

			return;
		}

		{
			int n0 = (n + 1) >> 1, n1 = (self->last_state_index + 2) >> 1;

			if (n0 != n1)
				self->states = dmc_unrar_ppmd_shrink_units(model->alloc, self->states, n0, n1);
		}
	}

	self->summ_freq += (esc_freq + 1) >> 1;

	/* The found state is the first one to breach the limit, thus it is the largest and also first. */
	model->found_state = dmc_unrar_ppmd_context_states(self, model);
}

static void dmc_unrar_ppmd_clear_model_mask(dmc_unrar_ppmd_core_model *self) {
	self->esc_count = 1;
	memset(self->char_mask, 0, sizeof(self->char_mask));
}

static void dmc_unrar_ppmd_start_model_h(dmc_unrar_ppmd_model_h *self,
		dmc_unrar_ppmd_read_func *read_func, void *input_context, dmc_unrar_ppmd_suballocator_h *alloc,
		int max_order, bool seven_zip) {

	int i, m = 3, k = 1, step = 1;

	dmc_unrar_ppmd_range_coder_restart(self, read_func, input_context, seven_zip);

	self->alloc = alloc;
	self->core.alloc = &alloc->core;

	self->core.rescale_ppmd_context = dmc_unrar_ppmd_rescale_context;

	self->max_order = max_order;
	self->seven_zip = seven_zip;
	self->core.esc_count = 1;

	self->ns2bs_index[0] = 2 * 0;
	self->ns2bs_index[1] = 2 * 1;

	for (i = 2; i < 11; i++)
		self->ns2bs_index[i] = 2 * 2;

	for (i = 11; i < 256; i++)
		self->ns2bs_index[i] = 2 * 3;

	for (i = 0; i < 3; i++)
		self->ns2index[i] = i;

	for (i = 3; i < 256; i++) {
		self->ns2index[i] = m;

		if (!--k) {
			m++;
			step++;
			k = step;
		}
	}

	memset(self->hb2flag, 0, 0x40);
	memset(self->hb2flag + 0x40, 0x08, 0x100 - 0x40);

	self->dummy_see_context.shift = DMC_UNRAR_PPMD_PERIOD_BITS;

	dmc_unrar_ppmd_model_restart(self);
}

static void dmc_unrar_ppmd_range_coder_restart(dmc_unrar_ppmd_model_h *self,
		dmc_unrar_ppmd_read_func *read_func, void *input_context, bool seven_zip) {

	if (seven_zip) {
		/* Skip one byte. */
		read_func(input_context);

		dmc_unrar_ppmd_range_coder_init(&self->core.coder, read_func, input_context, false, 0);

		return;
	}

	dmc_unrar_ppmd_range_coder_init(&self->core.coder, read_func, input_context, true, 0x8000);
}

static void dmc_unrar_ppmd_model_restart(dmc_unrar_ppmd_model_h *self) {
	static const uint16_t init_bin_esc[8] = {0x3cdd, 0x1f3f, 0x59bf, 0x48f3, 0x64a1, 0x5abc, 0x6632, 0x6051};
	int i, k, m;

	dmc_unrar_ppmd_init_suballocator(self->core.alloc);

	memset(self->core.char_mask, 0, sizeof(self->core.char_mask));

	self->core.prev_success = 0;
	self->core.order_fall = self->max_order;
	self->core.run_length = self->core.init_rl = -((self->max_order < 12) ? self->max_order : 12) - 1;

	self->max_context = self->min_context = dmc_unrar_ppmd_new_context(&self->core);
	self->max_context->last_state_index = 255;
	self->max_context->summ_freq = 257;
	self->max_context->states = dmc_unrar_ppmd_alloc_units(self->core.alloc, 256 / 2);

	{
		dmc_unrar_ppmd_state *max_states = dmc_unrar_ppmd_context_states(self->max_context, &self->core);

		for (i = 0; i < 256; i++) {
			max_states[i].symbol = i;
			max_states[i].freq = 1;
			max_states[i].successor = 0;
		}
	}

	self->core.found_state = dmc_unrar_ppmd_context_states(self->max_context, &self->core);

	for (i = 0; i < 128; i++)
		for (k = 0; k < 8; k++)
			for (m = 0; m < 64; m += 8)
				self->bin_summ[i][k + m] = DMC_UNRAR_PPMD_BIN_SCALE-init_bin_esc[k] / (i + 2);

	for (i = 0; i < 25; i++)
		for (k = 0; k < 16; k++)
			self->see_context[i][k]=dmc_unrar_ppmd_see_create(5 * i + 10, 4);
}

static int dmc_unrar_ppmd_next_byte(dmc_unrar_ppmd_model_h *self) {
	uint8_t byte_c;

	if (!self->min_context)
		return -1;

	if (self->min_context->last_state_index != 0)
		dmc_unrar_ppmd_decode_symbol1_h(self->min_context, self);
	else
		dmc_unrar_ppmd_decode_bin_symbol_h(self->min_context, self);

	while (!self->core.found_state) {
		do {
			self->core.order_fall++;
			self->min_context = dmc_unrar_ppmd_context_suffix(self->min_context, &self->core);

			if (!self->min_context)
				return -1;

		} while (self->min_context->last_state_index == self->core.last_mask_index);

		dmc_unrar_ppmd_decode_symbol2_h(self->min_context, self);
	}

	byte_c = self->core.found_state->symbol;

	if ((self->core.order_fall == 0) &&
	    ((uint8_t *)dmc_unrar_ppmd_state_successor(self->core.found_state, &self->core) > self->alloc->text)) {

		self->min_context = self->max_context = dmc_unrar_ppmd_state_successor(self->core.found_state, &self->core);
	} else {
		dmc_unrar_ppmd_model_update(self);
		if (self->core.esc_count == 0)
			dmc_unrar_ppmd_clear_model_mask(&self->core);
	}

	return byte_c;
}

static void dmc_unrar_ppmd_model_update(dmc_unrar_ppmd_model_h *self) {
	dmc_unrar_ppmd_state fs = *self->core.found_state;
	dmc_unrar_ppmd_state *state = NULL;

	dmc_unrar_ppmd_context *successor = NULL;
	dmc_unrar_ppmd_context *curr_context = NULL;

	int min_num = 0;
	int s0 = 0;

	if ((fs.freq < (DMC_UNRAR_PPMD_MAX_FREQ / 4)) && self->min_context->suffix) {
		dmc_unrar_ppmd_context *context = dmc_unrar_ppmd_context_suffix(self->min_context, &self->core);

		if (context->last_state_index != 0) {
			state = dmc_unrar_ppmd_context_states(context, &self->core);

			if (state->symbol != fs.symbol) {
				do state++;
				while (state->symbol != fs.symbol);

				if (state[0].freq >= state[-1].freq) {
					dmc_unrar_ppmd_state tmp = state[0];
					state[0] = state[-1];
					state[-1] = tmp;

					state--;
				}
			}

			if (state->freq < (DMC_UNRAR_PPMD_MAX_FREQ - 9)) {
				state->freq += 2;
				context->summ_freq += 2;
			}
		} else {
			state = dmc_unrar_ppmd_context_one_state(context);
			if (state->freq < 32)
				state->freq++;
		}
	}

	if (self->core.order_fall == 0) {
		self->min_context = self->max_context = dmc_unrar_ppmd_create_successors(self, true, state);
		dmc_unrar_ppmd_state_set_successor_pointer(self->core.found_state, self->min_context, &self->core);
		if (!self->min_context) {
			dmc_unrar_ppmd_model_restart(self);
			self->core.esc_count = 0;
		}

		return;
	}

	*self->alloc->text++ = fs.symbol;
	successor = (dmc_unrar_ppmd_context *)self->alloc->text;

	if (self->alloc->text >= self->alloc->units_start) {
		dmc_unrar_ppmd_model_restart(self);
		self->core.esc_count = 0;
		return;
	}

	if (fs.successor) {
		if ((uint8_t *)dmc_unrar_ppmd_state_successor(&fs, &self->core) <= self->alloc->text) {
			dmc_unrar_ppmd_state_set_successor_pointer(&fs, dmc_unrar_ppmd_create_successors(self, false, state), &self->core);
			if (!fs.successor) {
				dmc_unrar_ppmd_model_restart(self);
				self->core.esc_count = 0;
				return;
			}
		}

		if (--self->core.order_fall == 0) {
			successor = dmc_unrar_ppmd_state_successor(&fs, &self->core);
			if (self->max_context != self->min_context)
				self->alloc->text--;
		}
	} else {
		dmc_unrar_ppmd_state_set_successor_pointer(self->core.found_state, successor, &self->core);
		dmc_unrar_ppmd_state_set_successor_pointer(&fs, self->min_context, &self->core);
	}

	min_num = self->min_context->last_state_index + 1;
	s0 = self->min_context->summ_freq - min_num - (fs.freq - 1);

	for (curr_context = self->max_context; curr_context != self->min_context;
	     curr_context = dmc_unrar_ppmd_context_suffix(curr_context, &self->core)) {

		unsigned int cf, sf, freq;
		int curr_num = curr_context->last_state_index + 1;

		if (curr_num != 1) {
			if ((curr_num & 1) == 0) {
				curr_context->states =
					dmc_unrar_ppmd_expand_units(self->core.alloc, curr_context->states, curr_num >> 1);

				if (!curr_context->states) {
					dmc_unrar_ppmd_model_restart(self);
					self->core.esc_count = 0;
					return;
				}
			}

			if (((4 * curr_num) <= min_num) && (curr_context->summ_freq <= (8 * curr_num)))
				curr_context->summ_freq += 2;

			if ((2 * curr_num) < min_num)
				curr_context->summ_freq++;

		} else {
			dmc_unrar_ppmd_state *states = (dmc_unrar_ppmd_state *)
				dmc_unrar_ppmd_offset_to_pointer(self->core.alloc, dmc_unrar_ppmd_alloc_units(self->core.alloc, 1));

			if (!states) {
				dmc_unrar_ppmd_model_restart(self);
				self->core.esc_count = 0;
				return;
			}

			states[0] = *(dmc_unrar_ppmd_context_one_state(curr_context));
			dmc_unrar_ppmd_context_set_states_pointer(curr_context, states, &self->core);

			if (states[0].freq < ((DMC_UNRAR_PPMD_MAX_FREQ / 4) - 1))
				states[0].freq *= 2;
			else
				states[0].freq = DMC_UNRAR_PPMD_MAX_FREQ - 4;

			curr_context->summ_freq = states[0].freq + self->core.init_esc + ((min_num > 3) ? 1 : 0);
		}

		cf = 2 * fs.freq * (curr_context->summ_freq + 6);
		sf = s0 + curr_context->summ_freq;

		if (cf < (6 * sf)) {
			if      (cf >= (4 * sf))
				freq = 3;
			else if (cf > sf)
				freq = 2;
			else
				freq = 1;

			curr_context->summ_freq += 3;

		} else {
			if      (cf >= (15 * sf))
				freq = 7;
			else if (cf >= (12 * sf))
				freq = 6;
			else if (cf >= ( 9 * sf))
				freq = 5;
			else
				freq = 4;

			curr_context->summ_freq += freq;
		}

		{
			dmc_unrar_ppmd_state *curr_states = dmc_unrar_ppmd_context_states(curr_context, &self->core);
			dmc_unrar_ppmd_state *new_c = &curr_states[curr_num];

			dmc_unrar_ppmd_state_set_successor_pointer(new_c, successor, &self->core);
			new_c->symbol = fs.symbol;
			new_c->freq = freq;
			curr_context->last_state_index = curr_num;
		}
	}

	self->max_context = self->min_context = dmc_unrar_ppmd_state_successor(&fs, &self->core);
}

static dmc_unrar_ppmd_context *dmc_unrar_ppmd_create_successors(dmc_unrar_ppmd_model_h *self,
		bool skip, dmc_unrar_ppmd_state *state) {

	dmc_unrar_ppmd_context *context  = self->min_context;
	dmc_unrar_ppmd_context *upbranch = dmc_unrar_ppmd_state_successor(self->core.found_state, &self->core);
	dmc_unrar_ppmd_state *statelist[DMC_UNRAR_PPMD_MAX_O];
	int i, n = 0;

	if (!skip) {
		statelist[n++] = self->core.found_state;

		if (!context->suffix)
			goto skip;
	}

	if (state) {
		context = dmc_unrar_ppmd_context_suffix(context, &self->core);
		if (dmc_unrar_ppmd_state_successor(state, &self->core) != upbranch) {
			context = dmc_unrar_ppmd_state_successor(state, &self->core);
			goto skip;
		}

		statelist[n++] = state;
		if (!context->suffix)
			goto skip;
	}

	do {
		context = dmc_unrar_ppmd_context_suffix(context, &self->core);

		if (context->last_state_index != 0) {
			state = dmc_unrar_ppmd_context_states(context, &self->core);
			while (state->symbol != self->core.found_state->symbol)
				state++;

		} else
			state = dmc_unrar_ppmd_context_one_state(context);

		if (dmc_unrar_ppmd_state_successor(state, &self->core) != upbranch) {
			context = dmc_unrar_ppmd_state_successor(state, &self->core);
			break;
		}

		statelist[n++] = state;

	} while (context->suffix);

	skip:

	if (n == 0)
		return context;

	{
		dmc_unrar_ppmd_state upstate;

		upstate.symbol = *(uint8_t *)upbranch;
		dmc_unrar_ppmd_state_set_successor_pointer(&upstate,
				(dmc_unrar_ppmd_context *)(((uint8_t *)upbranch) + 1), &self->core);

		if (context->last_state_index != 0) {
			int cf, s0;

			state = dmc_unrar_ppmd_context_states(context, &self->core);
			while (state->symbol != upstate.symbol)
				state++;

			cf = state->freq - 1;
			s0 = context->summ_freq - context->last_state_index - 1 - cf;

			if (2 * cf <= s0) {
				if (5 * cf > s0)
					upstate.freq = 2;
				else
					upstate.freq = 1;

			} else
				upstate.freq = 1 + ((2 * cf + 3 * s0 - 1) / (2 * s0));

		} else
			upstate.freq = dmc_unrar_ppmd_context_one_state(context)->freq;

		for (i = n - 1; i >= 0; i--) {
			context = dmc_unrar_ppmd_new_context_as_child_of(&self->core, context, statelist[i], &upstate);

			if (!context)
				return NULL;
		}
	}

	return context;
}

static void dmc_unrar_ppmd_decode_bin_symbol_h(dmc_unrar_ppmd_context *self, dmc_unrar_ppmd_model_h *model)
{
	dmc_unrar_ppmd_state *rs = dmc_unrar_ppmd_context_one_state(self);
	uint16_t *bs;

	model->hi_bits_flag = model->hb2flag[model->core.found_state->symbol];

	bs = &model->bin_summ[rs->freq-1][model->core.prev_success +
		model->ns2bs_index[dmc_unrar_ppmd_context_suffix(self, &model->core)->last_state_index] +
		model->hi_bits_flag +
		2 * model->hb2flag[rs->symbol] +
		((model->core.run_length >> 26) & 0x20)];

	dmc_unrar_ppmd_decode_bin_symbol(self, &model->core, bs, 128, model->seven_zip);
}

static void dmc_unrar_ppmd_decode_symbol1_h(dmc_unrar_ppmd_context *self, dmc_unrar_ppmd_model_h *model) {
	int lastsym = dmc_unrar_ppmd_decode_symbol1(self, &model->core, false);

	if (lastsym >= 0)
		model->hi_bits_flag = model->hb2flag[lastsym];
}

static void dmc_unrar_ppmd_decode_symbol2_h(dmc_unrar_ppmd_context *self, dmc_unrar_ppmd_model_h *model) {
	int diff = self->last_state_index - model->core.last_mask_index;
	dmc_unrar_ppmd_see_context *see;

	if (self->last_state_index != 255) {
		see = &model->see_context[model->ns2index[diff-1]][
			+ ((diff < (dmc_unrar_ppmd_context_suffix(self, &model->core)->last_state_index - self->last_state_index)) ? 1 : 0)
			+ ((self->summ_freq < (11 * (self->last_state_index + 1))) ? 2 : 0)
			+ (((model->core.last_mask_index + 1) > diff) ? 4 : 0)
			+ model->hi_bits_flag];

		model->core.scale = dmc_unrar_ppmd_see_get_mean(see);

	} else {
		model->core.scale = 1;
		see = &model->dummy_see_context;
	}

	dmc_unrar_ppmd_decode_symbol2(self, &model->core, see);
}

/* --- Implementation of my PPMd interface --- */

static dmc_unrar_return dmc_unrar_ppmd_create(dmc_unrar_alloc *alloc, dmc_unrar_ppmd *ppmd) {
	DMC_UNRAR_ASSERT(alloc && ppmd);

	DMC_UNRAR_CLEAR_OBJ(*ppmd);

	ppmd->alloc = alloc;

	return DMC_UNRAR_OK;
}

static void dmc_unrar_ppmd_destroy(dmc_unrar_ppmd *ppmd) {
	if (!ppmd)
		return;

	if (ppmd->alloc) {
		dmc_unrar_ppmd_suballocator_h_destroy(ppmd->alloc, ppmd->suballoc);
		dmc_unrar_free(ppmd->alloc, ppmd->model);
	}

	DMC_UNRAR_CLEAR_OBJ(*ppmd);
}

static int dmc_unrar_ppmd_read_from_bs(void *context) {
	dmc_unrar_bs *bs = (dmc_unrar_bs *) context;
	DMC_UNRAR_ASSERT(bs);

	return dmc_unrar_bs_read_bits(bs, 8);
}

static dmc_unrar_return dmc_unrar_ppmd_start(dmc_unrar_ppmd *ppmd, dmc_unrar_bs *bs,
	size_t heap_size_mb, size_t max_order) {

	DMC_UNRAR_ASSERT(ppmd && bs && ppmd->alloc);

	dmc_unrar_ppmd_suballocator_h_destroy(ppmd->alloc, ppmd->suballoc);
	dmc_unrar_free(ppmd->alloc, ppmd->model);

	ppmd->model    = NULL;
	ppmd->suballoc = NULL;

	ppmd->model = (dmc_unrar_ppmd_model_h *)
		dmc_unrar_malloc(ppmd->alloc, 1, sizeof(dmc_unrar_ppmd_model_h));
	if (!ppmd->model)
		return DMC_UNRAR_ALLOC_FAIL;

	DMC_UNRAR_CLEAR_OBJ(*ppmd->model);

	ppmd->suballoc = dmc_unrar_ppmd_suballocator_h_create(ppmd->alloc, heap_size_mb << 20);
	if (!ppmd->suballoc)
		return DMC_UNRAR_ALLOC_FAIL;

	dmc_unrar_ppmd_start_model_h(ppmd->model, (dmc_unrar_ppmd_read_func *)dmc_unrar_ppmd_read_from_bs,
	bs, ppmd->suballoc, max_order, false);

	return DMC_UNRAR_OK;
}

static dmc_unrar_return dmc_unrar_ppmd_restart(dmc_unrar_ppmd *ppmd, dmc_unrar_bs *bs) {
	DMC_UNRAR_ASSERT(ppmd && bs && ppmd->model && ppmd->suballoc);

	dmc_unrar_ppmd_range_coder_restart(ppmd->model,
		(dmc_unrar_ppmd_read_func *)dmc_unrar_ppmd_read_from_bs, bs, false);

	return DMC_UNRAR_OK;
}

static uint8_t dmc_unrar_ppmd_get_byte(dmc_unrar_ppmd *ppmd) {
	DMC_UNRAR_ASSERT(ppmd && ppmd->model && ppmd->suballoc);

	return dmc_unrar_ppmd_next_byte(ppmd->model);
}
#endif /* DMC_UNRAR_DISABLE_PPMD */
/* '--- */

/* .--- CRC-32, based on the implementation by Gary S. Brown ---.
 *
 * The original copyright note stated as follows:
 * You may use this program, or code or tables extracted from it,
 * as desired without restriction.
 *
 * See also <http://web.mit.edu/freebsd/head/sys/libkern/crc32.c>
 */

/** Table of CRC-32 polynomial feedback terms, 0xEDB88320 polynomial. */
static const uint32_t DMC_UNRAR_CRC32_TABLE[] = {
	0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F,
	0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
	0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2,
	0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
	0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9,
	0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
	0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
	0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
	0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423,
	0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
	0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106,
	0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
	0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D,
	0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
	0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950,
	0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
	0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7,
	0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
	0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA,
	0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
	0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81,
	0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
	0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84,
	0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
	0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB,
	0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
	0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E,
	0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
	0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55,
	0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
	0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28,
	0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
	0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F,
	0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
	0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242,
	0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
	0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69,
	0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
	0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC,
	0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
	0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693,
	0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
	0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

uint32_t dmc_unrar_crc32_continue_from_mem(uint32_t hash, const void *mem, size_t size) {
	const uint8_t *bytes = (const uint8_t *)mem;

	hash ^= 0xFFFFFFFF;

	while (size-- > 0)
		hash = DMC_UNRAR_CRC32_TABLE[(hash ^ *bytes++) & 0xFF] ^ (hash >> 8);

	hash ^= 0xFFFFFFFF;

	return hash;
}

uint32_t dmc_unrar_crc32_calculate_from_mem(const void *mem, size_t size) {
	return dmc_unrar_crc32_continue_from_mem(0, mem, size);
}

/* '--- */

/* .--- Filters implementation */
#if DMC_UNRAR_DISABLE_FILTERS != 1

#define DMC_UNRAR_FILTERS_EXTRA_MEMORY_SIZE 0x4000
#define DMC_UNRAR_FILTERS_TOTAL_MEMORY_SIZE (DMC_UNRAR_FILTERS_MEMORY_SIZE + DMC_UNRAR_FILTERS_EXTRA_MEMORY_SIZE)

typedef dmc_unrar_return (*dmc_unrar_filter_func)(uint8_t *memory, size_t memory_size,
	size_t file_position, size_t in_length, const uint32_t *registers,
	size_t *out_offset, size_t *out_length);

typedef struct dmc_unrar_filters_filter_tag {
	size_t usage_count;
	size_t last_in_length;

	dmc_unrar_filter_func func;

} dmc_unrar_filters_filter;

typedef struct dmc_unrar_filters_stack_entry_tag {
	size_t filter_index;

	size_t offset;
	size_t length;

	uint32_t registers[8];

} dmc_unrar_filters_stack_entry;

struct dmc_unrar_filters_interal_state_tag {
	uint8_t memory[DMC_UNRAR_FILTERS_TOTAL_MEMORY_SIZE];

	size_t last_filter;

	size_t filter_count;
	dmc_unrar_filters_filter *filters;
	size_t filter_capacity;

	size_t stack_count;
	dmc_unrar_filters_stack_entry *stack;
	size_t stack_capacity;
};

static uint32_t dmc_unrar_filters_rar4_read_number(dmc_unrar_bs *bs);
static uint32_t dmc_unrar_filters_rar5_read_number(dmc_unrar_bs *bs);

static bool dmc_unrar_filters_init_filters(dmc_unrar_filters *filters);
static bool dmc_unrar_filters_ensure_capacity_filters(dmc_unrar_filters *filters);
static bool dmc_unrar_filters_grow_filters(dmc_unrar_filters *filters);
static bool dmc_unrar_filters_init_stack(dmc_unrar_filters *filters);
static bool dmc_unrar_filters_ensure_capacity_stack(dmc_unrar_filters *filters);
static bool dmc_unrar_filters_grow_stack(dmc_unrar_filters *filters);
static bool dmc_unrar_filters_stack_pop(dmc_unrar_filters *filters);

static dmc_unrar_return dmc_unrar_filters_create(dmc_unrar_alloc *alloc,
		dmc_unrar_filters *filters) {

	DMC_UNRAR_ASSERT(alloc && filters);

	DMC_UNRAR_CLEAR_OBJ(*filters);

	filters->alloc = alloc;

	filters->internal_state = (dmc_unrar_filters_interal_state *)
		dmc_unrar_malloc(alloc, 1, sizeof(dmc_unrar_filters_interal_state));

	if (!filters->internal_state)
		return DMC_UNRAR_ALLOC_FAIL;

	DMC_UNRAR_CLEAR_OBJ(*filters->internal_state);

	if (!dmc_unrar_filters_init_filters(filters)) {
		dmc_unrar_filters_destroy(filters);
		return DMC_UNRAR_ALLOC_FAIL;
	}

	if (!dmc_unrar_filters_init_stack(filters)) {
		dmc_unrar_filters_destroy(filters);
		return DMC_UNRAR_ALLOC_FAIL;
	}

	return DMC_UNRAR_OK;
}

static void dmc_unrar_filters_destroy(dmc_unrar_filters *filters) {
	if (!filters)
		return;

	if (filters->alloc) {
		if (filters->internal_state) {
			dmc_unrar_free(filters->alloc, filters->internal_state->filters);
			dmc_unrar_free(filters->alloc, filters->internal_state->stack);
		}

		dmc_unrar_free(filters->alloc, filters->internal_state);
	}

	DMC_UNRAR_CLEAR_OBJ(*filters);
}

static bool dmc_unrar_filters_empty(dmc_unrar_filters *filters) {
	if (filters && filters->internal_state)
		return filters->internal_state->stack_count == 0;

	return true;
}

static size_t dmc_unrar_filters_get_first_offset(dmc_unrar_filters *filters) {
	if (filters && filters->internal_state)
		if (filters->internal_state->stack_count > 0)
			return filters->internal_state->stack[0].offset;

	return SIZE_MAX;
}

static size_t dmc_unrar_filters_get_first_length(dmc_unrar_filters *filters) {
	if (filters && filters->internal_state)
		if (filters->internal_state->stack_count > 0)
			return filters->internal_state->stack[0].length;

	return 0;
}

static void dmc_unrar_filters_clear(dmc_unrar_filters *filters);

static dmc_unrar_return dmc_unrar_filters_create_rar4_filter_from_bytecode(dmc_unrar_filters *filters,
	const uint8_t *bytecode, size_t bytecode_length);
static dmc_unrar_return dmc_unrar_filters_create_stack_entry(dmc_unrar_filters *filters,
	size_t filter_index, size_t filter_offset, size_t filter_length, uint32_t *registers);

static dmc_unrar_return dmc_unrar_filters_rar4_parse(dmc_unrar_filters *filters,
		const uint8_t *data, size_t data_size, uint8_t flags, size_t current_output_offset) {

	dmc_unrar_mem_reader mem_reader;
	dmc_unrar_io io;
	dmc_unrar_bs bs;

	bool new_filter = false;
	size_t index, filter_offset, filter_length;
	uint32_t usage_count, registers[8];

	uint8_t bytecode[DMC_UNRAR_FILTERS_BYTECODE_SIZE];

	DMC_UNRAR_ASSERT(filters && filters->internal_state);
	DMC_UNRAR_ASSERT(data && data_size);

	dmc_unrar_io_init_mem_reader(&io, &mem_reader, data, data_size);
	if (!dmc_unrar_bs_init_from_io(&bs, &io, data_size))
		return DMC_UNRAR_INVALID_DATA;

	index = filters->internal_state->last_filter;

	if (flags & 0x80) {
		index = dmc_unrar_filters_rar4_read_number(&bs);

		if (index == 0) {
			dmc_unrar_filters_clear(filters);

			index = 1;
		}

		if (--index == filters->internal_state->filter_count)
			new_filter = true;

		filters->internal_state->last_filter = index;
	}

	if ((index > 1024) || (index > filters->internal_state->filter_count ||
	    (!new_filter && (index == filters->internal_state->filter_count))))
		return DMC_UNRAR_FILTERS_INVALID_FILTER_INDEX;

	filter_offset = dmc_unrar_filters_rar4_read_number(&bs) + current_output_offset;
	if (flags & 0x40)
		filter_offset += 258;

	/* Filter input length. */

	if (!(flags & 0x20)) {
		if (index >= filters->internal_state->filter_count)
			return DMC_UNRAR_FILTERS_REUSE_LENGTH_NEW_FILTER;

		filter_length = filters->internal_state->filters[index].last_in_length;
	} else
		filter_length = dmc_unrar_filters_rar4_read_number(&bs);

	if (filter_length >= DMC_UNRAR_FILTERS_MEMORY_SIZE)
		return DMC_UNRAR_FILTERS_INVALID_LENGTH;

	/* Registers. */

	usage_count = 0;
	if (index < filters->internal_state->filter_count)
		usage_count = filters->internal_state->filters[index].usage_count;

	registers[0] = 0;
	registers[1] = 0;
	registers[2] = 0;
	registers[3] = DMC_UNRAR_FILTERS_MEMORY_SIZE;
	registers[4] = filter_length;
	registers[5] = usage_count;
	registers[6] = 0;
	registers[7] = DMC_UNRAR_FILTERS_TOTAL_MEMORY_SIZE;

	if (flags & 0x10) {
		const uint8_t mask = dmc_unrar_bs_read_bits(&bs, 7);
		int i;

		for (i = 0; i < 7; i++)
			if (mask & (1 << i))
				registers[i] = dmc_unrar_filters_rar4_read_number(&bs);
	}

	if (new_filter) {
		size_t bytecode_length = dmc_unrar_filters_rar4_read_number(&bs), i;
		if ((bytecode_length < 2) || (bytecode_length > DMC_UNRAR_FILTERS_BYTECODE_SIZE))
			return DMC_UNRAR_FILTERS_UNKNOWN;

		for (i = 0; i < bytecode_length; i++)
			bytecode[i] = dmc_unrar_bs_read_bits(&bs, 8);

		{
			dmc_unrar_return result;
			result = dmc_unrar_filters_create_rar4_filter_from_bytecode(filters, bytecode, bytecode_length);
			if (result != DMC_UNRAR_OK)
				return result;
		}
	}

	DMC_UNRAR_ASSERT(index < filters->internal_state->filter_count);

	filters->internal_state->filters[index].last_in_length = filter_length;
	filters->internal_state->filters[index].usage_count++;

	/* We're skipping the data section here, because we don't care. */

	return dmc_unrar_filters_create_stack_entry(filters, index, filter_offset, filter_length, registers);
}

static uint8_t *dmc_unrar_filters_get_memory(dmc_unrar_filters *filters) {
	DMC_UNRAR_ASSERT(filters && filters->internal_state);

	return filters->internal_state->memory;
}

static dmc_unrar_return dmc_unrar_filters_run(dmc_unrar_filters *filters,
		size_t current_output_offset, size_t current_file_start, size_t *out_offset,
		size_t *out_length) {

	dmc_unrar_return return_code;
	bool first = true, result;

	DMC_UNRAR_ASSERT(filters && filters->internal_state);
	DMC_UNRAR_ASSERT(out_offset && out_length);

	DMC_UNRAR_ASSERT(!dmc_unrar_filters_empty(filters));
	DMC_UNRAR_ASSERT(dmc_unrar_filters_get_first_offset(filters) == current_output_offset);

	*out_length = filters->internal_state->stack[0].length;

	while (filters->internal_state->stack_count != 0) {
		uint8_t *memory = filters->internal_state->memory;
		dmc_unrar_filters_stack_entry *stack = &filters->internal_state->stack[0];
		dmc_unrar_filters_filter *filter = NULL;

		DMC_UNRAR_ASSERT(stack->filter_index < filters->internal_state->filter_count);
		filter = &filters->internal_state->filters[stack->filter_index];

		/* Check if this filter applies. */
		if ((stack->offset != current_output_offset) || (stack->length != *out_length))
			break;

		/* If this isn't the first filter in the chain, we need to move the last result in place. */
		if (!first)
			memmove(memory, memory + *out_offset, *out_length);

		return_code = filter->func(memory, DMC_UNRAR_FILTERS_MEMORY_SIZE,
			current_output_offset - current_file_start,
			stack->length, stack->registers, out_offset, out_length);

		if (return_code != DMC_UNRAR_OK)
			return return_code;

		result = dmc_unrar_filters_stack_pop(filters);
		DMC_UNRAR_ASSERT(result);

		first = false;
	}

	return DMC_UNRAR_OK;
}

static uint32_t dmc_unrar_filters_rar4_read_number(dmc_unrar_bs *bs) {
	/* Read a RAR4 RARVM number out of the bit stream. The number is prefixed by
	 * a 2-bit code to describe how wide it is, to pack small numbers more
	 * efficiently. */

	const uint8_t number_width = dmc_unrar_bs_read_bits(bs, 2);

	switch (number_width) {
		/* The number is 4 bits wide. */
		case 0:
			return dmc_unrar_bs_read_bits(bs, 4);

		/* The number is 8 bits wide, either positive or negative. */
		case 1:
		{
			const uint32_t value = dmc_unrar_bs_read_bits(bs, 8);
			if (value >= 16)
				return value;

			/* Negative values are stored in two's complement, so upper bits are all 1. */
			return 0xFFFFFF00 | (value << 4) | dmc_unrar_bs_read_bits(bs, 4);
		}

		/* The number is 16 bits wide. */
		case 2:
			return dmc_unrar_bs_read_bits(bs, 16);

		/* The number is 32 bits wide. */
		default:
			return dmc_unrar_bs_read_bits(bs, 32);
	}
}

static uint32_t dmc_unrar_filters_rar5_read_number(dmc_unrar_bs *bs) {
	const size_t count = dmc_unrar_bs_read_bits(bs, 2) + 1;
	uint32_t value = 0;

	size_t i;
	for (i = 0; i < count; i++)
		value += dmc_unrar_bs_read_bits(bs, 8) << (i * 8);

	return value;
}

static void dmc_unrar_filters_clear(dmc_unrar_filters *filters) {
	DMC_UNRAR_ASSERT(filters && filters->internal_state);

	filters->internal_state->filter_count = 0;
	filters->internal_state->stack_count  = 0;
}

typedef enum {
	DMC_UNRAR_FILTERS_PROGRAM4_30_DELTA,
	DMC_UNRAR_FILTERS_PROGRAM4_30_AUDIO,
	DMC_UNRAR_FILTERS_PROGRAM4_30_COLOR,
	DMC_UNRAR_FILTERS_PROGRAM4_30_X86,
	DMC_UNRAR_FILTERS_PROGRAM4_30_X86_E9,
	DMC_UNRAR_FILTERS_PROGRAM4_30_ITANIUM,

	DMC_UNRAR_FILTERS_PROGRAM4_UNKNOWN

} dmc_unrar_filters_program4;

typedef enum {
	DMC_UNRAR_FILTERS_PROGRAM5_50_DELTA,
	DMC_UNRAR_FILTERS_PROGRAM5_50_X86,
	DMC_UNRAR_FILTERS_PROGRAM5_50_X86_E9,
	DMC_UNRAR_FILTERS_PROGRAM5_50_ARM

} dmc_unrar_filters_program5;

static dmc_unrar_filters_program4 dmc_unrar_filters_identify_rar4(const uint8_t *bytecode, size_t size) {
	const uint32_t crc32 = dmc_unrar_crc32_calculate_from_mem(bytecode, size);

	switch (crc32) {
		case 0x0E06077D:
			if (size == 0x1D)
				return DMC_UNRAR_FILTERS_PROGRAM4_30_DELTA;
			break;

		case 0xBC85E701:
			if (size == 0xD8)
				return DMC_UNRAR_FILTERS_PROGRAM4_30_AUDIO;
			break;

		case 0x1C2C5DC8:
			if (size == 0x95)
				return DMC_UNRAR_FILTERS_PROGRAM4_30_COLOR;
			break;

		case 0xAD576887:
			if (size == 0x35)
				return DMC_UNRAR_FILTERS_PROGRAM4_30_X86;
			break;

		case 0x3CD7E57E:
			if (size == 0x39)
				return DMC_UNRAR_FILTERS_PROGRAM4_30_X86_E9;
			break;

		case 0x3769893F:
			if (size == 0x78)
				return DMC_UNRAR_FILTERS_PROGRAM4_30_ITANIUM;
			break;

		default:
			break;
	}

	return DMC_UNRAR_FILTERS_PROGRAM4_UNKNOWN;
}

static dmc_unrar_return dmc_unrar_filters_30_delta_func(uint8_t *memory, size_t memory_size,
	size_t file_position, size_t in_length, const uint32_t *registers,
	size_t *out_offset, size_t *out_length);
static dmc_unrar_return dmc_unrar_filters_30_audio_func(uint8_t *memory, size_t memory_size,
	size_t file_position, size_t in_length, const uint32_t *registers,
	size_t *out_offset, size_t *out_length);
static dmc_unrar_return dmc_unrar_filters_30_color_func(uint8_t *memory, size_t memory_size,
	size_t file_position, size_t in_length, const uint32_t *registers,
	size_t *out_offset, size_t *out_length);
static dmc_unrar_return dmc_unrar_filters_30_x86_func(uint8_t *memory, size_t memory_size,
	size_t file_position, size_t in_length, const uint32_t *registers,
	size_t *out_offset, size_t *out_length);
static dmc_unrar_return dmc_unrar_filters_30_x86_e9_func(uint8_t *memory, size_t memory_size,
	size_t file_position, size_t in_length, const uint32_t *registers,
	size_t *out_offset, size_t *out_length);

static dmc_unrar_return dmc_unrar_filters_create_rar4_filter_from_bytecode(dmc_unrar_filters *filters,
		const uint8_t *bytecode, size_t bytecode_length) {

	size_t i;
	uint8_t xor_sum = 0;
	dmc_unrar_filters_program4 program = DMC_UNRAR_FILTERS_PROGRAM4_UNKNOWN;
	dmc_unrar_filters_filter *filter = NULL;

	DMC_UNRAR_ASSERT(filters && bytecode && bytecode_length);

	for (i = 1; i < bytecode_length; i++)
		xor_sum ^= bytecode[i];

	if (xor_sum != bytecode[0])
		return DMC_UNRAR_FILTERS_XOR_SUM_NO_MATCH;

	program = dmc_unrar_filters_identify_rar4(bytecode, bytecode_length);
	if (program == DMC_UNRAR_FILTERS_PROGRAM4_UNKNOWN)
		return DMC_UNRAR_FILTERS_UNKNOWN;
	if (program == DMC_UNRAR_FILTERS_PROGRAM4_30_ITANIUM)
		return DMC_UNRAR_FILTERS_UNSUPPORED_ITANIUM;

	if (!dmc_unrar_filters_grow_filters(filters))
		return DMC_UNRAR_ALLOC_FAIL;

	filter = &filters->internal_state->filters[filters->internal_state->filter_count - 1];

	DMC_UNRAR_CLEAR_OBJ(*filter);

	switch (program) {
		case DMC_UNRAR_FILTERS_PROGRAM4_30_DELTA:
			filter->func = &dmc_unrar_filters_30_delta_func;
			break;

		case DMC_UNRAR_FILTERS_PROGRAM4_30_AUDIO:
			filter->func = &dmc_unrar_filters_30_audio_func;
			break;

		case DMC_UNRAR_FILTERS_PROGRAM4_30_COLOR:
			filter->func = &dmc_unrar_filters_30_color_func;
			break;

		case DMC_UNRAR_FILTERS_PROGRAM4_30_X86:
			filter->func = &dmc_unrar_filters_30_x86_func;
			break;

		case DMC_UNRAR_FILTERS_PROGRAM4_30_X86_E9:
			filter->func = &dmc_unrar_filters_30_x86_e9_func;
			break;

		default:
			return DMC_UNRAR_FILTERS_UNKNOWN;
	}

	return DMC_UNRAR_OK;
}

static dmc_unrar_return dmc_unrar_filters_create_stack_entry(dmc_unrar_filters *filters,
		size_t filter_index, size_t filter_offset, size_t filter_length, uint32_t *registers) {

	dmc_unrar_filters_stack_entry *entry;

	DMC_UNRAR_ASSERT(filters && registers);
	DMC_UNRAR_ASSERT(filters->internal_state && filters->internal_state->stack);
	DMC_UNRAR_ASSERT(filter_index < filters->internal_state->filter_count);

	if (!dmc_unrar_filters_grow_stack(filters))
		return DMC_UNRAR_ALLOC_FAIL;

	entry = &filters->internal_state->stack[filters->internal_state->stack_count - 1];

	entry->filter_index = filter_index;

	entry->offset = filter_offset;
	entry->length = filter_length;

	memcpy(entry->registers, registers, 8 * sizeof(uint32_t));

	return DMC_UNRAR_OK;
}

static bool dmc_unrar_filters_init_filters(dmc_unrar_filters *filters) {
	filters->internal_state->filter_count    = 0;
	filters->internal_state->filter_capacity = DMC_UNRAR_ARRAY_INITIAL_CAPACITY;

	filters->internal_state->filters = (dmc_unrar_filters_filter *)
		dmc_unrar_malloc(filters->alloc, filters->internal_state->filter_capacity,
		                 sizeof(dmc_unrar_filters_filter));

	return filters->internal_state->filters != NULL;
}

static bool dmc_unrar_filters_ensure_capacity_filters(dmc_unrar_filters *filters) {
	if (filters->internal_state->filter_count < filters->internal_state->filter_capacity)
		return true;

	filters->internal_state->filter_capacity =
		DMC_UNRAR_MAX(filters->internal_state->filter_capacity, 1);

	filters->internal_state->filter_capacity *= 2;
	filters->internal_state->filters = (dmc_unrar_filters_filter *)
		dmc_unrar_realloc(filters->alloc, filters->internal_state->filters,
		                  filters->internal_state->filter_capacity, sizeof(dmc_unrar_filters_filter));

	return filters->internal_state->filters != NULL;
}

static bool dmc_unrar_filters_grow_filters(dmc_unrar_filters *filters) {
	if (!dmc_unrar_filters_ensure_capacity_filters(filters))
		return false;

	filters->internal_state->filter_count++;
	return true;
}

static bool dmc_unrar_filters_init_stack(dmc_unrar_filters *filters) {
	filters->internal_state->stack_count    = 0;
	filters->internal_state->stack_capacity = DMC_UNRAR_ARRAY_INITIAL_CAPACITY;

	filters->internal_state->stack = (dmc_unrar_filters_stack_entry *)
		dmc_unrar_malloc(filters->alloc, filters->internal_state->stack_capacity,
		                 sizeof(dmc_unrar_filters_stack_entry));

	return filters->internal_state->filters != NULL;
}

static bool dmc_unrar_filters_ensure_capacity_stack(dmc_unrar_filters *filters) {
	if (filters->internal_state->stack_count < filters->internal_state->stack_capacity)
		return true;

	filters->internal_state->stack_capacity =
		DMC_UNRAR_MAX(filters->internal_state->stack_capacity, 1);

	filters->internal_state->stack_capacity *= 2;
	filters->internal_state->stack = (dmc_unrar_filters_stack_entry *)
		dmc_unrar_realloc(filters->alloc, filters->internal_state->stack,
		                  filters->internal_state->stack_capacity,
		                  sizeof(dmc_unrar_filters_stack_entry));

	return filters->internal_state->filters != NULL;
}

static bool dmc_unrar_filters_grow_stack(dmc_unrar_filters *filters) {
	if (!dmc_unrar_filters_ensure_capacity_stack(filters))
		return false;

	filters->internal_state->stack_count++;
	return true;
}

static bool dmc_unrar_filters_stack_pop(dmc_unrar_filters *filters) {
	if (!filters || !filters->internal_state || !filters->internal_state->stack ||
	    !filters->internal_state->stack_count)
		return false;

	filters->internal_state->stack_count--;
	memmove(&filters->internal_state->stack[0], &filters->internal_state->stack[1],
	        filters->internal_state->stack_count * sizeof(dmc_unrar_filters_stack_entry));

	return true;
}

static void dmc_unrar_filters_delta_filter(const uint8_t *src, uint8_t *dst,
		size_t length, size_t channel_count) {

	size_t i, offset;
	for (i = 0; i < channel_count; i++) {
		uint8_t last = 0;

		for (offset = i; offset < length; offset += channel_count)
			last = dst[offset] = last - *src++;
	}
}

static dmc_unrar_return dmc_unrar_filters_30_delta_func(uint8_t *memory, size_t memory_size,
		size_t file_position, size_t in_length, const uint32_t *registers,
		size_t *out_offset, size_t *out_length) {

	const size_t channel_count = registers[0];

	if (in_length > (memory_size / 2))
		return DMC_UNRAR_FILTERS_INVALID_LENGTH;

	*out_offset = in_length;
	*out_length = in_length;

	dmc_unrar_filters_delta_filter(memory, memory + *out_offset, in_length, channel_count);

	(void)file_position;
	return DMC_UNRAR_OK;
}

typedef struct dmc_unrar_filters_30_audio_state_tag {
	size_t count;

	int last_delta;

	int weight[5];
	int delta[4];
	int error[7];

	int last_byte;

} dmc_unrar_filters_30_audio_state;

static uint8_t dmc_unrar_filters_30_audio_decode(dmc_unrar_filters_30_audio_state *state,
		int cur_delta) {

	int pred_byte, cur_byte, pred_error;

	DMC_UNRAR_ASSERT(state);

	state->delta[2] = state->delta[1];
	state->delta[1] = state->last_delta - state->delta[0];
	state->delta[0] = state->last_delta;

	pred_byte = ((
		               8 * state->last_byte +
		state->weight[0] * state->delta[0] +
		state->weight[1] * state->delta[1] +
		state->weight[2] * state->delta[2]
	) >> 3) & 0xFF;

	cur_byte = (pred_byte - cur_delta) & 0xFF;

	pred_error = ((int8_t)cur_delta) << 3;

	state->error[0] += DMC_UNRAR_ABS(pred_error);
	state->error[1] += DMC_UNRAR_ABS(pred_error - state->delta[0]);
	state->error[2] += DMC_UNRAR_ABS(pred_error + state->delta[0]);
	state->error[3] += DMC_UNRAR_ABS(pred_error - state->delta[1]);
	state->error[4] += DMC_UNRAR_ABS(pred_error + state->delta[1]);
	state->error[5] += DMC_UNRAR_ABS(pred_error - state->delta[2]);
	state->error[6] += DMC_UNRAR_ABS(pred_error + state->delta[2]);

	state->last_delta = (int8_t)(cur_byte - state->last_byte);
	state->last_byte  = cur_byte;

	if ((state->count & 0x1F) == 0) {
		int min_error = state->error[0];
		int min_index = 0;
		int i;

		for (i = 1; i < 7; i++) {
			if (state->error[i] < min_error) {
				min_error = state->error[i];
				min_index = i;
			}
		}

		DMC_UNRAR_CLEAR_ARRAY(state->error);

		switch (min_index) {
			case 1: if (state->weight[0] >= -16) state->weight[0]--; break;
			case 2: if (state->weight[0] <   16) state->weight[0]++; break;
			case 3: if (state->weight[1] >= -16) state->weight[1]--; break;
			case 4: if (state->weight[1] <   16) state->weight[1]++; break;
			case 5: if (state->weight[2] >= -16) state->weight[2]--; break;
			case 6: if (state->weight[2] <   16) state->weight[2]++; break;
		}
	}

	state->count++;

	return cur_byte;
}

static dmc_unrar_return dmc_unrar_filters_30_audio_func(uint8_t *memory, size_t memory_size,
		size_t file_position, size_t in_length, const uint32_t *registers,
		size_t *out_offset, size_t *out_length) {

	if (in_length > (memory_size / 2))
		return DMC_UNRAR_FILTERS_INVALID_LENGTH;

	*out_offset = in_length;
	*out_length = in_length;

	{
		const size_t channel_count = registers[0];

		const uint8_t *src = memory;
		uint8_t *dst = memory + *out_offset;

		size_t i, offset;
		for (i = 0; i < channel_count; i++) {
			dmc_unrar_filters_30_audio_state state;
			DMC_UNRAR_CLEAR_OBJ(state);

			for (offset = i; offset < in_length; offset += channel_count)
				dst[offset] = dmc_unrar_filters_30_audio_decode(&state, *src++);
		}
	}

	(void)file_position;
	return DMC_UNRAR_OK;
}

static void dmc_unrar_filters_30_color_filter(const uint8_t *src, uint8_t *dst,
		size_t length, uint32_t r0, uint32_t r1) {

	int32_t r3[7];
	size_t color, offset, i;

	for (color = 0; color < 3; color++) {
		r3[0] = 0;

		for (offset = color; offset < length; offset += 3) {
			int32_t r6 = r3[0];

			if (offset >= r0) {
				r3[1] = dst[offset - r0];
				r3[2] = dst[offset - r0 + 3];

				r3[3] = r3[0] + r3[2] - r3[1];

				r3[4] = DMC_UNRAR_ABS(r3[3] - r3[0]);
				r3[5] = DMC_UNRAR_ABS(r3[3] - r3[2]);
				r3[6] = DMC_UNRAR_ABS(r3[3] - r3[1]);

				if ((r3[4] > r3[5]) || (r3[4] > r3[6])) {
					r6 = r3[2];

					if (r3[5] > r3[6])
						r6 = r3[1];
				}
			}

			r6 -= *src++;

			r3[0] = (uint8_t)r6;
			dst[offset] = (uint8_t)r6;
		}
	}

	for (i = r1; i < length - 2; i += 3) {
		dst[i    ] += dst[i + 1];
		dst[i + 2] += dst[i + 1];
	}
}

static dmc_unrar_return dmc_unrar_filters_30_color_func(uint8_t *memory, size_t memory_size,
		size_t file_position, size_t in_length, const uint32_t *registers,
		size_t *out_offset, size_t *out_length) {

	if ((in_length > (memory_size / 2)) || (in_length < 3))
		return DMC_UNRAR_FILTERS_INVALID_LENGTH;

	*out_offset = in_length;
	*out_length = in_length;

	dmc_unrar_filters_30_color_filter(memory, memory + *out_offset, in_length, registers[0], registers[1]);

	(void)file_position;
	return DMC_UNRAR_OK;
}

static void dmc_unrar_put_uint32le(uint8_t *data, uint32_t x) {
	data[3] = x >> 24;
	data[2] = x >> 16;
	data[1] = x >>  8;
	data[0] = x;
}

static void dmc_unrar_filters_x86_filter(uint8_t *memory, size_t length,
		int32_t file_pos, bool handle_e9, bool wrap_position) {

	const int32_t file_size = 0x1000000;
	size_t i;

	for (i = 0; i <= length - 5; i++) {
		if ((memory[i] == 0xE8) || (handle_e9 && (memory[i] == 0xE9))) {
			int32_t cur_pos = file_pos + i + 1, address;

			if (wrap_position)
				cur_pos %= file_size;

			address = (int32_t)dmc_unrar_get_uint32le(memory + i + 1);
			if (address < 0) {
				if ((address + cur_pos >= 0))
					dmc_unrar_put_uint32le(memory + i + 1, address + file_size);
			} else {
				if (address < file_size)
					dmc_unrar_put_uint32le(memory + i + 1, address - cur_pos);
			}

			i += 4;
		}
	}
}

static dmc_unrar_return dmc_unrar_filters_30_x86_func(uint8_t *memory, size_t memory_size,
		size_t file_position, size_t in_length, const uint32_t *registers,
		size_t *out_offset, size_t *out_length) {

	if ((in_length > memory_size) || (in_length < 4))
		return DMC_UNRAR_FILTERS_INVALID_LENGTH;
	if (file_position >= 0x7FFFFFFF)
		return DMC_UNRAR_FILTERS_INVALID_FILE_POSITION;

	*out_offset = 0;
	*out_length = in_length;

	dmc_unrar_filters_x86_filter(memory, in_length, file_position, false, false);

	(void)registers;
	return DMC_UNRAR_OK;
}

static dmc_unrar_return dmc_unrar_filters_30_x86_e9_func(uint8_t *memory, size_t memory_size,
		size_t file_position, size_t in_length, const uint32_t *registers,
		size_t *out_offset, size_t *out_length) {

	if ((in_length > memory_size) || (in_length < 4))
		return DMC_UNRAR_FILTERS_INVALID_LENGTH;
	if (file_position >= 0x7FFFFFFF)
		return DMC_UNRAR_FILTERS_INVALID_FILE_POSITION;

	*out_offset = 0;
	*out_length = in_length;

	dmc_unrar_filters_x86_filter(memory, in_length, file_position, true, false);

	(void)registers;
	return DMC_UNRAR_OK;
}

static dmc_unrar_return dmc_unrar_filters_50_delta_func(uint8_t *memory, size_t memory_size,
	size_t file_position, size_t in_length, const uint32_t *registers,
	size_t *out_offset, size_t *out_length);
static dmc_unrar_return dmc_unrar_filters_50_x86_func(uint8_t *memory, size_t memory_size,
	size_t file_position, size_t in_length, const uint32_t *registers,
	size_t *out_offset, size_t *out_length);
static dmc_unrar_return dmc_unrar_filters_50_x86_e9_func(uint8_t *memory, size_t memory_size,
	size_t file_position, size_t in_length, const uint32_t *registers,
	size_t *out_offset, size_t *out_length);
static dmc_unrar_return dmc_unrar_filters_50_arm_func(uint8_t *memory, size_t memory_size,
	size_t file_position, size_t in_length, const uint32_t *registers,
	size_t *out_offset, size_t *out_length);

static dmc_unrar_return dmc_unrar_filters_rar5_parse(dmc_unrar_filters *filters,
		dmc_unrar_bs *bs, size_t current_output_offset) {

	size_t filter_index = SIZE_MAX;
	dmc_unrar_filters_filter *filter = NULL;
	uint32_t registers[8];

	size_t  filter_offset = dmc_unrar_filters_rar5_read_number(bs) + current_output_offset;
	size_t  filter_length = dmc_unrar_filters_rar5_read_number(bs);
	uint8_t filter_type   = dmc_unrar_bs_read_bits(bs, 3);

	if (!dmc_unrar_filters_grow_filters(filters))
		return DMC_UNRAR_ALLOC_FAIL;

	filter_index = filters->internal_state->filter_count - 1;
	filter = &filters->internal_state->filters[filter_index];

	DMC_UNRAR_CLEAR_OBJ(*filter);

	registers[0] = 0;
	registers[1] = 0;
	registers[2] = 0;
	registers[3] = DMC_UNRAR_FILTERS_MEMORY_SIZE;
	registers[4] = filter_length;
	registers[5] = filter->usage_count;
	registers[6] = 0;
	registers[7] = DMC_UNRAR_FILTERS_TOTAL_MEMORY_SIZE;

	switch (filter_type) {
		case DMC_UNRAR_FILTERS_PROGRAM5_50_DELTA:
			registers[0] = dmc_unrar_bs_read_bits(bs, 5) + 1;
			filter->func = &dmc_unrar_filters_50_delta_func;
			break;

		case DMC_UNRAR_FILTERS_PROGRAM5_50_X86:
			filter->func = &dmc_unrar_filters_50_x86_func;
			break;

		case DMC_UNRAR_FILTERS_PROGRAM5_50_X86_E9:
			filter->func = &dmc_unrar_filters_50_x86_e9_func;
			break;

		case DMC_UNRAR_FILTERS_PROGRAM5_50_ARM:
			filter->func = &dmc_unrar_filters_50_arm_func;
			break;

		default:
			return DMC_UNRAR_FILTERS_UNKNOWN;
	}

	filter->last_in_length = filter_length;
	filter->usage_count++;

	if (dmc_unrar_bs_has_error(bs))
		return DMC_UNRAR_READ_FAIL;

	return dmc_unrar_filters_create_stack_entry(filters, filter_index,
		filter_offset, filter_length, registers);
}

static dmc_unrar_return dmc_unrar_filters_50_delta_func(uint8_t *memory, size_t memory_size,
	size_t file_position, size_t in_length, const uint32_t *registers,
	size_t *out_offset, size_t *out_length) {

	const size_t channel_count = registers[0];

	if (in_length > (memory_size / 2))
		return DMC_UNRAR_FILTERS_INVALID_LENGTH;

	*out_offset = in_length;
	*out_length = in_length;

	dmc_unrar_filters_delta_filter(memory, memory + *out_offset, in_length, channel_count);

	(void)file_position;
	return DMC_UNRAR_OK;
}

static dmc_unrar_return dmc_unrar_filters_50_x86_func(uint8_t *memory, size_t memory_size,
	size_t file_position, size_t in_length, const uint32_t *registers,
	size_t *out_offset, size_t *out_length) {

	if ((in_length > memory_size) || (in_length < 4))
		return DMC_UNRAR_FILTERS_INVALID_LENGTH;
	if (file_position >= 0x7FFFFFFF)
		return DMC_UNRAR_FILTERS_INVALID_FILE_POSITION;

	*out_offset = 0;
	*out_length = in_length;

	dmc_unrar_filters_x86_filter(memory, in_length, file_position, false, true);

	(void)registers;
	return DMC_UNRAR_OK;
}

static dmc_unrar_return dmc_unrar_filters_50_x86_e9_func(uint8_t *memory, size_t memory_size,
	size_t file_position, size_t in_length, const uint32_t *registers,
	size_t *out_offset, size_t *out_length) {

	if ((in_length > memory_size) || (in_length < 4))
		return DMC_UNRAR_FILTERS_INVALID_LENGTH;
	if (file_position >= 0x7FFFFFFF)
		return DMC_UNRAR_FILTERS_INVALID_FILE_POSITION;

	*out_offset = 0;
	*out_length = in_length;

	dmc_unrar_filters_x86_filter(memory, in_length, file_position, true, true);

	(void)registers;
	return DMC_UNRAR_OK;
}

static void dmc_unrar_filters_50_arm_filter(uint8_t *memory, size_t length, int32_t file_pos) {
	size_t i;

	for (i = 0; i < length - 4; i += 4) {
		if (memory[i + 3] == 0xEB) {
			uint32_t offset = memory[i] + (memory[i + 1] << 8) + (memory[i + 2] << 16);

			offset -= ((uint32_t)file_pos + i) / 4;

			memory[i    ] = offset;
			memory[i + 1] = offset >>  8;
			memory[i + 2] = offset >> 16;
		}
	}
}

static dmc_unrar_return dmc_unrar_filters_50_arm_func(uint8_t *memory, size_t memory_size,
	size_t file_position, size_t in_length, const uint32_t *registers,
	size_t *out_offset, size_t *out_length) {

	if ((in_length > memory_size) || (in_length < 4))
		return DMC_UNRAR_FILTERS_INVALID_LENGTH;
	if (file_position >= 0x7FFFFFFF)
		return DMC_UNRAR_FILTERS_INVALID_FILE_POSITION;

	*out_offset = 0;
	*out_length = in_length;

	dmc_unrar_filters_50_arm_filter(memory, in_length, file_position);

	(void)registers;
	return DMC_UNRAR_OK;
}
#endif /* DMC_UNRAR_DISABLE_FILTERS */
/* '--- */

#ifdef __cplusplus
}
#endif

#endif /* DMC_UNRAR_HEADER_FILE_ONLY */

/* The unrar code in this file is based heavily on three (L)GPL projects:
 *
 * - The Unarchiver (<http://unarchiver.c3.cx/unarchiver>) by Dag Ågren,
 *   licensed under the terms of the GNU Lesser General Public License version
 *   2.1 or later.
 *
 *   The original copyright note in The Unarchiver reads as follows:
 *
 *   This program, "The Unarchiver", its accompanying libraries, "XADMaster"
 *   and "UniversalDetector", and the various smaller utility programs, such
 *   as "unar" and "lsar", are distributed under the GNU Lesser General
 *   Public License as published by the Free Software Foundation; either
 *   version 2.1 of the License, or (at your option) any later version.
 *
 *   "UniversalDetector" is also available under other licenses, such as the
 *   Mozilla Public License. Please refer to the files in its subdirectory
 *   for further information.
 *
 *   The GNU Lesser General Public License might be too restrictive for some
 *   users of this code. Parts of the code are derived from earlier
 *   LGPL-licensed code and will as such always be bound by the LGPL, but
 *   some parts of the code are developed from scratch by the author of The
 *   Unarchiver, Dag Ågren, and can thus be made available under a more
 *   permissive license. For simplicity, everything is currently licensed
 *   under the LGPL, but if you are interested in using any code from this
 *   project under another license, please contact the author for further
 *   information.
 *
 *       - Dag Ågren, <paracelsus@gmail.com>
 *
 * - unrarlib, the UniquE RAR File Libary (<http://unrarlib.org/>) by
 *   Christian Scheurer. unrarlib is dual-licensed, available under
 *   the terms of the UniquE RAR File Library license and the GNU
 *   General Public License Version 2 or later.
 *
 *   The original copyright note in unrarlib reads as follows:
 *
 *   Copyright (C) 2000-2002 by Christian Scheurer (www.ChristianScheurer.ch)
 *   UNIX port copyright (c) 2000-2002 by Johannes Winkelmann (jw@tks6.net)
 *
 *   The contents of this file are subject to the UniquE RAR File Library
 *   License (the "unrarlib-license.txt"). You may not use this file except
 *   in compliance with the License. You may obtain a copy of the License
 *   at http://www.unrarlib.org/license.html.
 *   Software distributed under the License is distributed on an "AS IS"
 *   basis, WITHOUT WARRANTY OF ANY KIND, either express or implied warranty.
 *
 *   Alternatively, the contents of this file may be used under the terms
 *   of the GNU General Public License Version 2 or later (the "GPL"), in
 *   which case the provisions of the GPL are applicable instead of those
 *   above. If you wish to allow use of your version of this file only
 *   under the terms of the GPL and not to allow others to use your version
 *   of this file under the terms of the UniquE RAR File Library License,
 *   indicate your decision by deleting the provisions above and replace
 *   them with the notice and other provisions required by the GPL. If you
 *   do not delete the provisions above, a recipient may use your version
 *   of this file under the terms of the GPL or the UniquE RAR File Library
 *   License.
 *
 * - unrar-free (<https://gna.org/projects/unrar/>), by Jeroen Dekkers and
 *   Ben Asselstine, which itself is based on unrarlib. unrar-free is licensed
 *   under the terms of the GNU General Public License Version 2 or later.
 *
 *   The original copyright note in unrar-free reads as follows:
 *
 *   Copyright (C) 2004  Jeroen Dekkers <jeroen@dekkers.cx>
 *   Copyright (C) 2004  Ben Asselstine <benasselstine@canada.com>
 *   Copyright (C) 2000-2002  Christian Scheurer (www.ChristianScheurer.ch)
 *   Copyright (C) 2000-2002  Johannes Winkelmann (jw@tks6.net)
 *   RAR decompression code:
 *   Copyright (c) 1993-2002  Eugene Roshal
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation; either version 2
 *   of the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc.,
 *   51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *
 * The bitstream code is heavily based on the bitstream interface found in the
 * single-file FLAC decoding library dr_flac by David Reid
 * (<https://mackron.github.io/dr_flac.html>), licensed under the terms of the
 * unlicense.
 *
 * Additionally, dmc_unrar takes some inspiration from miniz.c, the public
 * domain single-file deflate/inflate, zlib-subset, ZIP reading/writing library
 * (<https://github.com/richgel999/miniz>) by Rich Geldreich. miniz.c is
 * licensed under the terms of the unlicense.
 */
