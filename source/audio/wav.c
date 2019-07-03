#include "audio.h"
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

static drwav wav;
static drwav_uint64 frames_read = 0;

int WAV_Init(const char *path) {
	if (!drwav_init_file(&wav, path))
		return -1;

	return 0;
}

u32 WAV_GetSampleRate(void) {
	return wav.sampleRate;
}

u8 WAV_GetChannels(void) {
	return wav.channels;
}

void WAV_Decode(void *buf, unsigned int length, void *userdata) {
	frames_read += drwav_read_pcm_frames_s32(&wav, length / (sizeof(int) * wav.channels), (drwav_int32 *)buf);

	if (frames_read == wav.totalPCMFrameCount)
		playing = false;
}

u64 WAV_GetPosition(void) {
	return frames_read;
}

u64 WAV_GetLength(void) {
	return wav.totalPCMFrameCount;
}

u64 WAV_Seek(u64 index) {
	drwav_uint64 seek_frame = (wav.totalPCMFrameCount * (index / 640.0));
	
	if (drwav_seek_to_pcm_frame(&wav, seek_frame) == DRWAV_TRUE) {
		frames_read = seek_frame;
		return frames_read;
	}
	
	return -1;
}

void WAV_Term(void) {
	frames_read = 0;
	drwav_uninit(&wav);
}
