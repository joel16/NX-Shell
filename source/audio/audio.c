#include <mpg123.h>
#include <stdio.h>
#include <string.h>

#include "audio.h"
#include "common.h"
#include "fs.h"

#include "flac.h"
#include "mp3.h"
#include "ogg.h"
#include "audio/opus.h"
#include "wav.h"
#include "xm.h"

enum Audio_FileType {
	FILE_TYPE_NONE,
	FILE_TYPE_FLAC,
	FILE_TYPE_MP3,
	FILE_TYPE_OGG,
	FILE_TYPE_OPUS,
	FILE_TYPE_WAV,
	FILE_TYPE_XM
};

typedef struct {
	int (* init)(const char *path);
	u32 (* rate)(void);
	u8 (* channels)(void);
	void (* decode)(void *buf, unsigned int length, void *userdata);
	u64 (* position)(void);
	u64 (* length)(void);
	u64 (* seek)(u64 index);
	void (* term)(void);
} Audio_Decoder;

bool playing = true, paused = false;
Audio_Metadata metadata = {0};
static Audio_Metadata empty_metadata = {0};
static Audio_Decoder decoder = {0}, empty_decoder = {0};
static enum Audio_FileType file_type = FILE_TYPE_NONE;
static SDL_AudioDeviceID audio_device;

static void Audio_Callback(void *userdata, Uint8 *stream, int length) {
	if (playing == false || paused == true)
		memset(stream, 0, length);
	else
		(* decoder.decode)(stream, length, userdata);
}

/// TODO: proper error checking
int Audio_Init(const char *path) {
	playing = true;
	paused = false;

	SDL_AudioSpec want, have;
	SDL_memset(&want, 0, sizeof(want));

	want.samples = 4096;

	if (!strncasecmp(FS_GetFileExt(path), "flac", 4))
		file_type = FILE_TYPE_FLAC;
	else if (!strncasecmp(FS_GetFileExt(path), "mp3", 3))
		file_type = FILE_TYPE_MP3;
	else if (!strncasecmp(FS_GetFileExt(path), "ogg", 3))
		file_type = FILE_TYPE_OGG;
	else if (!strncasecmp(FS_GetFileExt(path), "opus", 4))
		file_type = FILE_TYPE_OPUS;
	else if (!strncasecmp(FS_GetFileExt(path), "wav", 3))
		file_type = FILE_TYPE_WAV;
	else if ((!strncasecmp(FS_GetFileExt(path), "it", 2)) || (!strncasecmp(FS_GetFileExt(path), "mod", 3))
		|| (!strncasecmp(FS_GetFileExt(path), "s3m", 3)) || (!strncasecmp(FS_GetFileExt(path), "xm", 2)))
		file_type = FILE_TYPE_XM;

	switch(file_type) {
		case FILE_TYPE_FLAC:
			decoder.init = FLAC_Init;
			decoder.rate = FLAC_GetSampleRate;
			decoder.channels = FLAC_GetChannels;
			decoder.decode = FLAC_Decode;
			decoder.position = FLAC_GetPosition;
			decoder.length = FLAC_GetLength;
			decoder.seek = FLAC_Seek;
			decoder.term = FLAC_Term;
			want.format = AUDIO_S32;
			break;

		case FILE_TYPE_MP3:
			decoder.init = MP3_Init;
			decoder.rate = MP3_GetSampleRate;
			decoder.channels = MP3_GetChannels;
			decoder.decode = MP3_Decode;
			decoder.position = MP3_GetPosition;
			decoder.length = MP3_GetLength;
			decoder.seek = MP3_Seek;
			decoder.term = MP3_Term;
			want.format = AUDIO_S16;
			break;

		case FILE_TYPE_OGG:
			decoder.init = OGG_Init;
			decoder.rate = OGG_GetSampleRate;
			decoder.channels = OGG_GetChannels;
			decoder.decode = OGG_Decode;
			decoder.position = OGG_GetPosition;
			decoder.length = OGG_GetLength;
			decoder.seek = OGG_Seek;
			decoder.term = OGG_Term;
			want.format = AUDIO_S16;
			break;

		case FILE_TYPE_OPUS:
			decoder.init = OPUS_Init;
			decoder.rate = OPUS_GetSampleRate;
			decoder.channels = OPUS_GetChannels;
			decoder.decode = OPUS_Decode;
			decoder.position = OPUS_GetPosition;
			decoder.length = OPUS_GetLength;
			decoder.seek = OPUS_Seek;
			decoder.term = OPUS_Term;
			want.format = AUDIO_S16;
			want.samples = 960;
			break;

		case FILE_TYPE_WAV:
			decoder.init = WAV_Init;
			decoder.rate = WAV_GetSampleRate;
			decoder.channels = WAV_GetChannels;
			decoder.decode = WAV_Decode;
			decoder.position = WAV_GetPosition;
			decoder.length = WAV_GetLength;
			decoder.seek = WAV_Seek;
			decoder.term = WAV_Term;
			want.format = AUDIO_S32;
			break;

		case FILE_TYPE_XM:
			decoder.init = XM_Init;
			decoder.rate = XM_GetSampleRate;
			decoder.channels = XM_GetChannels;
			decoder.decode = XM_Decode;
			decoder.position = XM_GetPosition;
			decoder.length = XM_GetLength;
			decoder.seek = XM_Seek;
			decoder.term = XM_Term;
			want.format = AUDIO_S16;
			break;

		default:
			break;
	}

	(* decoder.init)(path);
	want.freq = (* decoder.rate)();
	want.channels = (* decoder.channels)();
	want.userdata = NULL;
	want.callback = Audio_Callback;
	
	audio_device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
	SDL_PauseAudioDevice(audio_device, paused);
	return 0;
}

bool Audio_IsPaused(void) {
	return paused;
}

void Audio_Pause(void) {
	paused = !paused;
	SDL_PauseAudioDevice(audio_device, paused);
}

void Audio_Stop(void) {
	playing = !playing;
}

u64 Audio_GetPosition(void) {
	return (* decoder.position)();
}

u64 Audio_GetLength(void) {
	return (* decoder.length)();
}

u64 Audio_GetPositionSeconds(void) {
	return (Audio_GetPosition() / (* decoder.rate)());
}

u64 Audio_GetLengthSeconds(void) {
	return (Audio_GetLength() / (* decoder.rate)());
}

u64 Audio_Seek(u64 index) {
	return (* decoder.seek)(index);
}

void Audio_Term(void) {
	playing = true;
	paused = false;

	SDL_PauseAudioDevice(audio_device, 1);
	SDL_CloseAudioDevice(audio_device);
	(* decoder.term)();

	// Clear metadata struct
	metadata = empty_metadata;
	decoder = empty_decoder;
}
