#include "audio.h"
#define STB_VORBIS_HEADER_ONLY
#define STB_VORBIS_NO_PUSHDATA_API
#include "stb_vorbis.c"

static stb_vorbis *ogg;
static stb_vorbis_info ogg_info;
static unsigned int samples_read = 0, max_lenth = 0;

int OGG_Init(const char *path) {
	int error = 0;
	ogg = stb_vorbis_open_filename(path, &error, NULL);

	if (!ogg)
		return -1;

	ogg_info = stb_vorbis_get_info(ogg);
	max_lenth = stb_vorbis_stream_length_in_samples(ogg);
	return 0;
}

u32 OGG_GetSampleRate(void) {
	return ogg_info.sample_rate;
}

u8 OGG_GetChannels(void) {
	return ogg_info.channels;
}

void OGG_Decode(void *buf, unsigned int length, void *userdata) {
	samples_read += stb_vorbis_get_samples_short_interleaved(ogg, ogg_info.channels, (short *)buf, (int)(length / ogg_info.channels));

	if (samples_read == max_lenth)
		playing = false;
}

u64 OGG_GetPosition(void) {
	return samples_read;
}

u64 OGG_GetLength(void) {
	return max_lenth;
}

void OGG_Term(void) {
	samples_read = 0;
	stb_vorbis_close(ogg);
}
