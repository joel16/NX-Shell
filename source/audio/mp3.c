#include <mpg123.h>
#include <stdio.h>
#include <string.h>

#include "audio.h"
#include "SDL_helper.h"

static mpg123_handle *mp3;
static u64 frames_read = 0, total_samples = 0;
static int channels = 0;

// 147 ID3 tagv1 list
static char *genre_list[] = {
	"Blues", "Classic Rock", "Country", "Dance", "Disco", "Funk",
	"Grunge", "Hip-Hop", "Jazz", "Metal", "New Age", "Oldies",
	"Other", "Pop", "R&B", "Rap", "Reggae", "Rock",
	"Techno", "Industrial", "Alternative", "Ska", "Death Metal", "Pranks",
	"Soundtrack", "Euro-Techno", "Ambient", "Trip-Hop", "Vocal", "Jazz+Funk",
	"Fusion", "Trance", "Classical", "Instrumental", "Acid", "House",
	"Game", "Sound Clip", "Gospel", "Noise", "AlternRock", "Bass",
	"Soul", "Punk", "Space", "Meditative", "Instrumental Pop", "Instrumental Rock",
	"Ethnic", "Gothic", "Darkwave", "Techno-Industrial", "Electronic", "Pop-Folk",
	"Eurodance", "Dream", "Southern Rock", "Comedy", "Cult", "Gangsta",
	"Top 40", "Christian Rap", "Pop/Funk", "Jungle", "Native American", "Cabaret",
	"New Wave", "Psychadelic", "Rave", "Showtunes", "Trailer", "Lo-Fi",
	"Tribal", "Acid Punk", "Acid Jazz", "Polka", "Retro", "Musical",
	"Rock & Roll", "Hard Rock", "Folk", "Folk/Rock", "National Folk", "Swing",
	"Fast-Fusion", "Bebob", "Latin", "Revival", "Celtic", "Bluegrass", "Avantgarde",
	"Gothic Rock", "Progressive Rock", "Psychedelic Rock", "Symphonic Rock", "Slow Rock", "Big Band",
	"Chorus", "Easy Listening", "Acoustic", "Humour", "Speech", "Chanson",
	"Opera", "Chamber Music", "Sonata", "Symphony", "Booty Bass", "Primus",
	"Porn Groove", "Satire", "Slow Jam", "Club", "Tango", "Samba",
	"Folklore", "Ballad", "Power Ballad", "Rhythmic Soul", "Freestyle", "Duet",
	"Punk Rock", "Drum Solo", "A capella", "Euro-House", "Dance Hall",
	"Goa", "Drum & Bass", "Club House", "Hardcore", "Terror",
	"Indie", "BritPop", "NegerPunk", "Polsk Punk", "Beat",
	"Christian Gangsta", "Heavy Metal", "Black Metal", "Crossover", "Contemporary C",
	"Christian Rock", "Merengue", "Salsa", "Thrash Metal", "Anime", "JPop",
	"SynthPop",
};

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
	safe_print(ID3tag->genre, "", genre_list[v1->genre], sizeof(genre_list[v1->genre]));
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
					snprintf(data, 64, "%s%s\n", prefix, line);
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

int MP3_Init(const char *path) {
	int error = mpg123_init();
	if (error != MPG123_OK)
		return error;

	mp3 = mpg123_new(NULL, &error);
	if (error != MPG123_OK)
		return error;

	error = mpg123_param(mp3, MPG123_FLAGS, MPG123_FORCE_SEEKABLE | MPG123_FUZZY | MPG123_SEEKBUFFER | MPG123_GAPLESS, 0.0);
	if (error != MPG123_OK)
		return error;

	// Let the seek index auto-grow and contain an entry for every frame
	error = mpg123_param(mp3, MPG123_INDEX_SIZE, -1, 0.0);
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

	mpg123_id3v1 *v1;
	mpg123_id3v2 *v2;

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

	mpg123_getformat(mp3, NULL, &channels, NULL);
	mpg123_format_none(mp3);
	mpg123_format(mp3, 44100, channels, MPG123_ENC_SIGNED_16);
	total_samples = mpg123_length(mp3);
	return 0;
}

u32 MP3_GetSampleRate(void) {
	return 44100;
}

u8 MP3_GetChannels(void) {
	return channels;
}

void MP3_Decode(void *buf, unsigned int length, void *userdata) {
	size_t done = 0;
	mpg123_read(mp3, buf, length, &done);
	frames_read += (done  / (sizeof(s16) * channels));

	if (frames_read == total_samples)
		playing = false;
}

u64 MP3_GetPosition(void) {
	return frames_read;
}

u64 MP3_GetLength(void) {
	return total_samples;
}

u64 MP3_Seek(u64 index) {
	off_t seek_frame = (total_samples * (index / 640.0));
	
	if (mpg123_seek(mp3, seek_frame, SEEK_SET) >= 0) {
		frames_read = seek_frame;
		return frames_read;
	}
	
	return -1;
}

void MP3_Term(void) {
	frames_read = 0;
	
	if (metadata.has_meta) {
		metadata.has_meta = false;

		if (metadata.cover_image)
			SDL_DestroyTexture(metadata.cover_image);
	}

	mpg123_close(mp3);
	mpg123_delete(mp3);
	mpg123_exit();
}
