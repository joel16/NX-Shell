/*
 * Copyright 2006 Richard Wilson <richard.wilson@netsurf-browser.org>
 * Copyright 2008 Sean Fox <dyntryx@gmail.com>
 *
 * This file is part of NetSurf's libnsbmp, http://www.netsurf-browser.org/
 * Licenced under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 */

/**
 * \file
 * Bitmap file decoding interface.
 */

#ifndef libnsbmp_h_
#define libnsbmp_h_

#if defined (__cplusplus)
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/* bmp flags */
#define BMP_NEW			0
/** image is opaque (as opposed to having an alpha mask) */
#define BMP_OPAQUE		(1 << 0)
/** memory should be wiped */
#define BMP_CLEAR_MEMORY	(1 << 1)

/**
 * error return values
 */
typedef enum {
        BMP_OK = 0,
        BMP_INSUFFICIENT_MEMORY = 1,
        BMP_INSUFFICIENT_DATA = 2,
        BMP_DATA_ERROR = 3
} bmp_result;

/**
 * encoding types
 */
typedef enum {
        BMP_ENCODING_RGB = 0,
        BMP_ENCODING_RLE8 = 1,
        BMP_ENCODING_RLE4 = 2,
        BMP_ENCODING_BITFIELDS = 3
} bmp_encoding;

/* API for Bitmap callbacks */
typedef void* (*bmp_bitmap_cb_create)(int width, int height, unsigned int state);
typedef void (*bmp_bitmap_cb_destroy)(void *bitmap);
typedef unsigned char* (*bmp_bitmap_cb_get_buffer)(void *bitmap);
typedef size_t (*bmp_bitmap_cb_get_bpp)(void *bitmap);

/**
 * The Bitmap callbacks function table
 */
typedef struct bmp_bitmap_callback_vt_s {
        /** Callback to allocate bitmap storage. */
        bmp_bitmap_cb_create bitmap_create;
        /** Called to free bitmap storage. */
        bmp_bitmap_cb_destroy bitmap_destroy;
        /** Return a pointer to the pixel data in a bitmap. */
        bmp_bitmap_cb_get_buffer bitmap_get_buffer;
        /** Find the width of a pixel row in bytes. */
        bmp_bitmap_cb_get_bpp bitmap_get_bpp;
} bmp_bitmap_callback_vt;

/**
 * bitmap image
 */
typedef struct bmp_image {
        /** callbacks for bitmap functions */
        bmp_bitmap_callback_vt bitmap_callbacks;
        /** pointer to BMP data */
        uint8_t *bmp_data;
        /** width of BMP (valid after _analyse) */
        uint32_t width;
        /** heigth of BMP (valid after _analyse) */
        uint32_t height;
        /** whether the image has been decoded */
        bool decoded;
        /** decoded image */
        void *bitmap;

        /* Internal members are listed below */
        /** total number of bytes of BMP data available */
        uint32_t buffer_size;
        /** pixel encoding type */
        bmp_encoding encoding;
        /** offset of bitmap data */
        uint32_t bitmap_offset;
        /** bits per pixel */
        uint16_t bpp;
        /** number of colours */
        uint32_t colours;
        /** colour table */
        uint32_t *colour_table;
        /** whether to use bmp's limited transparency */
        bool limited_trans;
        /** colour to display for "transparent" pixels when using limited
         * transparency
         */
        uint32_t trans_colour;
        /** scanlines are top to bottom */
        bool reversed;
        /** image is part of an ICO, mask follows */
        bool ico;
        /** true if the bitmap does not contain an alpha channel */
        bool opaque;
        /** four bitwise mask */
        uint32_t mask[4];
        /** four bitwise shifts */
        int32_t shift[4];
        /** colour representing "transparency" in the bitmap */
        uint32_t transparent_index;
} bmp_image;

typedef struct ico_image {
        bmp_image bmp;
        struct ico_image *next;
} ico_image;

/**
 * icon image collection
 */
typedef struct ico_collection {
        /** callbacks for bitmap functions */
        bmp_bitmap_callback_vt bitmap_callbacks;
        /** width of largest BMP */
        uint16_t width;
        /** heigth of largest BMP */
        uint16_t height;

        /* Internal members are listed below */
        /** pointer to ICO data */
        uint8_t *ico_data;
        /** total number of bytes of ICO data available */
        uint32_t buffer_size;
        /** root of linked list of images */
        ico_image *first;
} ico_collection;

/**
 * Initialises bitmap ready for analysing the bitmap.
 *
 * \param bmp The Bitmap to initialise
 * \param callbacks The callbacks the library will call on operations.
 * \return BMP_OK on success or appropriate error code.
 */
bmp_result bmp_create(bmp_image *bmp, bmp_bitmap_callback_vt *callbacks);

/**
 * Initialises icon ready for analysing the icon
 *
 * \param bmp The Bitmap to initialise
 * \param callbacks The callbacks the library will call on operations.
 * \return BMP_OK on success or appropriate error code.
 */
bmp_result ico_collection_create(ico_collection *ico,
                                 bmp_bitmap_callback_vt *callbacks);

/**
 * Analyse a BMP prior to decoding.
 *
 * This will scan the data provided and perform checks to ensure the data is a
 * valid BMP and prepare the bitmap image structure ready for decode.
 *
 * This function must be called and resturn BMP_OK before bmp_decode() as it
 * prepares the bmp internal state for the decode process.
 *
 * \param bmp the BMP image to analyse.
 * \param size The size of data in cdata.
 * \param data The bitmap source data.
 * \return BMP_OK on success or error code on faliure.
 */
bmp_result bmp_analyse(bmp_image *bmp, size_t size, uint8_t *data);

/**
 * Analyse an ICO prior to decoding.
 *
 * This function will scan the data provided and perform checks to ensure the
 * data is a valid ICO.
 *
 * This function must be called before ico_find().
 *
 * \param ico the ICO image to analyse
 * \param size The size of data in cdata.
 * \param data The bitmap source data.
 * \return BMP_OK on success
 */
bmp_result ico_analyse(ico_collection *ico, size_t size, uint8_t *data);

/**
 * Decode a BMP
 *
 * This function decodes the BMP data such that bmp->bitmap is a valid
 * image. The state of bmp->decoded is set to TRUE on exit such that it
 * can easily be identified which BMPs are in a fully decoded state.
 *
 * \param bmp the BMP image to decode
 * \return BMP_OK on success
 */
bmp_result bmp_decode(bmp_image *bmp);

/**
 * Decode a BMP using "limited transparency"
 *
 * Bitmaps do not have native transparency support.  However, there is a
 * "trick" that is used in some instances in which the first pixel of the
 * bitmap becomes the "transparency index".  The decoding application can
 * replace this index with whatever background colour it chooses to
 * create the illusion of transparency.
 *
 * When to use transparency is at the discretion of the decoding
 * application.
 *
 * \param bmp the BMP image to decode
 * \param colour the colour to use as "transparent"
 * \return BMP_OK on success
 */
bmp_result bmp_decode_trans(bmp_image *bmp, uint32_t transparent_colour);

/**
 * Finds the closest BMP within an ICO collection
 *
 * This function finds the BMP with dimensions as close to a specified set
 * as possible from the images in the collection.
 *
 * \param ico the ICO collection to examine
 * \param width the preferred width (0 to use ICO header width)
 * \param height the preferred height (0 to use ICO header height)
 */
bmp_image *ico_find(ico_collection *ico, uint16_t width, uint16_t height);

/**
 * Finalise a BMP prior to destruction.
 *
 * \param bmp the BMP image to finalise.
 */
void bmp_finalise(bmp_image *bmp);

/**
 * Finalise an ICO prior to destruction.
 *
 * \param ico the ICO image to finalise,
 */
void ico_finalise(ico_collection *ico);

#if defined (__cplusplus)
}
#endif

#endif
