#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <mpg123.h>
#include <stdio.h>
#include <string.h>
#include <switch.h>

#include "mp3.h"

#ifndef ULONG_MAX
#define ULONG_MAX ((unsigned long)-1)
#endif

#if !(defined PLAIN_C89) && (defined SIZEOF_SIZE_T) && (SIZEOF_SIZE_T > SIZEOF_LONG) && (defined PRIuMAX)
# define SIZE_P PRIuMAX
typedef uintmax_t size_p;
#else
# define SIZE_P "lu"
typedef unsigned long size_p;
#endif

#if !(defined PLAIN_C89) && (defined SIZEOF_SSIZE_T) && (SIZEOF_SSIZE_T > SIZEOF_LONG) && (defined PRIiMAX)
# define SSIZE_P PRIuMAX
typedef intmax_t ssize_p;
#else
# define SSIZE_P "li"
typedef long ssize_p;
#endif

static int errors = 0;
static off_t numberOfSamples;

static struct {
	int store_pics;
	int do_scan;
} param = {
	  false
	, true
};

static mpg123_handle *mp3_handle;

struct genre {
	int code;
	char text[112];
};

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

/* Helper for v1 printing, get these strings their zero byte. */
static void safe_print(char *tag, char *name, char *data, size_t size) {
	char safe[31];
	if (size > 30) 
		return;
	memcpy(safe, data, size);
	safe[size] = 0;
	snprintf(tag, 34, "%s: %s\n", name, safe);
}


/* Print out ID3v1 info. */
static void print_v1(ID3_Tag *ID3tag, mpg123_id3v1 *v1) {
	safe_print(ID3tag->title, "",   v1->title,   sizeof(v1->title));
	safe_print(ID3tag->artist, "",  v1->artist,  sizeof(v1->artist));
	safe_print(ID3tag->album, "",   v1->album,   sizeof(v1->album));
	safe_print(ID3tag->year, "",    v1->year,    sizeof(v1->year));
	safe_print(ID3tag->comment, "", v1->comment, sizeof(v1->comment));
	safe_print(ID3tag->genre, "", genreList[v1->genre].text, sizeof(genreList[v1->genre].text));
}

/* Split up a number of lines separated by \n, \r, both or just zero byte
   and print out each line with specified prefix. */
static void print_lines(char *data, const char *prefix, mpg123_string *inlines)
{
	size_t i;
	int hadcr = 0, hadlf = 0;
	char *lines = NULL;
	char *line  = NULL;
	size_t len = 0;

	if (inlines != NULL && inlines->fill)
	{
		lines = inlines->p;
		len   = inlines->fill;
	}
	else 
		return;

	line = lines;
	for (i = 0; i < len; ++i)
	{
		if (lines[i] == '\n' || lines[i] == '\r' || lines[i] == 0)
		{
			char save = lines[i]; /* saving, changing, restoring a byte in the data */
			if (save == '\n') 
				++hadlf;
			if (save == '\r') 
				++hadcr;
			if ((hadcr || hadlf) && (hadlf % 2 == 0) && (hadcr % 2 == 0)) 
				line = "";

			if (line)
			{
				lines[i] = 0;
				if (data == NULL)
					printf("%s%s\n", prefix, line);
				else
					snprintf(data, 0x1F, "%s%s\n", prefix, line);
				line = NULL;
				lines[i] = save;
			}
		}
		else
		{
			hadlf = hadcr = 0;
			if (line == NULL) 
				line = lines + i;
		}
	}
}

/* Print out the named ID3v2  fields. */
static void print_v2(ID3_Tag *ID3tag, mpg123_id3v2 *v2)
{
	print_lines(ID3tag->title, "", v2->title);
	print_lines(ID3tag->artist, "", v2->artist);
	print_lines(ID3tag->album, "", v2->album);
	print_lines(ID3tag->year, "",    v2->year);
	print_lines(ID3tag->comment, "", v2->comment);
	print_lines(ID3tag->genre, "",   v2->genre);
}

/* Easy conversion to string via lookup. */
static const char *pic_types[] = 
{
	 "other"
	,"icon"
	,"other icon"
	,"front cover"
	,"back cover"
	,"leaflet"
	,"media"
	,"lead"
	,"artist"
	,"conductor"
	,"orchestra"
	,"composer"
	,"lyricist"
	,"location"
	,"recording"
	,"performance"
	,"video"
	,"fish"
	,"illustration"
	,"artist logo"
	,"publisher logo"
};

static const char *pic_type(int id)
{
	return (id >= 0 && id < (sizeof(pic_types)/sizeof(char*))) ? pic_types[id] : "invalid type";
}

static void print_raw_v2(mpg123_id3v2 *v2)
{
	size_t i;
	for(i=0; i<v2->texts; ++i)
	{
		char id[5];
		char lang[4];
		memcpy(id, v2->text[i].id, 4);
		id[4] = 0;
		memcpy(lang, v2->text[i].lang, 3);
		lang[3] = 0;
		if (v2->text[i].description.fill)
		printf("%s language(%s) description(%s)\n", id, lang, v2->text[i].description.p);
		else printf("%s language(%s)\n", id, lang);

		print_lines(NULL, " ", &v2->text[i].text);
	}
	for(i=0; i<v2->extras; ++i)
	{
		char id[5];
		memcpy(id, v2->extra[i].id, 4);
		id[4] = 0;
		printf( "%s description(%s)\n",
		        id,
		        v2->extra[i].description.fill ? v2->extra[i].description.p : "" );
		print_lines(NULL, " ", &v2->extra[i].text);
	}
	for(i=0; i<v2->comments; ++i)
	{
		char id[5];
		char lang[4];
		memcpy(id, v2->comment_list[i].id, 4);
		id[4] = 0;
		memcpy(lang, v2->comment_list[i].lang, 3);
		lang[3] = 0;
		printf( "%s description(%s) language(%s):\n",
		        id,
		        v2->comment_list[i].description.fill ? v2->comment_list[i].description.p : "",
		        lang );
		print_lines(NULL, " ", &v2->comment_list[i].text);
	}
	for(i=0; i<v2->pictures; ++i)
	{
		mpg123_picture* pic;

		pic = &v2->picture[i];
		fprintf(stderr, "APIC type(%i, %s) mime(%s) size(%"SIZE_P")\n",
			pic->type, pic_type(pic->type), pic->mime_type.p, (size_p)pic->size);
		print_lines(NULL, " ", &pic->description);
	}
}

const char* unknown_end = "picture";

static char* mime2end(mpg123_string* mime)
{
	size_t len;
	char* end;
	if (strncasecmp("image/",mime->p,6))
	{
		len = strlen(unknown_end)+1;
		end = malloc(len);
		memcpy(end, unknown_end, len);
		return end;
	}

	/* Else, use fmt out of image/fmt ... but make sure that usage stops at
	   non-alphabetic character, as MIME can have funny stuff following a ";". */
	for(len=1; len<mime->fill-6; ++len)
	{
		if (!isalnum(mime->p[len-1+6])) break;
	}
	/* len now containing the number of bytes after the "/" up to the next
	   invalid char or null */
	if (len < 1) return "picture";

	end = malloc(len);
	if (!end) exit(11); /* Come on, is it worth wasting lines for a message? 
	                      If we're so broke, fprintf will also likely fail. */

	memcpy(end, mime->p+6,len-1);
	end[len-1] = 0;
	return end;
}

static void *safe_realloc(void *ptr, size_t size)
{
	if (ptr == NULL) return malloc(size);
	else return realloc(ptr, size);
}

/* Construct a sane file name without introducing spaces, then open.
   Example: /some/where/some.mp3.front_cover.jpeg
   If multiple ones are there: some.mp3.front_cover2.jpeg */
static int open_picfile(const char* prefix, mpg123_picture* pic)
{
	char *end, *typestr, *pfn;
	const char* pictype;
	size_t i, len;
	int fd;
	unsigned long count = 1;

	pictype = pic_type(pic->type);
	len = strlen(pictype);
	if (!(typestr = malloc(len+1))) exit(11);
	memcpy(typestr, pictype, len);
	for(i=0; i<len; ++i) if (typestr[i] == ' ') typestr[i] = '_';

	typestr[len] = 0;
	end = mime2end(&pic->mime_type);
	len = strlen(prefix)+1+strlen(typestr)+1+strlen(end);
	if (!(pfn = malloc(len+1))) exit(11);

	sprintf(pfn, "%s.%s.%s", prefix, typestr, end);
	pfn[len] = 0;

	errno = 0;
	fd = open(pfn, O_CREAT|O_WRONLY|O_EXCL, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);

	while(fd < 0 && errno == EEXIST && ++count < ULONG_MAX)
	{
		char dum;
		size_t digits;

		digits = snprintf(&dum, 1, "%lu", count);
		if (!(pfn=safe_realloc(pfn, len+digits+1))) exit(11);

		sprintf(pfn, "%s.%s%lu.%s", prefix, typestr, count, end);
		pfn[len+digits] = 0;
		errno = 0;
		fd = open(pfn, O_CREAT|O_WRONLY|O_EXCL, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
	}
	printf("writing %s\n", pfn);
	if (fd < 0)
	{
		//error("Cannot open for writing (counter exhaust? permissions?).");
		++errors;
	}

	free(end);
	free(typestr);
	free(pfn);
	return fd;
}

static void store_pictures(const char* prefix, mpg123_id3v2 *v2)
{
	int i;

	for(i=0; i<v2->pictures; ++i)
	{
		int fd;
		mpg123_picture* pic;

		pic = &v2->picture[i];
		fd = open_picfile(prefix, pic);
		if (fd >= 0)
		{ /* stream I/O for not having to care about interruptions */
			FILE* picfile = fdopen(fd, "w");
			if (picfile)
			{
				if (fwrite(pic->data, pic->size, 1, picfile) != 1)
				{
					//error("Failure to write data.");
					++errors;
				}
				if (fclose(picfile))
				{
					//error("Failure to close (flush?).");
					++errors;
				}
			}
			else
			{
				//error1("Unable to fdopen output: %s)", strerror(errno));
				++errors;
			}
		}
	}
}

int MP3_GetProgress(void)
{
	return (mpg123_tell(mp3_handle) / (numberOfSamples / 100.0));
}

void MP3_Init(char *path)
{
	int err = 0, meta = 0;

	err = mpg123_init();
	if (err != MPG123_OK)
		return;

	mp3_handle = mpg123_new(NULL, &err);
	if (err != MPG123_OK)
		return;

	err = mpg123_param(mp3_handle, MPG123_ADD_FLAGS, MPG123_PICTURE, 0.0);
	if (err != MPG123_OK)
		return;

	err = mpg123_open(mp3_handle, path);
	if (err != MPG123_OK)
		return;

	numberOfSamples = mpg123_length(mp3_handle);

	mpg123_id3v1 *v1;
	mpg123_id3v2 *v2;
			
	mpg123_seek(mp3_handle, 0, SEEK_SET);
	meta = mpg123_meta_check(mp3_handle);

	if (meta & MPG123_ID3 && mpg123_id3(mp3_handle, &v1, &v2) == MPG123_OK)
	{
		if (v1 != NULL)
			print_v1(&ID3, v1);
		if (v2 != NULL)
			print_v2(&ID3, v2);
		if (v2 != NULL)
		{
			print_raw_v2(v2);
			
			if (param.store_pics)
				store_pictures(path, v2);
		}
	}
}

void MP3_Exit(void)
{
	mpg123_close(mp3_handle);
	mpg123_delete(mp3_handle);
	mpg123_exit();
	
	// Clear ID3
	memset(ID3.artist, 0, 30);
	memset(ID3.title, 0, 30);
	memset(ID3.album, 0, 30);
	memset(ID3.year, 0, 4);
	memset(ID3.genre, 0, 30);
}