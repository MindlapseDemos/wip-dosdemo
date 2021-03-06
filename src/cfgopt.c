#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "cfgopt.h"

#ifdef NDEBUG
/* release build default options */
struct options opt = {
	0,	/* start_scr */
	1,	/* music */
	0,	/* mouse */
	0,	/* sball */
	0,	/* vsync */
	0	/* dbginfo */
};
#else
/* debug build default options */
struct options opt = {
	0,	/* start_scr */
	0,	/* music */
	1,	/* mouse */
	0,	/* sball */
	0,	/* vsync */
	1	/* dbginfo */
};
#endif

int parse_args(int argc, char **argv)
{
	int i;
	char *scrname = 0;

	for(i=1; i<argc; i++) {
		if(argv[i][0] == '-') {
			if(strcmp(argv[i], "-music") == 0) {
				opt.music = 1;
			} else if(strcmp(argv[i], "-nomusic") == 0) {
				opt.music = 0;
			} else if(strcmp(argv[i], "-scr") == 0 || strcmp(argv[i], "-screen") == 0) {
				scrname = argv[++i];
			} else if(strcmp(argv[i], "-mouse") == 0) {
				opt.mouse = 1;
			} else if(strcmp(argv[i], "-nomouse") == 0) {
				opt.mouse = 0;
			} else if(strcmp(argv[i], "-sball") == 0) {
				opt.sball = !opt.sball;
			} else if(strcmp(argv[i], "-vsync") == 0) {
				opt.vsync = 1;
			} else if(strcmp(argv[i], "-novsync") == 0) {
				opt.vsync = 0;
			} else if(strcmp(argv[i], "-dbg") == 0) {
				opt.dbginfo = 1;
			} else if(strcmp(argv[i], "-nodbg") == 0) {
				opt.dbginfo = 0;
#ifndef MSDOS
			} else if(strcmp(argv[i], "-fs") == 0) {
				opt.fullscreen = 1;
			} else if(strcmp(argv[i], "-win") == 0) {
				opt.fullscreen = 0;
			} else if(strcmp(argv[i], "-scaler-nearest") == 0) {
				opt.scaler = SCALER_NEAREST;
			} else if(strcmp(argv[i], "-scaler-linear") == 0) {
				opt.scaler = SCALER_LINEAR;
#endif
			} else {
				fprintf(stderr, "invalid option: %s\n", argv[i]);
				return -1;
			}
		} else {
			if(scrname) {
				fprintf(stderr, "unexpected option: %s\n", argv[i]);
				return -1;
			}
			scrname = argv[i];
		}
	}

	if(scrname) {
		opt.start_scr = scrname;
	}
	return 0;
}

static char *strip_space(char *s)
{
	int len;
	char *end;

	while(*s && isspace(*s)) ++s;
	if(!*s) return 0;

	if((end = strrchr(s, '#'))) {
		--end;
	} else {
		len = strlen(s);
		end = s + len - 1;
	}

	while(end > s && isspace(*end)) *end-- = 0;
	return end > s ? s : 0;
}

static int bool_value(char *s)
{
	char *ptr = s;
	while(*ptr) {
		*ptr = tolower(*ptr);
		++ptr;
	}

	return strcmp(s, "true") == 0 || strcmp(s, "yes") == 0 || strcmp(s, "1") == 0;
}

int load_config(const char *fname)
{
	FILE *fp;
	char buf[256];
	int nline = 0;

	if(!(fp = fopen(fname, "rb"))) {
		return 0;	/* just ignore missing config files */
	}

	while(fgets(buf, sizeof buf, fp)) {
		char *line, *key, *value;

		++nline;
		if(!(line = strip_space(buf))) {
			continue;
		}

		if(!(value = strchr(line, '='))) {
			fprintf(stderr, "%s:%d invalid key/value pair\n", fname, nline);
			return -1;
		}
		*value++ = 0;

		if(!(key = strip_space(line)) || !(value = strip_space(value))) {
			fprintf(stderr, "%s:%d invalid key/value pair\n", fname, nline);
			return -1;
		}

		if(strcmp(line, "music") == 0) {
			opt.music = bool_value(value);
		} else if(strcmp(line, "screen") == 0) {
			opt.start_scr = strdup(value);
		} else if(strcmp(line, "mouse") == 0) {
			opt.mouse = bool_value(value);
		} else if(strcmp(line, "sball") == 0) {
			opt.sball = bool_value(value);
		} else if(strcmp(line, "vsync") == 0) {
			opt.vsync = bool_value(value);
		} else if(strcmp(line, "debug") == 0) {
			opt.dbginfo = bool_value(value);
#ifndef MSDOS
		} else if(strcmp(line, "fullscreen") == 0) {
			opt.fullscreen = bool_value(value);
		} else if(strcmp(line, "scaler") == 0) {
			if(strcmp(value, "linear") == 0) {
				opt.scaler = SCALER_LINEAR;
			} else {
				opt.scaler = SCALER_NEAREST;
			}
#endif
		} else {
			fprintf(stderr, "%s:%d invalid option: %s\n", fname, nline, line);
			return -1;
		}
	}
	return 0;
}
