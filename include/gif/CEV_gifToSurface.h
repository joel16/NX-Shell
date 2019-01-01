//**********************************************************/
//** Done by  |      Date     |  version |    comment     **/
//**------------------------------------------------------**/
//**   CEV    |    05-2016    |   1.0    |  creation/SDL2 **/
//**********************************************************/


#ifndef GIFTOSURFACE_H_INCLUDED
#define GIFTOSURFACE_H_INCLUDED

#include <SDL.h>
#include "CEV_gif.h"
#include "CEV_gifDeflate.h"

//#define METHOD_OVERWRITE 1
//#define METHOD_REDRAW 2

#ifdef __cplusplus
extern "C" {
#endif

/** \brief gif method
 */
typedef enum GIF_METHOD
{
    METHOD_OVERWRITE  = 1,
    METHOD_REDRAW     = 2
}
GIF_METHOD;


/** \brief animation inner frame structure
 */
typedef struct L_GifFrame
{
    uint8_t dispMethod,
            *pixels;

    uint16_t time;

    SDL_Rect pos;
}
L_GifFrame;//local


/** \brief animation inner surface infos
 */
typedef struct L_GifSurfaceMain
{
    SDL_Rect pos;
    SDL_Texture *surface;
}
L_GifSurfaceMain;


/** \brief animation parameters and status
 */
typedef struct L_GifInfo
{
    char *comment,
         signature[4],
         version[4],
         loopDone,
         direction,
         refresh;

    uint8_t loopMode;
    unsigned int time,
                timeAct;
    int         imgNum,
                imgAct;

}L_GifInfo;


/** \brief gif animation instance
 */
struct CEV_GifAnim
{
    L_GifInfo status;
    L_GifSurfaceMain display;
    L_GifFrame *pictures;

};


void L_gifFillSurface(uint8_t *pixels, L_GifFile* gif, int index);

void L_gifFillSurfaceInterlace(uint8_t *pixels, L_GifFile* gif, int index);

int L_gifColorToInt(L_GifColor color);

void L_gifTextureRedraw(const uint8_t*, const SDL_Rect* , SDL_Texture*);

void L_gifTextureOverlay(const uint8_t*, const SDL_Rect* , SDL_Texture*);

char L_gifPtIsInBox(int , int , const SDL_Rect*);

char L_gifBlit(CEV_GifAnim *anim);

void L_gifPicSelectNxt(CEV_GifAnim *anim);

char L_gifAddModulo(int mode,int* val,int num);

char L_gifAddLim(int mode, int *val, int num);

void L_gifFitBoxInto(SDL_Rect*, const SDL_Rect*);

char L_gifAnimInit(CEV_GifAnim*, L_GifFile*);

#ifdef __cplusplus
}
#endif

#endif // GIFTOSURFACE_H_INCLUDED
