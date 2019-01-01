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
#include "CEV_gifDeflate.h"
#include "CEV_gifToSurface.h"
#include "CEV_gif.h"




CEV_GifAnim * CEV_gifAnimLoad(const char* fileName, SDL_Renderer *renderer)
{/*direct gif animation load from gif file*/

        /*DEC**/

    CEV_GifAnim *result = NULL;

        /*EXE*/

    result = CEV_gifAnimLoadRW(SDL_RWFromFile(fileName, "rb"), renderer, 1);

    if (result == NULL)
        fprintf(stderr, "Err : SDL_GIFAnimLoad / Unable to extract animation from file\n");

        /*POST*/

    return result;
}



CEV_GifAnim * CEV_gifAnimLoadRW(SDL_RWops* rwops, SDL_Renderer *renderer, char freeSrc)
{/*gif animation load from SDL_RWops */

        /*DEC**/

    int i;
    uint8_t     *pixels     = NULL;
    L_GifFile   *gif        = NULL;
    CEV_GifAnim *anim       = NULL;
    SDL_Texture *newTexture = NULL;

        /*PRE**/

    if (rwops == NULL || renderer == NULL)
        return NULL;

    CEV_gifReadWriteErr = 0;

        /*EXE*/

    anim = malloc(sizeof(CEV_GifAnim));

    if (anim == NULL)
        goto err_exit;

    /*extracting datas from gif file*/
    gif = L_gifLoadRW(rwops);

    if(gif == NULL)
        goto err_1;

    /*creating main texture aka the user one*/
    newTexture =SDL_CreateTexture(renderer,
                                SDL_PIXELFORMAT_RGBA8888,
                                SDL_TEXTUREACCESS_STREAMING,
                                gif->lsd.width,
                                gif->lsd.height);


    if(newTexture == NULL)
    {
        fprintf(stderr,"Err / SDL_GIFAnimLoad : %s\n", SDL_GetError());
        goto err_2;
    }
    else
    {
        anim->display.surface   = newTexture;
        anim->display.pos.h     = gif->lsd.height;
        anim->display.pos.w     = gif->lsd.width;
        SDL_SetTextureBlendMode(newTexture, SDL_BLENDMODE_BLEND);
    }

    /*Starting to create every pic, one by one*/

    if (L_gifAnimInit(anim, gif))
        goto err_3;

    for (i=0; i<anim->status.imgNum; i++)
    {
        pixels = malloc(gif->image[i].descriptor.width * gif->image[i].descriptor.height * sizeof(uint32_t));

        if(pixels != NULL)
        {
            switch (gif->image[i].descriptor.imgPack.interlace)
            {
                case 0:
                    L_gifFillSurface(pixels, gif, i);
                break;

                case 1:
                    L_gifFillSurfaceInterlace(pixels, gif, i);
                break;

                default:
                    /* TODO (cedric#1): error management to add here */
                break;
            }
        }
        else
        {
            fprintf(stderr,"Err / SDL_GIFAnimLoad : %s\n", strerror(errno));
            goto err_3;
        }

        anim->pictures[i].pixels     = pixels;
        anim->pictures[i].pos.x      = gif->image[i].descriptor.leftPos;
        anim->pictures[i].pos.y      = gif->image[i].descriptor.topPos;
        anim->pictures[i].pos.h      = gif->image[i].descriptor.height;
        anim->pictures[i].pos.w      = gif->image[i].descriptor.width;
        anim->pictures[i].dispMethod = gif->image[i].control.packField.disposalMethod;
        anim->pictures[i].time       = gif->image[i].control.delayTime*10;

        /*get sure frame rects fits into surface rect*/
        L_gifFitBoxInto(&anim->pictures[i].pos, &anim->display.pos);
    }

    L_gifFileFree(gif); /*free raw gif datas*/

    /*be ready to display first logical frame in case of no call to SDL_GIFAnimAuto*/
    L_gifBlit(anim);

    if(CEV_gifReadWriteErr)/*well.. wait and see if it's a problem*/
        fprintf(stderr,"Warn / SDL_GIFAnimLoad : some R/W err has occured, file may be unstable\n");

        /*POST**/

    if (freeSrc)
        SDL_RWclose(rwops);

    return anim;


    /*error management from here**/
err_3 :
    CEV_gifAnimFree(anim);
    anim = NULL; /*safety for err_1*/

err_2 :
    L_gifFileFree(gif);

err_1 :
    free(anim);

err_exit :

    if(freeSrc)
        SDL_RWclose(rwops);

    return NULL;
}



char *CEV_gifComment(CEV_GifAnim *anim)
{/*returns embedded comment*/

    return anim->status.comment;
}



char *CEV_gifVersion(CEV_GifAnim *anim)
{/*returns embedded version*/

    return anim->status.version;
}



char *CEV_gifSignature(CEV_GifAnim *anim)
{/*returns embedded signature*/

    return anim->status.signature;
}



SDL_Texture *CEV_gifTexture(CEV_GifAnim *anim)
{/*returns User's texture**/

    return anim->display.surface;
}



int CEV_gifFrameNum(CEV_GifAnim *anim)
{/*returns num of pics*/

    return anim->status.imgNum;
}



void CEV_gifFrameNext(CEV_GifAnim *anim)
{/*forces next frame*/

    L_gifBlit(anim);
}



void CEV_gifTimeSet(CEV_GifAnim *anim, unsigned int index, uint16_t timeMs)
{/*sets animation speed*/

    int i;

    if ((index >= anim->status.imgNum) || index < GIF_ALL)
        return;/*nothing to be done*/

    else if (index >=0 )
        anim->pictures[index].time = timeMs;/*the index one only*/

    else/*all frames*/
        for(i=0; i<anim->status.imgNum; i++)
            anim->pictures[i].time = timeMs;
}



void CEV_gifLoopMode(CEV_GifAnim *anim, unsigned int loopMode)
{/*sets loop reading mode*/

    if(loopMode > GIF_STOP)
        return;/*nothing to be done*/

    anim->status.loopMode = loopMode;

    switch (loopMode)
    {
        case GIF_REPEAT_REV :/*modes starting from last frame*/
        case GIF_ONCE_REV :
            anim->status.imgAct = anim->status.imgNum-1;
        break;

        default:/*other modes*/
            anim->status.imgAct = 0;
        break;
    }

    anim->status.refresh = 1;               /*force refreshing*/
    anim->status.time    = SDL_GetTicks();  /*restart from now*/
}



void CEV_gifLoopReset(CEV_GifAnim *anim)
{/*resets loop animation**/

    switch (anim->status.loopMode)
    {
        case GIF_REPEAT_FOR :
        case GIF_ONCE_FOR :
        case GIF_FORTH_BACK:
            anim->status.imgAct = 0;
            anim->status.loopDone = 0;
        break;

        case GIF_REPEAT_REV :
        case GIF_ONCE_REV :
            anim->status.imgAct = anim->status.imgNum-1;
            anim->status.loopDone = 0;
        break;

        default:
        break;
    }

    anim->status.refresh = 1;               /*force refreshing*/
    anim->status.time    = SDL_GetTicks();  /*restart from now*/
}



char CEV_gifAnimAuto(CEV_GifAnim *anim)
{/*updates animations**/

    char sts = 0;

    unsigned int actTime,
                 now = SDL_GetTicks();

    int imgAct = anim->status.imgAct;

    if(!anim)
        return 0;

    /*if first call*/
    if(!anim->status.time)
        anim->status.time = now;

    /*since last time ?*/
    actTime = now - anim->status.time;

    if((actTime >= anim->pictures[imgAct].time) || anim->status.refresh)
    {/*it's time or it's forced by else function*/
        sts = L_gifBlit(anim);
        anim->status.time = now;
    }

    return sts;
}



void CEV_gifReverse(CEV_GifAnim *anim)
{/*reverses play mode*/

    switch (anim->status.loopMode)
    {
        case GIF_REPEAT_FOR :
             anim->status.loopMode = GIF_REPEAT_REV;
        break;

        case GIF_REPEAT_REV :
             anim->status.loopMode = GIF_REPEAT_FOR;
        break;

        case GIF_ONCE_FOR :
            anim->status.loopMode = GIF_ONCE_REV;
        break;

        case GIF_ONCE_REV :
            anim->status.loopMode = GIF_ONCE_FOR;
        break;

        case GIF_FORTH_BACK :
            anim->status.direction ^= 1;
        break;

        default:
        break;
    }
}



void CEV_gifAnimFree(CEV_GifAnim *anim)
{/*frees animation structure**/

    int i;

    if(!anim)
        return;

    for(i=0; i<anim->status.imgNum; i++)
        free(anim->pictures[i].pixels);     /*every pixels table*/

    free(anim->pictures);                   /*the frames table*/
    free(anim->status.comment);             /*comments if some*/
    SDL_DestroyTexture(anim->display.surface); /*main surface*/
    free(anim);                             /*and itself*/
}



char CEV_gifLoopStatus(CEV_GifAnim* anim)
{/*returns if loop is playing or not**/

    return !(anim->status.loopDone || (anim->status.loopMode==GIF_STOP));
}



char CEV_gifMethod(CEV_GifAnim* anim, unsigned int index)
{/*returns used method**/

    if(index>anim->status.imgNum-1)
        index = 0;

    return (char)anim->pictures[index].dispMethod;
}



void CEV_gifMethodSet(CEV_GifAnim* anim, int index, uint8_t method)
{/*sets method**/

    int i;

    if(index > anim->status.imgNum-1 || index < GIF_ALL)
        return;

    else if (index==GIF_ALL)
    {
        for(i=0; i<anim->status.imgNum; i++)
            anim->pictures[i].dispMethod = method;
    }
    else
        anim->pictures[index].dispMethod = method;
}
