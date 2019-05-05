#include <tremor/ivorbiscodec.h>
#include <tremor/ivorbisfile.h>
#include <stdio.h>

#include "audio.h"

static OggVorbis_File ogg;
static FILE *ogg_file;
static vorbis_info *ogg_info;
static ogg_int64_t samples_read = 0, max_lenth = 0;

int OGG_Init(const char *path) {
	if ((ogg_file = fopen(path, "rb")) == NULL)
		return -1;

	if (ov_open(ogg_file, &ogg, NULL, 0) < 0)
		return -1;

	if ((ogg_info = ov_info(&ogg, -1)) == NULL)
		return -1;

	max_lenth = ov_pcm_total(&ogg, -1);

	char *value = NULL;
	vorbis_comment *comment = ov_comment(&ogg, -1);
	if (comment != NULL) {
		metadata.has_meta = true;

		if ((value = vorbis_comment_query(comment, "title", 0)) != NULL)
			strcpy(metadata.title, value);

		if ((value = vorbis_comment_query(comment, "album", 0)) != NULL)
			strcpy(metadata.album, value);

		if ((value = vorbis_comment_query(comment, "artist", 0)) != NULL)
			strcpy(metadata.artist, value);

		if ((value = vorbis_comment_query(comment, "year", 0)) != NULL)
			strcpy(metadata.year, value);

		if ((value = vorbis_comment_query(comment, "comment", 0)) != NULL)
			strcpy(metadata.comment, value);

		if ((value = vorbis_comment_query(comment, "genre", 0)) != NULL)
			strcpy(metadata.genre, value);
	}

	return 0;
}

u32 OGG_GetSampleRate(void) {
	return ogg_info->rate;
}

u8 OGG_GetChannels(void) {
	return ogg_info->channels;
}

static u64 OGG_FillBuffer(char *out) {
	u64 samples_read = 0;
	int samples_to_read = (sizeof(s16) * ogg_info->channels) * 4096;

	while(samples_to_read > 0) {
		static int current_section;
		int samples_just_read = ov_read(&ogg, out, samples_to_read > 4096 ? 4096 : samples_to_read, &current_section);

		if (samples_just_read < 0)
			return samples_just_read;
		else if (samples_just_read == 0)
			break;

		samples_read += samples_just_read;
		samples_to_read -= samples_just_read;
		out += samples_just_read;
	}

	return samples_read / sizeof(s16);
}

void OGG_Decode(void *buf, unsigned int length, void *userdata) {
	OGG_FillBuffer((char *)buf);
	samples_read = ov_pcm_tell(&ogg);

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

	if (metadata.has_meta)
        metadata.has_meta = false;

	ov_clear(&ogg);
}
