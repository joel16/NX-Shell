//**********************************************************/
//** Done by  |      Date     |  version |    comment     **/
//**------------------------------------------------------**/
//**   CEV    |    05-2016    |   1.0    |  creation/SDL2 **/
//**********************************************************/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <SDL.h>
#include "CEV_gif.h"
#include "CEV_gifDeflate.h"
#include "CEV_gifToSurface.h"


/***Local Structures declarations****/

typedef struct L_GifDicoEntry
{/*dictionnary element*/
    int16_t prev;
    uint8_t value;
}
L_GifDicoEntry;//local


typedef struct L_GifDico
{/*full lzw dictionnary*/
    L_GifDicoEntry  entry[4096];
    uint16_t        actSize;
}
L_GifDico;//local


/***Local functions declaration****/

/*allocates and fills new structure*/
L_GifFile *L_gifCreate(void);//local

/*Header file init*/
void L_gifHeaderInit(L_GifHeader*);//local

/*Graphic Control Extension file init*/
void L_gifGceInit(L_GifGCE*);//local

/*Logical Screen Descriptor file init*/
void L_gifLsdInit(L_GifLSD*);//local

/*Comment Extension file init*/
void L_gifComExtInit(L_GifComExt*);//local

/*allocates and fills image information*/
void L_gifImgNewRW(L_GifFile*, SDL_RWops*);//local

/*reads raw data sublock and send it thru lzw decompression*/
void L_gifDataFillRW(L_GifFile*, SDL_RWops*);//local

/*gets header from file*/
int L_gifHeaderFillRW(L_GifHeader* , SDL_RWops*);//local

/*gets misc extension blocks*/
int L_gifExtFillRW(L_GifFile*, SDL_RWops*);//local

/*gets Comment Extension*/
int L_gifComExtFillRW(L_GifComExt*, SDL_RWops*);//local

/*gets Logical Screen Descriptor*/
int L_gifLsdFillRW(L_GifLSD*, SDL_RWops*);//local

/*gets Logical Screen Descriptor byte Pack*/
int L_gifLsdPackFillRW(L_GifLSDpack*, SDL_RWops*);//local

/*gets color table*/
int L_gifColorTabFillRW(unsigned int, L_GifColorTable*, SDL_RWops*);//local

/*gets Graphic Control Extension*/
int L_gifGceFillRW(L_GifGCE*, SDL_RWops*);//local

/*gets Graphic Control Extension byte Pack*/
int L_gifGcePackFillRW(L_GifGCEPack*, SDL_RWops*);//local

/*gets Image Descriptor*/
int L_gifIdFillRW(L_GifID*, SDL_RWops*);//local

/*gets Image Descriptor byte Pack*/
int L_gifIdPackFillRW(L_GifIDpack*, SDL_RWops*);//local

/*skips useless blocks*/
void L_gifBlockSkipRW(SDL_RWops*);//local

/*Performs LZW decompression*/
void L_gifLzw(void*, L_GifFile*, unsigned int);//local

/*low level 16 bit extraction*/
uint16_t L_gifGetBitFieldValue16(void*, unsigned int*, size_t);//local

/*dictionnary initialisation*/
void L_gifDicoInit(L_GifDico*, unsigned int);//local

/*dictionnary sub functions*/
uint8_t L_gifDicoGetFirstOfString(L_GifDico*, L_GifDicoEntry);//local
void L_gifDicoStringOutput(L_GifDico*, L_GifDicoEntry, uint8_t*, unsigned int*, unsigned int);//local
void L_gifDicoOutputRepeat(L_GifDico*, int16_t, uint8_t*, unsigned int*, unsigned int);//local
void L_gifStreamValueOutput(uint8_t, uint8_t*, unsigned int*, unsigned int);//local


/***Functions Implementation**/

L_GifFile *L_gifLoadRW(SDL_RWops* file)
{/*create gif handler*/

    L_GifFile *gif = NULL;

    unsigned char
                endOfFile=0;

    CEV_gifReadWriteErr = 0;

    /*creating new gif structure*/
    gif = L_gifCreate();

    if (gif == NULL)
    {/*on error*/
        fprintf(stderr,"Err / CEV_LoadGIFRW : unable to create gif structure.\n");
        goto err1;
    }

    L_gifHeaderFillRW(&gif->header, file);  /*Mandatory / function checked*/
    L_gifLsdFillRW(&gif->lsd, file);        /*Mandatory / function checked*/

    /*Global color table reading*/
    if (gif->lsd.packField.usesGlobalColor)
        L_gifColorTabFillRW(gif->lsd.packField.numOfColors, &gif->globalColor, file);    /*function checked*/

    while(!endOfFile && !CEV_gifReadWriteErr)
    {
        uint8_t dataType = SDL_ReadU8(file);

        switch(dataType)
        {/*switch next data type*/

            case 0x21 : /*file extensions*/
                L_gifExtFillRW(gif, file);
            break;

            case 0x2C : /*Image Descriptor*/
                L_gifImgNewRW(gif, file);
            break;

            case 0x3B :/*EOF*/
                endOfFile = 1;
            break;

            default :
                fprintf(stderr,"Err / CEV_LoadGifRW : Unexpected value :%d\n", dataType);
                CEV_gifReadWriteErr++;
            break;
        }
    }

err1:

    return gif;
}


int L_gifExtFillRW(L_GifFile *gif, SDL_RWops* file)
{/*fills misc extension blocks*/

    switch(SDL_ReadU8(file))
    {/*switch file extension type*/

        case 0xf9 : /*Graphics Control Extension*/
            L_gifGceFillRW(&gif->gce, file);/*vérifié*/
        break;

        /* TODO (cedric#1#): eventually to be taken into consideration but why ? */
        case 0x01 :/*Plain Text extension*/
        case 0xff :/*Application Extension*/
            L_gifBlockSkipRW(file);
        break;

        case 0xfe :/*Comment Extension*/
            L_gifComExtFillRW(&gif->comExt, file);
        break;

        default :
            fprintf(stderr,"Err / CEV_GifFillExt : Unknow file extension type has occured\n");
        break;
    }


    if(CEV_gifReadWriteErr)
        fprintf(stderr,"CEV_GifFillExt : W/R error occured.\n");

    return (CEV_gifReadWriteErr)? GIF_ERR : GIF_OK;
}


L_GifFile *L_gifCreate()
{/*allocates and fills new structure*/

    L_GifFile *newgif = NULL;

    newgif = malloc(sizeof(*newgif));

    if(newgif == NULL)
    {
        fprintf(stderr,"Err /CEV_GifFile Unable to malloc : %s\n", strerror(errno));
        return NULL;
    }

    /*initializing values & pointers*/
    newgif->imgNum                  = 0;
    newgif->image                   = NULL;
    newgif->globalColor.numOfColors = 0;
    newgif->globalColor.table       = NULL;

    /*init header*/
    L_gifHeaderInit(&newgif->header);

    /*init graphic control extension*/
    L_gifGceInit(&newgif->gce);

    /*init lsd*/
    L_gifLsdInit(&newgif->lsd);

    /*init comment extension*/
    L_gifComExtInit(&newgif->comExt);

    return newgif;
}


void L_gifHeaderInit(L_GifHeader *header)
{/*header file init*/

    memset(header->signature, 0, sizeof(header->signature));
    memset(header->version, 0, sizeof(header->version));
}


void L_gifGceInit(L_GifGCE *gce)
{/*gce file init*/

    gce->alphaColorIndex = 0;
    gce->byteSize        = 0;
    gce->delayTime       = 0;
    gce->used            = 0;

    gce->packField.alphaFlag      = 0;
    gce->packField.disposalMethod = 0;
    gce->packField.res            = 0;
    gce->packField.userInput      = 0;
}


void L_gifLsdInit (L_GifLSD *lsd)
{/*lsd file init*/

    lsd->bckgrdColorIndex = 0;
    lsd->height           = 0;
    lsd->pxlAspectRatio   = 0;
    lsd->width            = 0;

    lsd->packField.colorRes         = 0;
    lsd->packField.numOfColors      = 0;
    lsd->packField.sorted           = 0;
    lsd->packField.usesGlobalColor  = 0;
}


void L_gifComExtInit(L_GifComExt* comment)
{/*comment extension init*/

    comment->numOfBlocks = 0;
    comment->text        = NULL;
}


void L_gifImgNewRW(L_GifFile *gif, SDL_RWops* file)
{/*allocate and fills image information*/

    int frameIndex = gif->imgNum;/*rw purpose*/

    /*temporay image data*/
    L_GifImage *temp = NULL;

    /*one more frame to come*/
    gif->imgNum ++;

    temp = realloc(gif->image, gif->imgNum * sizeof(L_GifImage));

    if (temp != NULL)
        gif->image = temp;
    else
    {/*on error*/
        fprintf(stderr, "Err / L_gifImgNewRW : unable to allocate :%s\n", strerror(errno));
        gif->imgNum --;
        return;
    }

    if(gif->gce.used)
    {/*if graphic Control Extension was read before*/
        gif->image[frameIndex].control = gif->gce; /*store into new picture*/
    }


    /*fills Image Descriptor*/
    L_gifIdFillRW(&gif->image[frameIndex].descriptor, file);



    if (gif->image[frameIndex].descriptor.imgPack.usesLocalColor)
    {/*if there is a local color table*/
        L_gifColorTabFillRW(gif->image[frameIndex].descriptor.imgPack.colorTabSize, &gif->image[frameIndex].localColor, file);
    }
    else
    {/*default values*/
        gif->image[frameIndex].localColor.numOfColors    = 0;
        gif->image[frameIndex].localColor.table          = NULL;
    }

    /*restores raw data into pixels table*/
    L_gifDataFillRW(gif, file);
}


void L_gifFileFree(L_GifFile *gif)
{/*free all gif structure allocations*/

    int i;

    for(i=0; i<gif->imgNum; i++)
    {
        free(gif->image[i].localColor.table);
        free(gif->image[i].imageData);
    }

    if(gif->globalColor.table != NULL)
        free(gif->globalColor.table);

    free(gif->image);

    free(gif);
}


void L_gifDataFillRW(L_GifFile *gif, SDL_RWops* file)
{/*read raw data sublock and send it thru lzw decompression*/

    uint8_t
            *rawData    = NULL, /*data field only*/
            *temp       = NULL; /*temporary*/

    unsigned char
                LZWminiCodeSize = 0, /*min code size*/
                subBlockSize    = 0; /*size of data sub block*/

    size_t rawDataSize = 0; /*raw data full size in bytes*/

    LZWminiCodeSize = SDL_ReadU8(file);

    while((subBlockSize = SDL_ReadU8(file))) /*a size of 0 means no data to follow*/
    {/*fill rawData with subBlocks datas*/

        temp = realloc(rawData, rawDataSize + subBlockSize);/*alloc/realloc volume for new datas*/

        if (temp != NULL)/*realloc is ok*/
            rawData = temp;
        else
        {/*on error*/
            fprintf(stderr, "Err / CEV_fillImgData : realloc error. %s\n", strerror(errno));
            goto err;
        }

        /*fetch datas from SDL_RWops*/
        SDL_RWread(file, rawData+rawDataSize, sizeof(uint8_t), subBlockSize);

        rawDataSize +=  subBlockSize;   /*increase datasize by blocksize*/
    }

    /*pass data through lzw deflate*/
    L_gifLzw(rawData, gif, LZWminiCodeSize);

err :
    free(rawData);
}


int L_gifHeaderFillRW(L_GifHeader* header, SDL_RWops* file)
{/*fills header**/

    int i;

    for(i=0; i<3; i++)
        header->signature[i] = SDL_ReadU8(file);

    for(i=0; i<3; i++)
        header->version[i] = SDL_ReadU8(file);


    header->signature[3] = header->version[3] = '\0';

    if(CEV_gifReadWriteErr)
        fprintf(stderr,"R/W error while filling Header.\n");

    return (CEV_gifReadWriteErr)? GIF_ERR : GIF_OK;
}


int L_gifComExtFillRW(L_GifComExt *ext, SDL_RWops* file)
{/*fills comment extension*/

    uint8_t temp;

    temp = SDL_ReadU8(file);
    temp+=1;

    ext->text = malloc(temp * sizeof(char));
    if(ext->text == NULL)
    {
        fprintf(stderr, "err / L_gifComExtFillRW : %s\n", strerror(errno));
        return GIF_ERR;
    }

    if(SDL_RWread(file, ext->text, 1, temp) != temp)

        CEV_gifReadWriteErr++;

    /*debug supprimé ici*/
    /*ext->text[temp-1]='\0';*/

    if(CEV_gifReadWriteErr)
        fprintf(stderr,"R/W error while filling Comment Extension.\n");

    return (CEV_gifReadWriteErr)? GIF_ERR : GIF_OK;
}


int L_gifLsdFillRW(L_GifLSD* lsd, SDL_RWops* file)
{/*fills Logical Screen Descriptor*/

    uint8_t temp;

    lsd->width  = SDL_ReadLE16(file);
    lsd->height = SDL_ReadLE16(file);

    L_gifLsdPackFillRW(&lsd->packField, file);

/* TODO (cedric#1#): background color unused ? W8&C if it ever becomes a problem */
    lsd->bckgrdColorIndex   = SDL_ReadU8(file);
    temp                    = SDL_ReadU8(file);/*useless info*/
    lsd->pxlAspectRatio     = (temp + 15) / 64;

    if(CEV_gifReadWriteErr)
        fprintf(stderr,"R/W error while filling Logical Screen Descriptor.\n");

    return (CEV_gifReadWriteErr)? GIF_ERR : GIF_OK;
}


int L_gifLsdPackFillRW(L_GifLSDpack* pack, SDL_RWops* file)
{/*extract LSD's packed flags*/

    uint8_t temp = SDL_ReadU8(file);

    pack->usesGlobalColor   = (temp & 0x80)? 1 :0 ;
    pack->colorRes          = ((temp & 0x70)>>4) + 1;
    pack->sorted            = (temp & 0x08)? 1: 0;
    pack->numOfColors       = 1 << ((temp & 0x07)+1);

    if(CEV_gifReadWriteErr)
        fprintf(stderr,"R/W error while filling Logical Screen Descriptor Pack.\n");

    return (CEV_gifReadWriteErr)? GIF_ERR : GIF_OK;
}


int L_gifColorTabFillRW(unsigned int numOfColor, L_GifColorTable *colors, SDL_RWops* file)
{/*fills color table*/

    int i;

    colors->table = malloc(numOfColor*sizeof(L_GifColor));

    if (colors->table == NULL)
    {
        fprintf(stderr, "Err CEV_fillColorTab : unable to allocate memory :%s\n", strerror(errno));
        return GIF_ERR;
    }

    colors->numOfColors = numOfColor;

    for(i= 0; i<numOfColor; i++)
    {
        colors->table[i].r = SDL_ReadU8(file);
        colors->table[i].g = SDL_ReadU8(file);
        colors->table[i].b = SDL_ReadU8(file);
        colors->table[i].a = 0xff;
    }

    if(CEV_gifReadWriteErr)
        fprintf(stderr,"R/W error while filling Color table.\n");

    return (CEV_gifReadWriteErr)? GIF_ERR : GIF_OK;
}


int L_gifGceFillRW(L_GifGCE* gce, SDL_RWops* file)
{/*fills Graphic Control Extension*/

    gce->byteSize         = SDL_ReadU8(file);
    L_gifGcePackFillRW(&gce->packField, file);
    gce->delayTime        = SDL_ReadLE16(file);
    gce->alphaColorIndex  = SDL_ReadU8(file);
    SDL_ReadU8(file); /*read garbage byte*/
    gce->used = 1;

    if(CEV_gifReadWriteErr)
        fprintf(stderr,"R/W error while filling Graphic Contol Extension._n");

    return (CEV_gifReadWriteErr)? GIF_ERR : GIF_OK;
}


int L_gifGcePackFillRW(L_GifGCEPack* pack, SDL_RWops* file)
{/*extract Graphic Control Extension's packed flags*/

    uint8_t temp;

    temp = SDL_ReadU8(file);

    pack->res               = (temp & 0xE0)>>5;
    pack->disposalMethod    = (temp & 0x1C)>>2;
    pack->userInput         = (temp & 0x02)>>1;
    pack->alphaFlag         = temp & 0x01;

    if(CEV_gifReadWriteErr)
        fprintf(stderr,"R/W error while filling Graphic Contol Extension Pack.\n");

    return (CEV_gifReadWriteErr)? GIF_ERR : GIF_OK;
}


int L_gifIdFillRW(L_GifID* gifid, SDL_RWops* file)
{/*fills Image Descriptor*/

    gifid->leftPos   = SDL_ReadLE16(file);
    gifid->topPos    = SDL_ReadLE16(file);
    gifid->width     = SDL_ReadLE16(file);
    gifid->height    = SDL_ReadLE16(file);

    L_gifIdPackFillRW(&gifid->imgPack, file);

    if(CEV_gifReadWriteErr)
        fprintf(stderr,"R/W error while filling Image Descriptor.\n");

    return (CEV_gifReadWriteErr)? GIF_ERR : GIF_OK;
}


int L_gifIdPackFillRW(L_GifIDpack* IDpack, SDL_RWops* file)
{/*extract Image Descriptor's packed flag*/

    uint8_t temp;

    temp = SDL_ReadU8(file);

    IDpack->usesLocalColor  = (temp & 0x80)? 1 : 0;
    IDpack->interlace       = (temp & 0x40)? 1 : 0;
    IDpack->sorted          = (temp & 0x20)? 1 : 0;
    IDpack->res             = (temp & 0x18)? 1 : 0;
    IDpack->colorTabSize    = 1 << ((temp & 0x07)+1);

    if(CEV_gifReadWriteErr)
        fprintf(stderr,"R/W error while filling Image Descriptor Pack.\n");

    return (CEV_gifReadWriteErr)? GIF_ERR : GIF_OK;
}


void L_gifBlockSkipRW(SDL_RWops* file)
{/*skip useless subBlock in SDL_RWops*/

    uint8_t temp;

    while ((temp = (SDL_ReadU8(file))))
        SDL_RWseek(file, temp, RW_SEEK_CUR);
}


void L_gifLzw(void *codeStream, L_GifFile *gif, unsigned int lzwMinCode)
{/*LZW Decompression*/

    /*** DECLARATIONS ***/

    unsigned int bitPtr     = 0,   /*actual bit position in codestream*/
                 count      = 0,   /*count of index output*/
                 indexNum   = gif->image[gif->imgNum-1].descriptor.width * gif->image[gif->imgNum-1].descriptor.height;

    uint8_t *indexStream    =  NULL; /*result*/

    size_t maskSize = lzwMinCode +1; /*size of mask used to read bitstream in code Stream*/

    uint16_t
            actCodeValue    = 0,/*actual code value*/
            prevCodeValue   = 0;/*previous code value*/

    const uint16_t
            resetCode       = 1<<lzwMinCode,
            EOICode         = resetCode + 1;

    L_GifDico codeTable;
    L_GifDicoEntry prevString;

    /*** PRL ***/

    indexStream = malloc(indexNum *sizeof(uint8_t));

    if(indexStream == NULL)
    {
        fprintf(stderr, "gifLZW : unable to allocate indexStream : %s\n", strerror(errno));
        return;
    }

    /*** EXECUTION ***/

    while(count<indexNum)
    {/*starts loop*/

        /*read value in code stream*/
        actCodeValue  = L_gifGetBitFieldValue16(codeStream, &bitPtr, maskSize);

        if(actCodeValue == resetCode)
        {/*reinit dico if reset code*/

            L_gifDicoInit(&codeTable, resetCode); /*CodeTable init*/
            maskSize    = lzwMinCode +1;            /*maskSize init*/

            actCodeValue =  L_gifGetBitFieldValue16(codeStream, &bitPtr, maskSize);

            prevString = codeTable.entry[actCodeValue];
            prevCodeValue = actCodeValue;
            L_gifStreamValueOutput(actCodeValue, indexStream, &count, indexNum);

            continue;
        }

        if(actCodeValue == EOICode)
        {
            fprintf(stderr,"received EOI at %d; should be %d\n", count, indexNum);
            break;
        }

        if(actCodeValue < codeTable.actSize)
        {/**is CODE in the code table? Yes:**/

            uint8_t string0 = L_gifDicoGetFirstOfString(&codeTable, codeTable.entry[actCodeValue]);
            /*output {CODE} to index stream */
            L_gifDicoStringOutput(&codeTable, codeTable.entry[actCodeValue], indexStream, &count, indexNum);

            codeTable.entry[codeTable.actSize].prev  = prevCodeValue;
            codeTable.entry[codeTable.actSize].value = string0;

            prevCodeValue = actCodeValue;

            prevString = codeTable.entry[actCodeValue];
            codeTable.actSize++;
        }
        else
        {/**is CODE in the code table? No:**/

            uint8_t prevString0 = L_gifDicoGetFirstOfString(&codeTable, prevString);

            L_gifDicoStringOutput(&codeTable, prevString, indexStream, &count, indexNum);
            L_gifStreamValueOutput(prevString0, indexStream, &count, indexNum);

            codeTable.entry[codeTable.actSize].prev = prevCodeValue;
            codeTable.entry[codeTable.actSize].value = prevString0;
            prevCodeValue   = codeTable.actSize;
            prevString      = codeTable.entry[codeTable.actSize];
            codeTable.actSize++;
        }

        if (codeTable.actSize == (1<<(maskSize)))
        {/*if index reaches max value for actMask*/

            maskSize++; /*maskSize increase*/

            if (maskSize>=13) /*max gif mask is 12 bits by spec*/
                maskSize = 12;/*limited to 12*/
        }
    }

    /** POST **/

    /*image data =  index stream*/
    gif->image[gif->imgNum-1].imageData = indexStream;
}


uint16_t L_gifGetBitFieldValue16(void *data, unsigned int *bitStart, size_t maskSize)
{/*16 bits value extraction*/

    uint32_t value  = 0,
             mask   = (1<<maskSize) - 1;

    uint8_t* ptr    = data;

    if (maskSize > 16 || !data)
        return 0;

    ptr +=(*bitStart) /8;

    value = ((*(uint32_t*)ptr) >> ((*bitStart)%8)) & mask;

    (*bitStart) += maskSize ;

    return (uint16_t)value;
}


void L_gifDicoInit(L_GifDico * dico, unsigned int code)
{/*init dictionnary*/
    int i;

    dico->actSize = code+2;

    for(i=0; i< code; i++)
    {
        dico->entry[i].prev  = -1;
        dico->entry[i].value = (uint8_t)i;
    }
}


uint8_t L_gifDicoGetFirstOfString(L_GifDico *dico, L_GifDicoEntry entry)
{
    int16_t index = entry.prev ;

    if (index == -1)
        return entry.value;

    while(dico->entry[index].prev != -1)
        index = dico->entry[index].prev;

    return dico->entry[index].value;
}


void L_gifDicoStringOutput(L_GifDico *dico, L_GifDicoEntry string, uint8_t *out, unsigned int *count, unsigned int maxCount)
{
    L_gifDicoOutputRepeat(dico, string.prev, out, count, maxCount);
    L_gifStreamValueOutput(string.value, out, count, maxCount);
}


void L_gifDicoOutputRepeat(L_GifDico *dico, int16_t index, uint8_t *out, unsigned int *count, unsigned int maxCount)
{
    if(index==-1)
        return ;

    L_gifDicoOutputRepeat(dico, dico->entry[index].prev, out, count, maxCount);
    L_gifStreamValueOutput(dico->entry[index].value, out, count, maxCount);
    return ;
}


void L_gifStreamValueOutput(uint8_t value, uint8_t *out, unsigned int *count, unsigned int maxCount)
{
    if (*count == maxCount)
        return;

    out[(*count)++] = value;

    return;
}
