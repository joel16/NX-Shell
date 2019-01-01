//**********************************************************/
//** Done by  |      Date     |  version |    comment     **/
//**------------------------------------------------------**/
//**   CEV    |    05-2016    |   1.0    |  creation/SDL2 **/
//**********************************************************/

//*******************************************************************************/
//** Based upon  http://giflib.sourceforge.net/whatsinagif/bits_and_bytes.html **/
//*******************************************************************************/


#ifndef GIF_DEC_INCLUDED
#define GIF_DEC_INCLUDED

#include <SDL.h>


//#if SDL_BYTEORDER == SDL_BIG_ENDIAN //SDL1 only


    #define   RMASK  0xff000000
    #define   GMASK  0x00ff0000
    #define   BMASK  0x0000ff00
    #define   AMASK  0x000000ff

    #define COLOR_R(X) ((X&RMASK)>>24)
    #define COLOR_G(X) ((X&GMASK)>>16)
    #define COLOR_B(X) ((X&BMASK)>>8)
    #define COLOR_A(X) (X&AMASK)

/*#else //SDL1 Only

    #define RMASK  0x000000ff
    #define GMASK  0x0000ff00
    #define BMASK  0x00ff0000
    #define AMASK  0xff000000

    #define COLOR_R(X) (X&RMASK)
    #define COLOR_G(X) ((X&GMASK)>>8)
    #define COLOR_B(X) ((X&BMASK)>>16)
    #define COLOR_A(X) ((X&AMASK)>>24)

#endif
*/
#ifdef __cplusplus
extern "C" {
#endif


/***Structures***/

typedef struct
{/*color type*/
    uint8_t r,
            g,
            b,
            a;
}
L_GifColor;


typedef struct
{/*color table*/
    unsigned int numOfColors;
    L_GifColor   *table;
}
L_GifColorTable;//local


typedef struct
{/*header Block*/
    char signature[5];
    char version[5];
}
L_GifHeader;//local


typedef struct
{/*Logical Screen Descriptor pack field, from byte*/
    uint8_t     usesGlobalColor,
                sorted;

    size_t      colorRes,
                numOfColors;
}
L_GifLSDpack;//local


typedef struct
{/*Logical Screen Descriptor Block*/
    unsigned int    pxlAspectRatio;
    uint16_t        width,
                    height;
    L_GifLSDpack    packField;
    uint8_t         bckgrdColorIndex;
}
L_GifLSD;//local


typedef struct
{/*Graphics Control Extension pack field, from byte*/
    uint8_t res,
            disposalMethod,
            userInput,
            alphaFlag;
}
L_GifGCEPack;//local


typedef struct
{/*Graphics Control Extension*/
    uint8_t         byteSize,
                    alphaColorIndex,
                    used;

    uint16_t        delayTime;

    L_GifGCEPack    packField;
}
L_GifGCE;//local


typedef struct
{/*Image Descriptor packed file, from Byte*/
    uint8_t     usesLocalColor,
                interlace,
                sorted,
                res;

    size_t      colorTabSize;
}
L_GifIDpack;//local


typedef struct
{/*Image Descriptor*/

    uint16_t     leftPos,
                 topPos,
                 width,
                 height;

    L_GifIDpack  imgPack;
}
L_GifID;//local


typedef struct
{
    L_GifGCE         control;
    L_GifID          descriptor;
    L_GifColorTable  localColor;
    uint8_t          *imageData;
}
L_GifImage;


typedef struct
{
    uint8_t     blockSize,
                subBlockSize;
    uint16_t    repeat;
    char        *text;
}
L_GifAppExt;//local unused


typedef struct
{
    unsigned int numOfBlocks;
    char         *text;

}
L_GifComExt;//local


typedef struct
{
    uint8_t rShift,
            gShift,
            bShift,
            aShift;
}
L_GifColorShift;//local unused


typedef struct
{/*gif file descriptor*/
    L_GifHeader       header;
    L_GifLSD          lsd;
    L_GifColorTable   globalColor;
    L_GifGCE          gce;
    L_GifImage        *image;
    L_GifComExt       comExt;
    L_GifColorShift   colorShift;
    unsigned int      imgNum;

}
L_GifFile;

/***Functions**/

/*create gif handler*/
L_GifFile *L_gifLoadRW(SDL_RWops* file);

/*free all gif structure allocations*/
void L_gifFileFree(L_GifFile*);


#ifdef __cplusplus
}
#endif

#endif // GIF_DEC_INCLUDED
