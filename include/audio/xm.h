#ifndef NX_SHELL_AUDIO_XM_H
#define NX_SHELL_AUDIO_XM_H

int XM_Init(const char *path);
u32 XM_GetSampleRate(void);
u8 XM_GetChannels(void);
void XM_Decode(void *buf, unsigned int length, void *userdata);
u64 XM_GetPosition(void);
u64 XM_GetLength(void);
u64 XM_Seek(u64 index);
void XM_Term(void);

#endif
