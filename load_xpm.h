#include <SDL2/SDL.h>

#define STARTING_HASH_SIZE 256
#define QUICK_COLORHASH(hash, key) ((hash)->table[*(Uint8*)(key)]->color)

static char* linebuf;
static int buflen;
static char* error;

struct hash_entry {
	char* key;
	Uint32 color;
	struct hash_entry* next;
};

struct color_hash {
	struct hash_entry** table;
	struct hash_entry* entries; /* array of all entries */
	struct hash_entry* next_free;
	int size;
	int maxnum;
};

#define SKIPSPACE(p)                             \
	do {                                         \
		while (SDL_isspace((unsigned char)*(p))) \
			++(p);                               \
	} while (0)

#define SKIPNONSPACE(p)                                 \
	do {                                                \
		while (!SDL_isspace((unsigned char)*(p)) && *p) \
			++(p);                                      \
	} while (0)

static int hash_key(const char* key, int cpp, int size) {
	int hash;

	hash = 0;
	while (cpp-- > 0) {
		hash = hash * 33 + *key++;
	}
	return hash & (size - 1);
}

static int add_colorhash(struct color_hash* hash, char* key, int cpp, Uint32 color) {
	int index = hash_key(key, cpp, hash->size);
	struct hash_entry* e = hash->next_free++;
	e->color = color;
	e->key = key;
	e->next = hash->table[index];
	hash->table[index] = e;
	return 1;
}

static Uint32 get_colorhash(struct color_hash* hash, const char* key, int cpp) {
	struct hash_entry* entry = hash->table[hash_key(key, cpp, hash->size)];
	while (entry) {
		if (SDL_memcmp(key, entry->key, cpp) == 0)
			return entry->color;
		entry = entry->next;
	}
	return 0; /* garbage in - garbage out */
}

static void free_colorhash(struct color_hash* hash) {
	if (hash) {
		if (hash->table)
			SDL_free(hash->table);
		if (hash->entries)
			SDL_free(hash->entries);
		SDL_free(hash);
	}
}

static int color_to_argb(char* spec, int speclen, Uint32* argb) {
	/* poor man's rgb.txt */
	static struct {
		char* name;
		Uint32 argb;
	} known[] = {
		{(char*)"none", 0x00000000}, {(char*)"black", 0xff000000}, {(char*)"white", 0xffFFFFFF},
		{(char*)"red", 0xffFF0000},	 {(char*)"green", 0xff00FF00}, {(char*)"blue", 0xff0000FF},
	};

	if (spec[0] == '#') {
		char buf[7];
		switch (speclen) {
			case 4:
				buf[0] = buf[1] = spec[1];
				buf[2] = buf[3] = spec[2];
				buf[4] = buf[5] = spec[3];
				break;
			case 7:
				SDL_memcpy(buf, spec + 1, 6);
				break;
			case 13:
				buf[0] = spec[1];
				buf[1] = spec[2];
				buf[2] = spec[5];
				buf[3] = spec[6];
				buf[4] = spec[9];
				buf[5] = spec[10];
				break;
		}
		buf[6] = '\0';
		*argb = 0xff000000 | (Uint32)SDL_strtol(buf, NULL, 16);
		return 1;
	} else {
		for (unsigned int i = 0; i < SDL_arraysize(known); i++) {
			if (SDL_strncasecmp(known[i].name, spec, speclen) == 0) {
				*argb = known[i].argb;
				return 1;
			}
		}
		return 0;
	}
}

static struct color_hash* create_colorhash(int maxnum) {
	int bytes, s;
	struct color_hash* hash;

	/* we know how many entries we need, so we can allocate
	   everything here */
	hash = (struct color_hash*)SDL_calloc(1, sizeof(*hash));
	if (!hash)
		return NULL;

	/* use power-of-2 sized hash table for decoding speed */
	for (s = STARTING_HASH_SIZE; s < maxnum; s <<= 1)
		;
	hash->size = s;
	hash->maxnum = maxnum;

	bytes = hash->size * sizeof(struct hash_entry**);
	/* Check for overflow */
	if ((bytes / sizeof(struct hash_entry**)) != hash->size) {
		SDL_SetError("memory allocation overflow");
		SDL_free(hash);
		return NULL;
	}
	hash->table = (struct hash_entry**)SDL_calloc(1, bytes);
	if (!hash->table) {
		SDL_free(hash);
		return NULL;
	}

	bytes = maxnum * sizeof(struct hash_entry);
	/* Check for overflow */
	if ((bytes / sizeof(struct hash_entry)) != maxnum) {
		SDL_SetError("memory allocation overflow");
		SDL_free(hash->table);
		SDL_free(hash);
		return NULL;
	}
	hash->entries = (struct hash_entry*)SDL_calloc(1, bytes);
	if (!hash->entries) {
		SDL_free(hash->table);
		SDL_free(hash);
		return NULL;
	}
	hash->next_free = hash->entries;
	return hash;
}

static char* get_next_line(char*** lines, SDL_RWops* src, int len) {
	char* linebufnew;

	if (lines) {
		return *(*lines)++;
	} else {
		char c;
		int n;
		do {
			if (!SDL_RWread(src, &c, 1, 1)) {
				error = (char*)"Premature end of data";
				return NULL;
			}
		} while (c != '"');
		if (len) {
			len += 4; /* "\",\n\0" */
			if (len > buflen) {
				buflen = len;
				linebufnew = (char*)SDL_realloc(linebuf, buflen);
				if (!linebufnew) {
					SDL_free(linebuf);
					error = (char*)"Out of memory";
					return NULL;
				}
				linebuf = linebufnew;
			}
			if (!SDL_RWread(src, linebuf, len - 1, 1)) {
				error = (char*)"Premature end of data";
				return NULL;
			}
			n = len - 2;
		} else {
			n = 0;
			do {
				if (n >= buflen - 1) {
					if (buflen == 0)
						buflen = 16;
					buflen *= 2;
					linebufnew = (char*)SDL_realloc(linebuf, buflen);
					if (!linebufnew) {
						SDL_free(linebuf);
						error = (char*)"Out of memory";
						return NULL;
					}
					linebuf = linebufnew;
				}
				if (!SDL_RWread(src, linebuf + n, 1, 1)) {
					error = (char*)"Premature end of data";
					return NULL;
				}
			} while (linebuf[n++] != '"');
			n--;
		}
		linebuf[n] = '\0';
		return linebuf;
	}
}

static SDL_Surface* load_xpm(char** xpm, SDL_RWops* src, SDL_bool force_32bit) {
	Sint64 start = 0;
	SDL_Surface* image = NULL;
	int index;
	int x, y;
	int w, h, ncolors, cpp;
	int indexed;
	Uint8* dst;
	struct color_hash* colors = NULL;
	SDL_Color* im_colors = NULL;
	char *keystrings = NULL, *nextkey;
	char* line;
	char*** xpmlines = NULL;
	int pixels_len;

	error = NULL;
	linebuf = NULL;
	buflen = 0;

	if (src)
		start = SDL_RWtell(src);

	if (xpm)
		xpmlines = &xpm;

	line = get_next_line(xpmlines, src, 0);
	if (!line)
		goto done;
	/*
	 * The header string of an XPMv3 image has the format
	 *
	 * <width> <height> <ncolors> <cpp> [ <hotspot_x> <hotspot_y> ]
	 *
	 * where the hotspot coords are intended for mouse cursors.
	 * Right now we don't use the hotspots but it should be handled
	 * one day.
	 */
	if (SDL_sscanf(line, "%d %d %d %d", &w, &h, &ncolors, &cpp) != 4 || w <= 0 || h <= 0 || ncolors <= 0 ||
		cpp <= 0) {
		error = (char*)"Invalid format description";
		goto done;
	}

	/* Check for allocation overflow */
	if ((size_t)(ncolors * cpp) / cpp != ncolors) {
		error = (char*)"Invalid color specification";
		goto done;
	}
	keystrings = (char*)SDL_malloc(ncolors * cpp);
	if (!keystrings) {
		error = (char*)"Out of memory";
		goto done;
	}
	nextkey = keystrings;

	/* Create the new surface */
	if (ncolors <= 256 && !force_32bit) {
		indexed = 1;
		image = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 8, 0, 0, 0, 0);
		im_colors = image->format->palette->colors;
		image->format->palette->ncolors = ncolors;
	} else {
		indexed = 0;
		image = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
	}
	if (!image) {
		/* Hmm, some SDL error (out of memory?) */
		goto done;
	}

	/* Read the colors */
	colors = create_colorhash(ncolors);
	if (!colors) {
		error = (char*)"Out of memory";
		goto done;
	}
	for (index = 0; index < ncolors; ++index) {
		char* p;
		line = get_next_line(xpmlines, src, 0);
		if (!line)
			goto done;

		p = line + cpp + 1;

		/* parse a colour definition */
		for (;;) {
			char nametype;
			char* colname;
			Uint32 argb, pixel;

			SKIPSPACE(p);
			if (!*p) {
				error = (char*)"colour parse error";
				goto done;
			}
			nametype = *p;
			SKIPNONSPACE(p);
			SKIPSPACE(p);
			colname = p;
			SKIPNONSPACE(p);
			if (nametype == 's')
				continue; /* skip symbolic colour names */

			if (!color_to_argb(colname, (int)(p - colname), &argb))
				continue;

			SDL_memcpy(nextkey, line, cpp);
			if (indexed) {
				SDL_Color* c = im_colors + index;
				c->a = (Uint8)(argb >> 24);
				c->r = (Uint8)(argb >> 16);
				c->g = (Uint8)(argb >> 8);
				c->b = (Uint8)(argb);
				pixel = index;
				if (argb == 0x00000000) {
					SDL_SetColorKey(image, SDL_TRUE, pixel);
				}
			} else {
				pixel = argb;
			}
			add_colorhash(colors, nextkey, cpp, pixel);
			nextkey += cpp;
			break;
		}
	}

	/* Read the pixels */
	pixels_len = w * cpp;
	dst = (Uint8*)image->pixels;
	for (y = 0; y < h; y++) {
		line = get_next_line(xpmlines, src, pixels_len);
		if (!line)
			goto done;

		if (indexed) {
			/* optimization for some common cases */
			if (cpp == 1)
				for (x = 0; x < w; x++)
					dst[x] = (Uint8)QUICK_COLORHASH(colors, line + x);
			else
				for (x = 0; x < w; x++)
					dst[x] = (Uint8)get_colorhash(colors, line + x * cpp, cpp);
		} else {
			for (x = 0; x < w; x++)
				((Uint32*)dst)[x] = get_colorhash(colors, line + x * cpp, cpp);
		}
		dst += image->pitch;
	}

done:
	if (error) {
		if (src)
			SDL_RWseek(src, start, RW_SEEK_SET);
		if (image) {
			SDL_FreeSurface(image);
			image = NULL;
		}
		SDL_SetError("%s", error);
	}
	if (keystrings)
		SDL_free(keystrings);
	free_colorhash(colors);
	if (linebuf)
		SDL_free(linebuf);
	return (image);
}

static SDL_Surface* IMG_ReadXPMFromArray(char** xpm) {
	if (!xpm) {
		SDL_SetError("array is NULL");
		return NULL;
	}
	return load_xpm(xpm, NULL, SDL_FALSE);
}
