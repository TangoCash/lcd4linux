/* $Id$
 * $URL$
 *
 * plugin to perform simple file operations
 *
 * Copyright (C) 2006 Chris Maj <cmaj@freedomcorpse.com>
 * Copyright (C) 2006 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
 *
 * This file is part of LCD4Linux.
 *
 * LCD4Linux is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * LCD4Linux is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/* 
 * exported functions:
 *
 * int plugin_init_file (void)
 *  adds various functions
 *
 */


/* define the include files you need */
#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* these should always be included */
#include "debug.h"
#include "plugin.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif


/* function 'readline' */
/* takes two arguments, file name and line number */
/* returns text of that line */

static void my_readline(RESULT * result, RESULT * arg1, RESULT * arg2)
{
    char value[256], val2[256];
    FILE *fp;
    int reqline, i, size;

    reqline = R2N(arg2);
    fp = fopen(R2S(arg1), "r");
    if (!fp) {
	info("readline couldn't open file '%s'", R2S(arg1));
	value[0] = '\0';
    } else {
	i = 0;
	while (!feof(fp) && i++ < reqline) {
	    fgets(val2, sizeof(val2), fp);
	    size = strcspn(val2, "\r\n");
	    strncpy(value, val2, size);
	    value[size] = '\0';
	    /* more than 256 chars, chew up rest of line */
	    while (!feof(fp) && strchr(val2, '\n') == NULL) {
		fgets(val2, sizeof(val2), fp);
	    }
	}
	fclose(fp);
	if (i <= reqline) {
	    info("readline requested line %d but file only had %d lines", reqline, i - 1);
	    value[0] = '\0';
	}
    }

    /* store result */
    SetResult(&result, R_STRING, &value);
}

/* function 'readconf' */
/* takes three arguments, config file name, (config key + delimiter) and fallback value */
/* returns value of config key */

static void my_readconf(RESULT *result, RESULT *arg1, RESULT *arg2, RESULT *arg3)
{
	char *key, *fallback, *s, *b;
	char *value;
	char line[512];
	FILE *fp;

	int i = 0, found = 0;

	key = R2S(arg2);
	int keylen = strlen(R2S(arg2));
	fallback = R2S(arg3);

	fp = fopen(R2S(arg1), "r");
	if (!fp)
	{
		info("readconf couldn't open file '%s'", R2S(arg1));
	}
	else
	{
		while (!feof(fp))
		{
			i++;
			fgets(line, sizeof(line), fp);
			s = strstr(line, key);
			if (s == NULL)
				continue;
			b = &line[s - line + keylen];
			value = malloc(strlen(b) + 1);
			strcpy(value, b);
			found = 1;
		}
		fclose(fp);
	}

	if (found == 0)
	{
		value = malloc(strlen(fallback) + 1);
		strcpy(value, fallback);
	}

	// remove trailing line feed
	int len = strlen(value);
	if ((len > 0) && (value[len - 1] == '\n'))
		value[len - 1] = '\0';
	len = strlen(value);
	if ((len > 0) && (value[len - 1] == '\r'))
		value[len - 1] = '\0';

	//info("readconf: %s %sfound, value '%s', fallback '%s'", key, found == 0 ? "not " : "", value, fallback);

	/* store result */
	SetResult(&result, R_STRING, value);
}

/* function 'exist' */
/* takes one argument, file name */
/* returns 1 if found, 0 if not */

static void my_exist(RESULT * result, RESULT * arg1)
{
	char *value;
	value = strdup("0");

	/* int access() return 0 if permitted, -1 otherwise */
	if ( access(R2S(arg1), F_OK) == 0 ) {
		value = strdup("1");
	}

	/* store result */
	SetResult(&result, R_STRING, value);
}

/* plugin initialization */
/* MUST NOT be declared 'static'! */
int plugin_init_file(void)
{
    /* register all our cool functions */
    /* the second parameter is the number of arguments */
    /* -1 stands for variable argument list */
    AddFunction("file::readline", 2, my_readline);
    AddFunction("file::readconf", 3, my_readconf);
    AddFunction("file::exist", 1, my_exist);

    return 0;
}

void plugin_exit_file(void)
{
    /* free any allocated memory */
    /* close filedescriptors */
}
