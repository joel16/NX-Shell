#include <mpg123.h>
#include <stdio.h>
#include <string.h>

#include "audio.h"
#include "common.h"
#include "fs.h"
#include "SDL_helper.h"

#define DR_FLAC_IMPLEMENTATION
#include "dr_flac.h"
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"
#define JAR_XM_IMPLEMENTATION
#include "jar_xm.h"
#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"

enum Audio_FileType {
	FILE_TYPE_NONE = 0,
	FILE_TYPE_FLAC = 1,
	FILE_TYPE_MP3 = 2,
	FILE_TYPE_OGG = 3,
	FILE_TYPE_WAV = 4,
	FILE_TYPE_XM = 5
};

// For MP3 ID3 tags
struct genre {
	int code;
	char text[112];
};

// For MP3 ID3 tags
struct genre genreList[] = {
	{0 , "Blues"}, {1 , "Classic Rock"}, {2 , "Country"}, {3 , "Dance"}, {4 , "Disco"}, {5 , "Funk"}, {6 , "Grunge"}, {7 , "Hip-Hop"}, {8 , "Jazz"}, {9 , "Metal"}, {10 , "New Age"},
	{11 , "Oldies"}, {12 , "Other"}, {13 , "Pop"}, {14 , "R&B"}, {15 , "Rap"}, {16 , "Reggae"}, {17 , "Rock"}, {18 , "Techno"}, {19 , "Industrial"}, {20 , "Alternative"},
	{21 , "Ska"}, {22 , "Death Metal"}, {23 , "Pranks"}, {24 , "Soundtrack"}, {25 , "Euro-Techno"}, {26 , "Ambient"}, {27 , "Trip-Hop"}, {28 , "Vocal"}, {29 , "Jazz+Funk"}, {30 , "Fusion"},
	{31 , "Trance"}, {32 , "Classical"}, {33 , "Instrumental"}, {34 , "Acid"}, {35 , "House"}, {36 , "Game"}, {37 , "Sound Clip"}, {38 , "Gospel"}, {39 , "Noise"}, {40 , "Alternative Rock"},
	{41 , "Bass"}, {42 , "Soul"}, {43 , "Punk"}, {44 , "Space"}, {45 , "Meditative"}, {46 , "Instrumental Pop"}, {47 , "Instrumental Rock"}, {48 , "Ethnic"}, {49 , "Gothic"}, {50 , "Darkwave"},
	{51 , "Techno-Industrial"}, {52 , "Electronic"}, {53 , "Pop-Folk"}, {54 , "Eurodance"}, {55 , "Dream"}, {56 , "Southern Rock"}, {57 , "Comedy"}, {58 , "Cult"}, {59 , "Gangsta"}, {60 , "Top 40"},
	{61 , "Christian Rap"}, {62 , "Pop/Funk"}, {63 , "Jungle"}, {64 , "Native US"}, {65 , "Cabaret"}, {66 , "New Wave"}, {67 , "Psychadelic"}, {68 , "Rave"}, {69 , "Showtunes"}, {70 , "Trailer"},
	{71 , "Lo-Fi"}, {72 , "Tribal"}, {73 , "Acid Punk"}, {74 , "Acid Jazz"}, {75 , "Polka"}, {76 , "Retro"}, {77 , "Musical"}, {78 , "Rock & Roll"}, {79 , "Hard Rock"}, {80 , "Folk"},
	{81 , "Folk-Rock"}, {82 , "National Folk"}, {83 , "Swing"}, {84 , "Fast Fusion"}, {85 , "Bebob"}, {86 , "Latin"}, {87 , "Revival"}, {88 , "Celtic"}, {89 , "Bluegrass"}, {90 , "Avantgarde"},
	{91 , "Gothic Rock"}, {92 , "Progressive Rock"}, {93 , "Psychedelic Rock"}, {94 , "Symphonic Rock"}, {95 , "Slow Rock"}, {96 , "Big Band"}, {97 , "Chorus"}, {98 , "Easy Listening"}, {99 , "Acoustic"},
	{100 , "Humour"}, {101 , "Speech"}, {102 , "Chanson"}, {103 , "Opera"}, {104 , "Chamber Music"}, {105 , "Sonata"}, {106 , "Symphony"}, {107 , "Booty Bass"}, {108 , "Primus"}, {109 , "Porn Groove"},
	{110 , "Satire"}, {111 , "Slow Jam"}, {112 , "Club"}, {113 , "Tango"}, {114 , "Samba"}, {115 , "Folklore"}, {116 , "Ballad"}, {117 , "Power Ballad"}, {118 , "Rhytmic Soul"}, {119 , "Freestyle"}, {120 , "Duet"},
	{121 , "Punk Rock"}, {122 , "Drum Solo"}, {123 , "A capella"}, {124 , "Euro-House"}, {125 , "Dance Hall"}, {126 , "Goa"}, {127 , "Drum & Bass"}, {128 , "Club-House"}, {129 , "Hardcore"}, {130 , "Terror"},
	{131 , "Indie"}, {132 , "BritPop"}, {133 , "Negerpunk"}, {134 , "Polsk Punk"}, {135 , "Beat"}, {136 , "Christian Gangsta"}, {137 , "Heavy Metal"}, {138 , "Black Metal"}, {139 , "Crossover"}, {140 , "Contemporary C"},
	{141 , "Christian Rock"}, {142 , "Merengue"}, {143 , "Salsa"}, {144 , "Thrash Metal"}, {145 , "Anime"}, {146 , "JPop"}, {147 , "SynthPop"}
};

static enum Audio_FileType file_type = FILE_TYPE_NONE;
static SDL_AudioDeviceID audio_device;
static u64 frames_read = 0, total_samples = 0;
Audio_Metadata metadata;

static drflac *flac;
static stb_vorbis *ogg;
static stb_vorbis_info ogg_info;
static drwav *wav;
static jar_xm_context_t *xm;
static char *xm_data = NULL;
static mpg123_handle *mp3;

bool playing = true, paused = false;


static u32 Audio_GetSampleRate(void) {
	u32 rate = 0;

	switch(file_type) {
		case FILE_TYPE_FLAC:
			rate = flac->sampleRate;
			break;

		case FILE_TYPE_MP3:
			mpg123_getformat(mp3, (long *)&rate, NULL, NULL);
			break;

		case FILE_TYPE_OGG:
			rate = ogg_info.sample_rate;
			break;

		case FILE_TYPE_WAV:
			rate = wav->sampleRate;
			break;

		case FILE_TYPE_XM:
			rate = 44100;
			break;

		default:
			break;
	}

	return rate;
}

static u8 Audio_GetChannels(void) {
	u8 channels = 0;

	switch(file_type) {
		case FILE_TYPE_FLAC:
			channels = flac->channels;
			break;

		case FILE_TYPE_MP3:
			mpg123_getformat(mp3, NULL, (int *)&channels, NULL);
			break;

		case FILE_TYPE_OGG:
			channels = ogg_info.channels;
			break;

		case FILE_TYPE_WAV:
			channels = wav->channels;
			break;

		case FILE_TYPE_XM:
			channels = 2;
			break;

		default:
			break;
	}

	return channels;
}

/* // Causes Problems with FLAC files that do not contain metadata
// For FLAC metadata
static void FLAC_SplitVorbisComments(char *comment, char *tag, char *value){
	char *result = NULL;
	result = strtok(comment, "=");
	int count = 0;

	while((result != NULL) && (count < 2)) {
		if (strlen(result) > 0) {
			switch (count) {
				case 0:
					strncpy(tag, result, 30);
					tag[30] = '\0';
					break;
				case 1:
					strncpy(value, result, 255);
					value[255] = '\0';
					break;
			}

			count++;
		}
		result = strtok(NULL, "=");
	}
}

// For FLAC metadata
static void FLAC_MetaCallback(void *pUserData, drflac_metadata *pMetadata) {
	char tag[31];
	char value[256];

	if (pMetadata->type == DRFLAC_METADATA_BLOCK_TYPE_VORBIS_COMMENT) {
		drflac_vorbis_comment_iterator iterator;
		drflac_uint32 comment_length;
		const char *comment_str;

		drflac_init_vorbis_comment_iterator(&iterator, pMetadata->data.vorbis_comment.commentCount, pMetadata->data.vorbis_comment.pComments);

		while((comment_str = drflac_next_vorbis_comment(&iterator, &comment_length)) != NULL) {
			FLAC_SplitVorbisComments((char *)comment_str, tag, value);
			if (!strcasecmp(tag, "TITLE")) {
				strcpy(metadata.title, value);
			}
			if (!strcasecmp(tag, "ALBUM")) {
				strcpy(metadata.album, value);
			}
			if (!strcasecmp(tag, "ARTIST")) {
				strcpy(metadata.artist, value);
			}
		}
	}
	if (pMetadata->type == DRFLAC_METADATA_BLOCK_TYPE_PICTURE) {
		if (pMetadata->data.picture.type == DRFLAC_PICTURE_TYPE_COVER_FRONT) {
			SDL_LoadImageMem(&metadata.cover_image, (drflac_uint8 *)pMetadata->data.picture.pPictureData, pMetadata->data.picture.pictureDataSize);
		}
	}
}*/

// For MP3 ID3 tags
// Helper for v1 printing, get these strings their zero byte.
static void safe_print(char *tag, char *name, char *data, size_t size) {
	char safe[31];
	if (size > 30) 
		return;
	memcpy(safe, data, size);
	safe[size] = 0;
	snprintf(tag, 34, "%s: %s\n", name, safe);
}


// For MP3 ID3 tags
// Print out ID3v1 info.
static void print_v1(Audio_Metadata *ID3tag, mpg123_id3v1 *v1) {
	safe_print(ID3tag->title, "",   v1->title,   sizeof(v1->title));
	safe_print(ID3tag->artist, "",  v1->artist,  sizeof(v1->artist));
	safe_print(ID3tag->album, "",   v1->album,   sizeof(v1->album));
	safe_print(ID3tag->year, "",    v1->year,    sizeof(v1->year));
	safe_print(ID3tag->comment, "", v1->comment, sizeof(v1->comment));
	safe_print(ID3tag->genre, "", genreList[v1->genre].text, sizeof(genreList[v1->genre].text));
}

// For MP3 ID3 tags
// Split up a number of lines separated by \n, \r, both or just zero byte
// and print out each line with specified prefix.
static void print_lines(char *data, const char *prefix, mpg123_string *inlines) {
	size_t i;
	int hadcr = 0, hadlf = 0;
	char *lines = NULL;
	char *line  = NULL;
	size_t len = 0;

	if (inlines != NULL && inlines->fill) {
		lines = inlines->p;
		len   = inlines->fill;
	}
	else 
		return;

	line = lines;
	for (i = 0; i < len; ++i) {
		if (lines[i] == '\n' || lines[i] == '\r' || lines[i] == 0) {
			char save = lines[i]; /* saving, changing, restoring a byte in the data */
			if (save == '\n') 
				++hadlf;
			if (save == '\r') 
				++hadcr;
			if ((hadcr || hadlf) && (hadlf % 2 == 0) && (hadcr % 2 == 0)) 
				line = "";

			if (line) {
				lines[i] = 0;
				if (data == NULL)
					printf("%s%s\n", prefix, line);
				else
					snprintf(data, 0x1F, "%s%s\n", prefix, line);
				line = NULL;
				lines[i] = save;
			}
		}
		else {
			hadlf = hadcr = 0;
			if (line == NULL) 
				line = lines + i;
		}
	}
}

// For MP3 ID3 tags
// Print out the named ID3v2  fields.
static void print_v2(Audio_Metadata *ID3tag, mpg123_id3v2 *v2) {
	print_lines(ID3tag->title, "", v2->title);
	print_lines(ID3tag->artist, "", v2->artist);
	print_lines(ID3tag->album, "", v2->album);
	print_lines(ID3tag->year, "",    v2->year);
	print_lines(ID3tag->comment, "", v2->comment);
	print_lines(ID3tag->genre, "",   v2->genre);
}

static void Audio_Callback(void *userdata, Uint8 *stream, int length) {
	memset(stream, 0, length);
	size_t done;

	switch(file_type) {
		case FILE_TYPE_FLAC:
			frames_read += drflac_read_pcm_frames_s32(flac, length / (sizeof(int) * 2), (drflac_int32 *)stream);
			break;

		case FILE_TYPE_MP3:
			mpg123_read(mp3, stream, length, &done);
			frames_read += (done  / (sizeof(s16) * 2));
			break;

		case FILE_TYPE_WAV:
			frames_read += drwav_read_pcm_frames_s32(wav, length / (sizeof(int) * 2), (drflac_int32 *)stream);
			break;

		case FILE_TYPE_OGG:
			frames_read += stb_vorbis_get_samples_short_interleaved(ogg, ogg_info.channels, (short *)stream, (int)(length / 2));
			break;

		case FILE_TYPE_XM:
			jar_xm_generate_samples_16bit(xm, (short *)stream, (size_t)(length / (sizeof(s16) * 2)));
			jar_xm_get_position(xm, NULL, NULL, NULL, &frames_read);
			break;

		default:
			break;
	}

	if (frames_read == total_samples)
		playing = false;
}

/// TODO: proper error checking
int Audio_Init(const char *path) {
	playing = true;
	paused = false;
	frames_read = 0;
	total_samples = 0;

	int error = 0;
	u64 xm_size_bytes = 0;

	SDL_AudioSpec want, have;
	SDL_memset(&want, 0, sizeof(want));

	// Clear struct
	static const Audio_Metadata empty;
	metadata = empty;
	mpg123_id3v1 *v1;
	mpg123_id3v2 *v2;

	if (!strncasecmp(FS_GetFileExt(path), "flac", 4))
		file_type = FILE_TYPE_FLAC;
	else if (!strncasecmp(FS_GetFileExt(path), "mp3", 3))
		file_type = FILE_TYPE_MP3;
	else if (!strncasecmp(FS_GetFileExt(path), "ogg", 3))
		file_type = FILE_TYPE_OGG;
	else if (!strncasecmp(FS_GetFileExt(path), "wav", 3))
		file_type = FILE_TYPE_WAV;
	else if (!strncasecmp(FS_GetFileExt(path), "xm", 2))
		file_type = FILE_TYPE_XM;

	switch(file_type) {
		case FILE_TYPE_FLAC:
			flac = drflac_open_file(path);
			if (!flac)
				return -1;

			total_samples = flac->totalPCMFrameCount;
			want.format = AUDIO_S32;
			break;

		case FILE_TYPE_MP3:
			error = mpg123_init();
			if (error != MPG123_OK)
				return error;

			mp3 = mpg123_new(NULL, &error);
			if (error != MPG123_OK)
				return error;

			error = mpg123_param(mp3, MPG123_ADD_FLAGS, MPG123_PICTURE, 0.0);
			if (error != MPG123_OK)
				return error;

			error = mpg123_open(mp3, path);
			if (error != MPG123_OK)
				return error;

			mpg123_seek(mp3, 0, SEEK_SET);
			metadata.has_meta = mpg123_meta_check(mp3);
			if (metadata.has_meta & MPG123_ID3 && mpg123_id3(mp3, &v1, &v2) == MPG123_OK) {
				if (v1 != NULL)
					print_v1(&metadata, v1);
				if (v2 != NULL) {
					print_v2(&metadata, v2);

					for (size_t count = 0; count < v2->pictures; count++) {
						mpg123_picture *pic = &v2->picture[count];
						char *str = pic->mime_type.p;

						if ((pic->type == 3 ) || (pic->type == 0)) {
							if ((!strcasecmp(str, "image/jpg")) || (!strcasecmp(str, "image/jpeg")) || (!strcasecmp(str, "image/png"))) {
								SDL_LoadImageMem(&metadata.cover_image, pic->data, pic->size);
								break;
							}
						}
					}
				}
			}

			total_samples = mpg123_length(mp3);
			mpg123_format_none(mp3);
			mpg123_format(mp3, Audio_GetSampleRate(), Audio_GetChannels(), MPG123_ENC_SIGNED_16);
			want.format = AUDIO_S16;
			break;

		case FILE_TYPE_OGG:
			ogg = stb_vorbis_open_filename(path, NULL, NULL);
			if (!ogg)
				return -1;

			ogg_info = stb_vorbis_get_info(ogg);
			total_samples = stb_vorbis_stream_length_in_samples(ogg);
			want.format = AUDIO_S16;
			break;

		case FILE_TYPE_WAV:
			wav = drwav_open_file(path);
			if (!wav)
				return -1;

			total_samples = wav->totalPCMFrameCount;
			want.format = AUDIO_S32;
			break;

		case FILE_TYPE_XM:
			if (R_FAILED(error = FS_GetFileSize(fs, path, &xm_size_bytes)))
				return error;

			xm_data = malloc(xm_size_bytes);
			
			if (R_FAILED(error = FS_Read(fs, path, xm_size_bytes, xm_data)))
				return error;

			jar_xm_create_context_safe(&xm, xm_data, (size_t)xm_size_bytes, (u32)44100);
			total_samples = jar_xm_get_remaining_samples(xm);
			want.format = AUDIO_S16;
			break;

		default:
			break;
	}

	want.freq = Audio_GetSampleRate();
	want.channels = Audio_GetChannels();
	want.userdata = NULL;
	want.samples = 4096;
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
	return frames_read;
}

u64 Audio_GetLength(void) {
	return total_samples;
}

u64 Audio_GetPositionSeconds() {
	return (Audio_GetPosition() / Audio_GetSampleRate());
}

u64 Audio_GetLengthSeconds() {
	return (Audio_GetLength() / Audio_GetSampleRate());
}

void Audio_Term(void) {
	switch(file_type) {
		case FILE_TYPE_FLAC:
			drflac_close(flac);
			break;

		case FILE_TYPE_MP3:
			mpg123_close(mp3);
			mpg123_delete(mp3);
			mpg123_exit();
			break;

		case FILE_TYPE_OGG:
			stb_vorbis_close(ogg);
			break;

		case FILE_TYPE_WAV:
			drwav_close(wav);
			break;

		case FILE_TYPE_XM:
			jar_xm_free_context(xm);
			free(xm_data);
			break;

		default:
			break;
	}

	playing = true;
	paused = false;
	SDL_PauseAudioDevice(audio_device, 1);
	SDL_CloseAudioDevice(audio_device);
}
