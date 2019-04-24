#pragma once

int MP3_Init(const char *path);
u32 MP3_GetSampleRate(void);
u8 MP3_GetChannels(void);
void MP3_Decode(void *buf, unsigned int length, void *userdata);
u64 MP3_GetPosition(void);
u64 MP3_GetLength(void);
void MP3_Term(void);
