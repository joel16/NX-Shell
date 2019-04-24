#include "audio.h"
#define DR_FLAC_IMPLEMENTATION
#include "dr_flac.h"
#include "SDL_helper.h"

static drflac *flac;
static drflac_uint64 frames_read = 0;

static void FLAC_MetaCallback(void *pUserData, drflac_metadata *pMetadata) {
	if (pMetadata->type == DRFLAC_METADATA_BLOCK_TYPE_PICTURE) {
		if (pMetadata->data.picture.type == DRFLAC_PICTURE_TYPE_COVER_FRONT) {
			metadata.has_meta = true;

			if ((!strcasecmp(pMetadata->data.picture.mime, "image/jpg")) || (!strcasecmp(pMetadata->data.picture.mime, "image/jpeg"))
				|| (!strcasecmp(pMetadata->data.picture.mime, "image/png")))
				SDL_LoadImageMem(&metadata.cover_image, (drflac_uint8 *)pMetadata->data.picture.pPictureData, pMetadata->data.picture.pictureDataSize);
		}
	}
}

int FLAC_Init(const char *path) {
	flac = drflac_open_file_with_metadata(path, FLAC_MetaCallback, NULL);
	if (flac == NULL)
		return -1;

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

void FLAC_Term(void) {
	frames_read = 0;

	if (metadata.has_meta) {
		metadata.has_meta = false;

		if (metadata.cover_image)
			SDL_DestroyTexture(metadata.cover_image);
	}

	drflac_close(flac);
}
