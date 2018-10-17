#ifndef NX_SHELL_AUDIO_MP3_H
#define NX_SHELL_AUDIO_MP3_H

typedef struct 
{
    char title[34];
    char album[34];
    char artist[34];
    char year[0x5];
    char comment[34];
    char genre[34];
} ID3_Tag;

ID3_Tag ID3;

int MP3_GetProgress(void);
void MP3_Init(char *path);
void MP3_Exit(void);

#endif