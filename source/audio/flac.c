#include <FLAC/metadata.h>

#include "audio.h"
#define DR_FLAC_IMPLEMENTATION
#include "dr_flac.h"
#include "SDL_helper.h"

static drflac *flac;
static drflac_uint64 frames_read = 0;

int FLAC_Init(const char *path) {
	flac = drflac_open_file(path, NULL);
	if (flac == NULL)
		return -1;

	FLAC__StreamMetadata *tags;
	if (FLAC__metadata_get_tags(path, &tags)) {
		for (int i = 0; i < tags->data.vorbis_comment.num_comments; i++)  {
			char *tag = (char *)tags->data.vorbis_comment.comments[i].entry;

			if (!strncasecmp("TITLE=", tag, 6)) {
				metadata.has_meta = true;
				snprintf(metadata.title, 31, "%s\n", tag + 6);
			}

			if (!strncasecmp("ALBUM=", tag, 6)) {
				metadata.has_meta = true;
				snprintf(metadata.album, 31, "%s\n", tag + 6);
			}

			if (!strncasecmp("ARTIST=", tag, 7)) {
				metadata.has_meta = true;
				snprintf(metadata.artist, 31, "%s\n", tag + 7);
			}

			if (!strncasecmp("DATE=", tag, 5)) {
				metadata.has_meta = true;
				snprintf(metadata.year, 31, "%d\n", atoi(tag + 5));
			}

			if (!strncasecmp("COMMENT=", tag, 8)) {
				metadata.has_meta = true;
				snprintf(metadata.comment, 31, "%s\n", tag + 8);
			}

			if (!strncasecmp("GENRE=", tag, 6)) {
				metadata.has_meta = true;
				snprintf(metadata.genre, 31, "%s\n", tag + 6);
			}
		}
	}

	if (tags)
		FLAC__metadata_object_delete(tags);

	FLAC__StreamMetadata *picture;
	if (FLAC__metadata_get_picture(path, &picture, FLAC__STREAM_METADATA_PICTURE_TYPE_FRONT_COVER, "image/jpg", NULL, (unsigned)(-1), (unsigned)(-1),
		(unsigned)(-1), (unsigned)(-1))) {
		metadata.has_meta = true;
		SDL_LoadImageMem(&metadata.cover_image, picture->data.picture.data, picture->length);
		FLAC__metadata_object_delete(picture);
	}
	else if (FLAC__metadata_get_picture(path, &picture, FLAC__STREAM_METADATA_PICTURE_TYPE_FRONT_COVER, "image/jpeg", NULL, (unsigned)(-1), (unsigned)(-1),
		(unsigned)(-1), (unsigned)(-1))) {
		metadata.has_meta = true;
		SDL_LoadImageMem(&metadata.cover_image, picture->data.picture.data, picture->length);
		FLAC__metadata_object_delete(picture);
	}
	else if (FLAC__metadata_get_picture(path, &picture, FLAC__STREAM_METADATA_PICTURE_TYPE_FRONT_COVER, "image/png", NULL, (unsigned)(-1), (unsigned)(-1),
		(unsigned)(-1), (unsigned)(-1))) {
		metadata.has_meta = true;
		SDL_LoadImageMem(&metadata.cover_image, picture->data.picture.data, picture->length);
		FLAC__metadata_object_delete(picture);
	}

	return 0;
}

u32 FLAC_GetSampleRate(void) {
	return flac->sampleRate;
}

u8 FLAC_GetChannels(void) {
	return flac->channels;
}

void FLAC_Decode(void *buf, unsigned int length, void *userdata) {
	frames_read += drflac_read_pcm_frames_s32(flac, length / (sizeof(int) * flac->channels), (drflac_int32 *)buf);
	
	if (frames_read == flac->totalPCMFrameCount)
		playing = false;
}

u64 FLAC_GetPosition(void) {
	return frames_read;
}

u64 FLAC_GetLength(void) {
	return flac->totalPCMFrameCount;
}

u64 FLAC_Seek(u64 index) {
	drflac_uint64 seek_frame = (flac->totalPCMFrameCount * (index / 640.0));
	
	if (drflac_seek_to_pcm_frame(flac, seek_frame) == DRFLAC_TRUE) {
		frames_read = seek_frame;
		return frames_read;
	}
	
	return -1;
}

void FLAC_Term(void) {
	frames_read = 0;

	if (metadata.has_meta) {
		metadata.has_meta = false;

		if (metadata.cover_image)
			SDL_DestroyTexture(metadata.cover_image);
	}

	drflac_close(flac);
}
