#pragma once

int FLAC_Init(const char *path);
u32 FLAC_GetSampleRate(void);
u8 FLAC_GetChannels(void);
void FLAC_Decode(void *buf, unsigned int length, void *userdata);
u64 FLAC_GetPosition(void);
u64 FLAC_GetLength(void);
void FLAC_Term(void);
