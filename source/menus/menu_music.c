#include <switch.h>
#include <mpg123.h>

#include "common.h"
#include "dirbrowse.h"
#include "menu_music.h"
#include "SDL_helper.h"
#include "status_bar.h"
#include "textures.h"
#include "utils.h"

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

#define MUSIC_STATUS_BAR_COLOUR SDL_MakeColour(97, 97, 97)
#define MUSIC_STATUS_BG_COLOUR  SDL_MakeColour(43, 53, 61)
#define MUSIC_SEPARATOR_COLOUR  SDL_MakeColour(34, 41, 48)

typedef struct 
{
	char title[0x1F];
	char album[0x1F];
	char artist[0x1F];
	char year[0x5];
	char comment[0x1F];
	char genre[0x1F];
} ID3_Tag;

static ID3_Tag ID3;
static mpg123_handle *mp3_handle;

struct genre
{
	int code;
	char text[112];
};

struct genre genreList[] =
{
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
static void safe_print(char *tag, char *name, char *data, size_t size)
{
	char safe[31];
	if (size > 30) 
		return;
	memcpy(safe, data, size);
	safe[size] = 0;
	snprintf(tag, 0x1F, "%s: %s\n", name, safe);
}


/* Print out ID3v1 info. */
static void print_v1(ID3_Tag *ID3tag, mpg123_id3v1 *v1)
{
	safe_print(ID3tag->title, "Title",   v1->title,   sizeof(v1->title));
	safe_print(ID3tag->artist, "Artist",  v1->artist,  sizeof(v1->artist));
	safe_print(ID3tag->album, "Album",   v1->album,   sizeof(v1->album));
	safe_print(ID3tag->year, "Year",    v1->year,    sizeof(v1->year));
	safe_print(ID3tag->comment, "Comment", v1->comment, sizeof(v1->comment));
	safe_print(ID3tag->genre, "Genre", genreList[v1->genre].text, sizeof(genreList[v1->genre].text));
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
	print_lines(ID3tag->title, "Title: ", v2->title);
	print_lines(ID3tag->artist, "", v2->artist);
	print_lines(ID3tag->album, "Album: ", v2->album);
	print_lines(ID3tag->year, "Year: ",    v2->year);
	print_lines(ID3tag->comment, "Comment: ", v2->comment);
	print_lines(ID3tag->genre, "Genre: ",   v2->genre);
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
		if(v2->text[i].description.fill)
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

void Menu_PlayMusic(char *path)
{
	Mix_OpenAudio(22050, AUDIO_S16SYS, 2, 640);
	Mix_Music *audio = Mix_LoadMUS(path);

	int progress = 0, err = 0, meta = 0;

	switch(Mix_GetMusicType(audio))
	{
		case MUS_CMD:
			break;
		case MUS_WAV:
			break;
		case MUS_MID:
			break;
		case MUS_MOD:
		case MUS_OGG:
		case MUS_MP3:
			err = mpg123_init();
			if (err != MPG123_OK)
				break;

			mp3_handle = mpg123_new(NULL, &err);
			if (err != MPG123_OK)
				break;

			err = mpg123_open(mp3_handle, path);
			if (err != MPG123_OK)
				break;

			progress = (mpg123_tell(mp3_handle) / (mpg123_length(mp3_handle) / 100));

			mpg123_id3v1 *v1;
			mpg123_id3v2 *v2;

			meta = mpg123_meta_check(mp3_handle);
			meta = mpg123_meta_check(mp3_handle);

			if (meta & MPG123_ID3 && mpg123_id3(mp3_handle, &v1, &v2) == MPG123_OK)
			{
				if (v1 != NULL)
					print_v1(&ID3, v1);
				if (v2 != NULL)
					print_v2(&ID3, v2);
				if (v2 != NULL)
					print_raw_v2(v2);
		
			}

			break;
		case MUS_NONE:
			break;
	}


	Mix_PlayMusic(audio, 1);

	int title_height = 0;
	TTF_SizeText(Roboto_large, Utils_Basename(path), NULL, &title_height);

	bool isPlaying = true;

	int btn_width = 0, btn_height = 0;
	SDL_QueryTexture(btn_pause, NULL, NULL, &btn_width, &btn_height);

	while(appletMainLoop())
	{
		SDL_ClearScreen(RENDERER, MUSIC_STATUS_BG_COLOUR);
		SDL_RenderClear(RENDERER);

		SDL_DrawRect(RENDERER, 0, 0, 1280, 40, MUSIC_STATUS_BAR_COLOUR); // Status bar
		SDL_DrawRect(RENDERER, 0, 40, 1280, 100, MUSIC_STATUS_BG_COLOUR); // Menu bar
		SDL_DrawRect(RENDERER, 0, 140, 1280, 1, MUSIC_SEPARATOR_COLOUR); // Separator

		SDL_DrawText(Roboto_large, 40, 40 + ((100 - title_height)/2), WHITE, strlen(ID3.title) == 0? Utils_Basename(path) : ID3.title); // Audio filename

		SDL_DrawRect(RENDERER, 0, 141, 560, 560, SDL_MakeColour(158, 158, 158)); // Draw album art background
		SDL_DrawImage(RENDERER, default_artwork, 0, 141, 560, 560); // Default album art

		SDL_DrawRect(RENDERER, 570, 141, 710, 559, SDL_MakeColour(45, 48, 50)); // Draw info box (outer)
		SDL_DrawRect(RENDERER, 575, 146, 700, 549, SDL_MakeColour(46, 49, 51)); // Draw info box (inner)

		if (strlen(ID3.artist) != 0)
			SDL_DrawText(Roboto_large, 590, 161, WHITE, ID3.artist);
		if (strlen(ID3.album) != 0)
			SDL_DrawText(Roboto_large, 590, 201, WHITE, ID3.album);
		if (strlen(ID3.year) != 0)
			SDL_DrawText(Roboto_large, 590, 241, WHITE, ID3.year);
		if (strlen(ID3.genre) != 0)
			SDL_DrawText(Roboto_large, 590, 281, WHITE, ID3.genre);

		SDL_DrawCircle(RENDERER, 615 + ((710 - btn_width) / 2), 186 + ((559 - btn_height) / 2), 80, SDL_MakeColour(98, 100, 101)); // Render outer circle
		SDL_DrawCircle(RENDERER, (615 + ((710 - btn_width) / 2)), (186 + ((559 - btn_height) / 2)), 60, SDL_MakeColour(46, 49, 51)); // Render inner circle

		if (isPlaying)
			SDL_DrawImage(RENDERER, btn_pause, 570 + ((710 - btn_width) / 2), 141 + ((559 - btn_height) / 2), btn_width, btn_height); // Playing
		else
			SDL_DrawImage(RENDERER, btn_play, 570 + ((710 - btn_width) / 2), 141 + ((559 - btn_height) / 2), btn_width, btn_height); // Paused

		StatusBar_DisplayTime();

		SDL_RenderPresent(RENDERER);

		hidScanInput();
		u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

		if ((kDown & KEY_B) || (!Mix_PlayingMusic()))
		{
			Mix_HaltMusic();
			break;
		}

		if (kDown & KEY_A)
		{
			if (isPlaying)
			{
				Mix_PauseMusic();
				isPlaying = false;
			}
			else
			{
				Mix_ResumeMusic();
				isPlaying = true;
			}
		}
	}

	Mix_FreeMusic(audio);
	Mix_CloseAudio();

	switch(Mix_GetMusicType(audio))
	{
		case MUS_MP3:
			mpg123_close(mp3_handle);
			// Clear ID3
			memset(ID3.artist, 0, 30);
			memset(ID3.title, 0, 30);
			memset(ID3.album, 0, 30);
			memset(ID3.year, 0, 4);
			memset(ID3.genre, 0, 30);
			break;
	}

	MENU_DEFAULT_STATE = MENU_STATE_HOME;
}