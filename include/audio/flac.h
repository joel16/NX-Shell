#ifndef NX_SHELL_AUDIO_FLAC_H
#define NX_SHELL_AUDIO_FLAC_H

int FLAC_Init(const char *path);
u32 FLAC_GetSampleRate(void);
u8 FLAC_GetChannels(void);
void FLAC_Decode(void *buf, unsigned int length, void *userdata);
u64 FLAC_GetPosition(void);
u64 FLAC_GetLength(void);
u64 FLAC_Seek(u64 index);
void FLAC_Term(void);

#endif
