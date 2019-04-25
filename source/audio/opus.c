#include <opus/opusfile.h>

#include "audio.h"

static OggOpusFile *opus;
static ogg_int64_t samples_read = 0, max_samples = 0;

int OPUS_Init(const char *path) {
	int error = 0;

	if ((opus = op_open_file(path, &error)) == NULL)
		return -1;

	if ((error = op_current_link(opus)) < 0)
		return -1;

	max_samples = op_pcm_total(opus, -1);

	return 0;
}

u32 OPUS_GetSampleRate(void) {
	return 48000;
}

u8 OPUS_GetChannels(void) {
	return 2;
}

void OPUS_Decode(void *buf, unsigned int length, void *userdata) {
	int read = op_read_stereo(opus, (opus_int16 *)buf, (int)length * (sizeof(s16) * 4));
	if (read)
		samples_read = op_pcm_tell(opus);

	if (samples_read == max_samples)
		playing = false;
}

u64 OPUS_GetPosition(void) {
	return samples_read;
}

u64 OPUS_GetLength(void) {
	return max_samples;
}

void OPUS_Term(void) {
	samples_read = 0;
	op_free(opus);
}
