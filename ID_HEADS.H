// ID_HEADS.H

#ifndef _ID_HEADS_H_
#define _ID_HEADS_H_

#include <conio.h>
#include <io.h>
#include <fcntl.h>
#include <dos.h>
#include <math.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <mem.h>
#include <i86.h>
#include "VERSION.H"

//--------------------------------------------------------------------------

#ifndef SPEAR
#include "audiowl6.h"
#ifdef UPLOAD
#include "gfxv_apo.h"
#else
#ifdef GOODTIMES
#include "gfxv_wl6.h"
#else
#include "gfxv_apo.h"
#endif
#endif
#else
#include "audiosod.h"
#include "gfxv_sod.h"
#endif
//-----------------

//
//	ID Engine
//	Types.h - Generic types, #defines, etc.
//	v1.0d1
//
enum { false,true };
typedef char           boolean;
typedef unsigned char  byte;
typedef unsigned short word;
typedef long           fixed;
typedef unsigned long  longword;
typedef void           *memptr;

typedef struct
{
    int x,y;
} Point;

typedef struct
{
    Point ul,lr;
} Rect;


#include "ID_PM.H"
#include "ID_CA.H"
#include "ID_VL.H"
#include "ID_VH.H"
#include "ID_IN.H"
#include "ID_SD.H"
#include "ID_US.H"


void	Quit (char *error, ...);	// defined in user program

//
// replacing refresh manager with custom routines
//

#define PORTTILESWIDE           20      // all drawing takes place inside a
#define PORTTILESHIGH           13      // non displayed port of this size

#define UPDATEWIDE              PORTTILESWIDE
#define UPDATEHIGH              PORTTILESHIGH

#define MAXTICS                 10
#define DEMOTICS                4

extern	unsigned	mapwidth,mapheight,tics;

extern	word    	uwidthtable[UPDATEHIGH];
extern  word    	blockstarts[UPDATEWIDE*UPDATEHIGH];

extern  byte        fontcolor,backcolor;

extern  byte        signon[];

#define SETFONTCOLOR(f,b) fontcolor=f;backcolor=b;

#endif
