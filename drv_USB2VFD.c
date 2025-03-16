/* $Id$
 * $URL$
 *
 * driver for the Aliexpress AIDA64 USB2VFD displays 40x2
 * they are compatible-ish with the Matrix Orbital LK displays,
 * atleast that's the AIDA64 settings you have to use
 * but in reality they ignore the contrast and brightness commands
 * and freeze up after any command they don't like, also no custom chars
 * (it only likes go-home and go-to-position, all the rest are bad)
 * it also only displays anything if the display buffer is completely filled
 * special characters: | is °C, \DEL (0x7F) is a full block
 *
 * Copyright (C) 2025 Marceline <c0de@c0de.works>
 * Copyright (C) 2005, 2006, 2007 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
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
 *
 * exported fuctions:
 *
 * struct DRIVER drv_USB2VFD
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "debug.h"
#include "cfg.h"
#include "qprintf.h"
#include "udelay.h"
#include "plugin.h"
#include "widget.h"
#include "widget_text.h"
#include "widget_icon.h"
#include "widget_bar.h"
#include "drv.h"

/* text mode display? */
#include "drv_generic_text.h"

/* serial port? */
#include "drv_generic_serial.h"

static char Name[] = "USB2VFD";

static char dispBuffer[2][44];

/****************************************/
/***  hardware dependant functions    ***/
/****************************************/

static int drv_USB2VFD_open(const char *section)
{
    /* open serial port */
    /* don't mind about device, speed and stuff, this function will take care of */

    if (drv_generic_serial_open(section, Name, 0) < 0)
	return -1;

    return 0;
}


static int drv_USB2VFD_close(void)
{
    /* close whatever port you've opened */
    drv_generic_serial_close();

    return 0;
}


/* dummy function that sends something to the display */
static void drv_USB2VFD_send(const char *data, const unsigned int len)
{
    drv_generic_serial_write(data, len);
}

/* text mode displays only */
static void drv_USB2VFD_write(const int row, const int col, const char *data, int len)
{
    int i;
    char home[2] = { 0xFE, 0x48 };
    char line2[4] = { 0xFE, 0x47, 0x01, 0x02 };

    strncpy(&(dispBuffer[row][col]), data, len);

    drv_USB2VFD_send(home, 2);
    drv_USB2VFD_send(dispBuffer[0], DCOLS);
    drv_USB2VFD_send(line2, 4);
    drv_USB2VFD_send(dispBuffer[1], DCOLS);
}

/* text mode displays only */
static void drv_USB2VFD_clear(void)	// ~stolen~ borrowed from the MatrixOrbital Driver
{
    int i, j;

    for (i = 0; i < DROWS; i++) {
	for (j = 0; j < DCOLS + 4; j++) {
	    dispBuffer[i][j] = ' ';
	}
	drv_USB2VFD_write(1, i + 1, dispBuffer[i], DCOLS);
    }
}

/* text mode displays only */
static void drv_USB2VFD_defchar(const int ascii, const unsigned char *matrix)
{

}

/* start text mode display */
static int drv_USB2VFD_start(const char *section)
{
    DROWS = 2;
    DCOLS = 40;

    /* open communication with the display */
    if (drv_USB2VFD_open(section) < 0) {
	return -1;
    }

    drv_USB2VFD_clear();	/* clear display */

    return 0;
}


/****************************************/
/***            plugins               ***/
/****************************************/

/* none */


/****************************************/
/***        widget callbacks          ***/
/****************************************/


/* using drv_generic_text_draw(W) */


/****************************************/
/***        exported functions        ***/
/****************************************/


/* list models */
int drv_USB2VFD_list(void)
{
    printf("USB2VFD driver");
    return 0;
}


/* initialize driver & display */
/* use this function for a text display */
int drv_USB2VFD_init(const char *section, const int quiet)
{
    WIDGET_CLASS wc;
    int ascii;
    int ret;

    info("%s: %s", Name, "$Rev$");

    /* display preferences */
    XRES = 5;			/* pixel width of one char  */
    YRES = 8;			/* pixel height of one char  */
    CHARS = 8;			/* number of user-defineable characters */
    CHAR0 = 0;			/* ASCII of first user-defineable char */
    GOTO_COST = 4;		/* number of bytes a goto command requires */

    /* real worker functions */
    drv_generic_text_real_write = drv_USB2VFD_write;
    drv_generic_text_real_defchar = NULL;

    /* start display */
    if ((ret = drv_USB2VFD_start(section)) != 0)
	return ret;

    if (!quiet) {
	char buffer[40];
	qprintf(buffer, sizeof(buffer), "%s %dx%d", Name, DCOLS, DROWS);
	if (drv_generic_text_greet(buffer, "Marceline the Vampire Queen!")) {
	    sleep(3);
	    drv_USB2VFD_clear();
	}
    }

    /* initialize generic text driver */
    if ((ret = drv_generic_text_init(section, Name)) != 0)
	return ret;

    /* initialize generic bar driver */
    if ((ret = drv_generic_text_bar_init(1)) != 0)
	return ret;

    cfg_number(section, "BarChar", 0x7F, 1, 255, &ascii);

    /* add fixed chars to the bar driver */
    drv_generic_text_bar_add_segment(0, 0, 255, 32);	/* ASCII  32 = blank */
    drv_generic_text_bar_add_segment(255, 255, 255, ascii);


    /* register text widget */
    wc = Widget_Text;
    wc.draw = drv_generic_text_draw;
    widget_register(&wc);

    /* register bar widget */
    wc = Widget_Bar;
    wc.draw = drv_generic_text_bar_draw;
    widget_register(&wc);

    /* register plugins */
    // AddFunction("LCD::contrast", 1, plugin_contrast);

    return 0;
}


/* close driver & display */
/* use this function for a text display */
int drv_USB2VFD_quit(const int quiet)
{

    info("%s: shutting down.", Name);

    drv_generic_text_quit();

    /* clear display */
    drv_USB2VFD_clear();

    /* say goodbye... */
    if (!quiet) {
	drv_generic_text_greet("goodbye!", NULL);
    }

    debug("closing connection");
    drv_USB2VFD_close();

    return (0);
}


/* use this one for a text display */
DRIVER drv_USB2VFD = {
    .name = Name,
    .list = drv_USB2VFD_list,
    .init = drv_USB2VFD_init,
    .quit = drv_USB2VFD_quit,
};
