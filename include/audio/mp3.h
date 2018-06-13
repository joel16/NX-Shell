#ifndef NX_SHELL_AUDIO_MP3_H
#define NX_SHELL_AUDIO_MP3_H

typedef struct 
{
    char title[0x1F];
    char album[0x1F];
    char artist[0x1F];
    char year[0x5];
    char comment[0x1F];
    char genre[0x1F];
} ID3_Tag;

ID3_Tag ID3;

int MP3_GetProgress(void);
void MP3_Init(char *path);
void MP3_Exit(void);

#endif