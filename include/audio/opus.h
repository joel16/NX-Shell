#pragma once

int OPUS_Init(const char *path);
u32 OPUS_GetSampleRate(void);
u8 OPUS_GetChannels(void);
void OPUS_Decode(void *buf, unsigned int length, void *userdata);
u64 OPUS_GetPosition(void);
u64 OPUS_GetLength(void);
void OPUS_Term(void);
