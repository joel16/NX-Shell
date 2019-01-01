//**********************************************************/
//** Done by  |      Date     |  version |    comment     **/
//**------------------------------------------------------**/
//**   CEV    |    05-2016    |   1.0    |  creation/SDL2 **/
//**********************************************************/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <SDL.h>
#include "CEV_gifToSurface.h"
#include "CEV_gif.h"



void L_gifFillSurface(uint8_t *pixels, L_GifFile* gif, int index)
{/*fills pixel fields / consecutive*/

    L_GifImage imageAct = gif->image[index]; /*easy rw*/

    uint32_t *ptr = (uint32_t*)pixels;

    L_GifColor *colorTable    = NULL;

    uint8_t  alphaColor = imageAct.control.alphaColorIndex;

    int i;

    unsigned int height,/*easy rw*/
                 width; /*easy rw*/

    switch (imageAct.descriptor.imgPack.usesLocalColor)
    {/*selecting which color table to use*/
        case 0 :/*global*/
            colorTable = gif->globalColor.table;
        break;

        case 1 :/*local*/
            colorTable = imageAct.localColor.table;
        break;

        default :/*shits happen..*/
            fprintf(stderr,"Err/ GifFillSurface : color table undefined.\n");
            return;
        break;
    }

    height  = imageAct.descriptor.height;/*rw purpose*/
    width   = imageAct.descriptor.width; /*rw purpose*/

    for (i = 0 ; i< height * width ; i++)
    {
       L_GifColor tempColor = colorTable[imageAct.imageData[i]]; /*used for easy rw*/

            if (imageAct.control.packField.alphaFlag && (alphaColor == imageAct.imageData[i]))
            {/*if color has index designed to be alpha*/
               tempColor = (L_GifColor){0, 0, 0, 0};
            }

            *(ptr++) = L_gifColorToInt(tempColor);
    }
}


void L_gifFillSurfaceInterlace(uint8_t *pixels, L_GifFile* gif, int index)
{/*fills pixel field / interlaced*/

/** interlace method values :

    pass1   = (height+7)/8,             temp = 8*i
    pass2   = pass1 + (height+3)/8,     temp = 8*i + 4
    pass3   = pass2 + (height+1)/4,     temp = 4*i + 2
    pass4   = pass3 + (height)/2        temp = 2*i + 1
*/

    L_GifImage imageAct = gif->image[index];/*used for easy rw*/

    uint32_t *ptr = (uint32_t*)pixels;

    uint8_t  alphaColor = imageAct.control.alphaColorIndex;

    L_GifColor *ColorTable;

    int i, j, pass;

    unsigned int height  = imageAct.descriptor.height, /*used for easy rw*/
                 width   = imageAct.descriptor.width;  /*used for easy rw*/

    switch (imageAct.descriptor.imgPack.usesLocalColor)
    {/*selecting which color table to use*/
        case 0 :/*global*/
            ColorTable = gif->globalColor.table;
        break;

        case 1 :/*local*/
            ColorTable = gif->image[index].localColor.table;
        break;

        default :/*shits happen...*/
            fprintf(stderr,"Err/ In function GifFillSurfaceInterlace : color table undefined.\n");
            return;
        break;
    }

    int factor = 8, tempAdd = 8,
        passMin = 0, passMax, passAdd = 7;

    for (pass=0; pass<4; pass++)
    {
        passMax = (passMin + (height + passAdd)/factor);

        for (i=passMin ; i<passMax ; i++)
        {
            int temp = factor*(i-passMin) + (tempAdd * (pass>0));

            for (j=0 ; j<width; j++)
            {
                L_GifColor tempColor = ColorTable[imageAct.imageData[i*width+j]];

                /*if color index is to be alpha*/
                if (imageAct.control.packField.alphaFlag && (alphaColor == imageAct.imageData[i*width+j]))
                    tempColor.a = 0;

                else// TODO (drxvd#1#02/17/17): added 17/02/17
                    tempColor.a = 255;

                ptr[temp*width +j]= L_gifColorToInt(tempColor);
            }
        }
        passMin = passMax;
        tempAdd/=2;
        passAdd/=2;
        factor /=(pass)? 2 : 1;
    }
}


int L_gifColorToInt(L_GifColor color)
{/*color struct to int*/

    //SDL1_version here under
    //return 0U | color.r<<format->rShift | color.g<<format->gShift | color.b<<format->bShift | color.a<<format->aShift;

    return color.r<<24 | color.g<<16 | color.b<<8 | color.a;
}


void L_gifTextureRedraw(const uint8_t *pixels, const SDL_Rect* blitPos, SDL_Texture *dstTex)
{/*redraw method, anything off the frame rect is made full alpha*/

    int x, y,
        width,  /*rw purpose*/
        height, /*rw purpose*/
        pitch;  /*rw purpose*/

    void *access;

    uint32_t *src = (uint32_t*)pixels,
             *dst;

    SDL_QueryTexture(dstTex, NULL, NULL, &width, &height);

    SDL_LockTexture(dstTex, NULL, &access, &pitch); /*lock for pxl access*/

    dst = (uint32_t*)access;

    /* TODO (cedric#1#): could be done linearly with 1 loop, but not so understandable, W8&C if slow */
    for(y =0; y < height; y++)
    {
        for(x =0; x < width; x++)
        {
            dst[y*pitch/sizeof(uint32_t) + x] = (L_gifPtIsInBox(x, y, blitPos))? src[(y-blitPos->y)*blitPos->w + x-blitPos->x] : (uint32_t)0x0;
        }
    }

    SDL_UnlockTexture(dstTex); /*unlock texture*/
}


void L_gifTextureOverlay(const uint8_t *pixels, const SDL_Rect* blitPos, SDL_Texture *dstTex)
{/*overlay method, only frame rect is copied to surface*/

    int i, j,
        pitch; /*rw purpose*/

    uint32_t *src = (uint32_t*)pixels,
             *dst;

    void* access;
    /***Error handeling to do**/

    SDL_LockTexture(dstTex, NULL, &access, &pitch);

    dst = (uint32_t*)access;

    for(i=0 ; i<blitPos->h; i++)
    {
        for(j=0 ; j< blitPos->w; j++)
        {
            int dstX = blitPos->x + j,/*rw purpose*/
                dstY = blitPos->y + i;/*rw purpose*/

            uint32_t color  = src[i*blitPos->w + j];/*rw purpose*/

            if(COLOR_A(color))
                dst[dstY*pitch/sizeof(uint32_t) + dstX] = color;
        }
    }

    SDL_UnlockTexture(dstTex);
}


char L_gifPtIsInBox(int x, int y, const SDL_Rect *rect)
{/*is this point in this Rect ?*/
    return ((x >= rect->x)
            && (x < (rect->x + rect->w))
            && (y >= rect->y)
            && (y <(rect->y+rect->h)));
}



char L_gifBlit(CEV_GifAnim *anim)
{/*redraw main texture*/

    if(!anim->status.refresh)/*if not refresh only*/
        L_gifPicSelectNxt(anim); /*select next frame*/

    int imgAct      = anim->status.imgAct;              /*rw purpose*/
    char dispMethod = anim->pictures[imgAct].dispMethod;/*rw purpose*/

    switch (dispMethod)
    {/*select disposal method*/

        default:/*default uses redraw, tests make it more appropriate, deal with it...*/

        case METHOD_REDRAW : /*redraws full surface*/
            L_gifTextureRedraw(anim->pictures[imgAct].pixels, &anim->pictures[imgAct].pos, anim->display.surface);
        break;

        case METHOD_OVERWRITE : /*overwrites frame surface*/
            L_gifTextureOverlay(anim->pictures[imgAct].pixels, &anim->pictures[imgAct].pos, anim->display.surface);
        break;
    }
    anim->status.refresh = 0;

    return(1);
}



char L_gifAddModulo(int mode,int* val,int num)
{/*incrément / décrément de val par modulo***VALIDE***/

    char result = 0;

    switch (mode)
    {
        case 0 : /*incremental*/
            if ((*val+1)<num)
                *val +=1;
            else
            {
                *val   = 0;
                result = 1;/*modulo reached*/
            }
        break;

        case 1 : /*decremental*/
            if (*val-1 >= 0)
                *val -=1;
            else
            {
                *val   = num-1;
                result = 1;/*modulo reached*/
            }
        break;

        default :/*avoid warning, unlikely*/
        break;
    }

    return result;
}


char L_gifAddLim(int mode, int *val, int num)
{/*incrément / décrément de val par limite***VALIDE***/
    switch (mode)
    {
        case 0 :/*incremental*/
            if (*val+1 < num)
                *val += 1;
        break;

        case 1 :/*decremental*/
            if (*val-1 >=0)
                *val -= 1;
        break;
    }

    return (char)(*val==0 || *val==num-1);/*limit reached*/
}


void L_gifPicSelectNxt(CEV_GifAnim *anim)
{/*selects next frame to be displayed*/

    int *imgAct = &anim->status.imgAct;

    switch (anim->status.loopMode)
    {
        case GIF_ONCE_FOR :

            if(!anim->status.loopDone && L_gifAddLim(0, imgAct, anim->status.imgNum))
                anim->status.loopDone = 1;
        break;

        case GIF_REPEAT_FOR :
            L_gifAddModulo(0, imgAct, anim->status.imgNum);

        break;

        case GIF_ONCE_REV :

            if(!anim->status.loopDone && L_gifAddLim(1, imgAct, anim->status.imgNum))
                anim->status.loopDone = 1;
        break;

        case GIF_REPEAT_REV :
            L_gifAddModulo(1, imgAct, anim->status.imgNum);
        break;

        case GIF_FORTH_BACK :
            switch(anim->status.direction)
            {
                case GIF_FORWARD :
                    if(*imgAct+1 == (anim->status.imgNum-1))
                        anim->status.direction = GIF_BACKWARD;

                    if (*imgAct+1 <= (anim->status.imgNum-1))
                        *imgAct += 1;

                break;

                case GIF_BACKWARD :
                    if(*imgAct-1 == 0)
                        anim->status.direction = GIF_FORWARD;

                    if (*imgAct-1 >= 0)
                        *imgAct -= 1;
                break;
            }
        break;

        case GIF_STOP :
            *imgAct = 0;
        break;

        default :
        break;

    }

    return;
}


void L_gifFitBoxInto(SDL_Rect *adapt , const SDL_Rect *ref)
{/*fits adapt into ref*/
    if (adapt->x <0)
        adapt->x = 0;

    if (adapt->x + adapt->w > ref->w)
        adapt->w = ref->w - adapt->x;

    if (adapt->y <0)
        adapt->y = 0;

    if(adapt->y+adapt->h > ref->h)
        adapt->h = ref->h-adapt->y;
}


char L_gifAnimInit(CEV_GifAnim * anim, L_GifFile* gif)
{/*animation initialization*/

    /*filling animation informations from gif file*/
    anim->pictures          = NULL;
    anim->status.imgNum     = gif->imgNum;
    anim->status.comment    = gif->comExt.text;
    anim->status.imgAct     = 0;
    anim->status.time       = 0;
    anim->status.loopMode   = GIF_STOP;
    anim->status.loopDone   = 0;
    anim->status.refresh    = 0;
    anim->status.direction  = GIF_FORWARD;

    strcpy(anim->status.signature, gif->header.signature);
    strcpy(anim->status.version, gif->header.version);

    /*allocating frames table*/
    anim->pictures = malloc(anim->status.imgNum * sizeof(L_GifFrame));

    if (!anim->pictures)
    {/*on error*/
        fprintf(stderr,"Err / L_gifAnimInit : Unable to allocate :%s\n",strerror(errno));
        return GIF_ERR;
    }

    for(int i=0; i<anim->status.imgNum; i++)
    {
        anim->pictures[i].dispMethod    = 0;
        anim->pictures[i].pixels        = NULL;
        anim->pictures[i].pos           = (SDL_Rect){0, 0, 0, 0};
        anim->pictures[i].time          = 0;
    }

    return GIF_OK;
}




