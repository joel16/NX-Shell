#ifndef NX_SHELL_AUDIO_OGG_H
#define NX_SHELL_AUDIO_OGG_H

int OGG_Init(const char *path);
u32 OGG_GetSampleRate(void);
u8 OGG_GetChannels(void);
void OGG_Decode(void *buf, unsigned int length, void *userdata);
u64 OGG_GetPosition(void);
u64 OGG_GetLength(void);
u64 OGG_Seek(u64 index);
void OGG_Term(void);

#endif
