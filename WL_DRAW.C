// WL_DRAW.C

#include "WL_DEF.H"
#pragma hdrstop

//#define DEBUGWALLS

/*
=============================================================================

						 LOCAL CONSTANTS

=============================================================================
*/

#define ACTORSIZE	0x4000

/*
=============================================================================

						 GLOBAL VARIABLES

=============================================================================
*/


#ifdef DEBUGWALLS
unsigned screenloc[3]= {0,0,0};
#else
unsigned screenloc[3]= {PAGE1START,PAGE2START,PAGE3START};
#endif
unsigned freelatch = FREESTART;

long    lasttimecount;
long    frameon;
boolean fpscounter;
boolean drawplanes = false;

int     fpsframes,fpstime,fps;

int     wallheight[MAXVIEWWIDTH];


//
// math tables
//
short   pixelangle[MAXVIEWWIDTH];
long    finetangent[FINEANGLES/4];
fixed   sintable[ANGLES+ANGLES/4];
fixed   *costable = &sintable[ANGLES/4];

//
// refresh variables
//
fixed   viewx,viewy;            // the focal point
short   viewangle;
fixed   viewsin,viewcos;

//
// wall optimization variables
//
short    lastside;              // true for vertical
word     lasttilehit;
word     lasttexture;

//
// wall drawing variables
//
byte     *postsource;
int      postx;
int      postwidth;

//
// ray tracing variables
//
int      focaltx,focalty;

short    midangle;
longword xpartialup,xpartialdown,ypartialup,ypartialdown;
short    xinttile,yinttile;

word     tilehit;
int      pixx;

short    xtile,ytile;
short    xtilestep,ytilestep;
long     xintercept,yintercept;
word     pwallofs;

int      horizwall[MAXWALLTILES],vertwall[MAXWALLTILES];


/*
=============================================================================

						 LOCAL VARIABLES

=============================================================================
*/


/*
============================================================================

			   3 - D  DEFINITIONS

============================================================================
*/


//==========================================================================


/*
========================
=
= TransformActor
=
= Takes paramaters:
=   gx,gy		: globalx/globaly of point
=
= globals:
=   viewx,viewy		: point of view
=   viewcos,viewsin	: sin/cos of viewangle
=   scale		: conversion from global value to screen value
=
= sets:
=   screenx,transx,transy,screenheight: projected edge location and size
=
========================
*/

void TransformActor (objtype *ob)
{
    fixed gx,gy,gxt,gyt,nx,ny;

//
// translate point to view centered coordinates
//
    gx = ob->x-viewx;
    gy = ob->y-viewy;

//
// calculate newx
//
    gxt = FixedMul(gx,viewcos);
    gyt = FixedMul(gy,viewsin);
    nx = gxt-gyt-ACTORSIZE;     // fudge the shape forward a bit, because
                                // the midpoint could put parts of the shape
                                // into an adjacent wall
//
// calculate newy
//
    gxt = FixedMul(gx,viewsin);
    gyt = FixedMul(gy,viewcos);
    ny = gyt+gxt;

//
// calculate height / perspective ratio
//
    ob->transx = nx;
    ob->transy = ny;

    if (nx<MINDIST)         // too close, don't overflow the divide
    {
        ob->viewheight = 0;
        return;
    }

    ob->viewx = (short)(centerx + ny*scale/nx);      // DEBUG: use assembly divide
    ob->viewheight = (word)(heightnumerator/(nx>>8));
}

//==========================================================================

/*
========================
=
= TransformTile
=
= Takes paramaters:
=   tx,ty		: tile the object is centered in
=
= globals:
=   viewx,viewy		: point of view
=   viewcos,viewsin	: sin/cos of viewangle
=   scale		: conversion from global value to screen value
=
= sets:
=   screenx,transx,transy,screenheight: projected edge location and size
=
= Returns true if the tile is withing getting distance
=
========================
*/

boolean TransformTile (int tx, int ty, short *dispx, short *dispheight)
{
    fixed gx,gy,gxt,gyt,nx,ny;

//
// translate point to view centered coordinates
//
    gx = ((fixed)tx<<TILESHIFT)+0x8000-viewx;
    gy = ((fixed)ty<<TILESHIFT)+0x8000-viewy;

//
// calculate newx
//
    gxt = FixedMul(gx,viewcos);
    gyt = FixedMul(gy,viewsin);
    nx = gxt-gyt-0x2000;        // 0x2000 is size of object

//
// calculate newy
//
    gxt = FixedMul(gx,viewsin);
    gyt = FixedMul(gy,viewcos);
    ny = gyt+gxt;


//
// calculate height / perspective ratio
//
    if (nx<MINDIST)         // too close, don't overflow the divide
        *dispheight = 0;
    else
    {
        *dispx = (short)(centerx + ny*scale/nx);        // DEBUG: use assembly divide
        *dispheight = (short)(heightnumerator/(nx>>8));
    }

//
// see if it should be grabbed
//
    if (nx<TILEGLOBAL && ny>-TILEGLOBAL/2 && ny<TILEGLOBAL/2)
        return true;
    else
        return false;
}

//==========================================================================

/*
====================
=
= CalcHeight
=
= Calculates the height of xintercept,yintercept from viewx,viewy
=
====================
*/

int	CalcHeight (void)
{
	int   height;
    fixed gxt,gyt,nx;
	long  gx,gy;

	gx = xintercept - viewx;
	gxt = FixedMul(gx,viewcos);

	gy = yintercept - viewy;
	gyt = FixedMul(gy,viewsin);

	nx = gxt - gyt;

//
// calculate perspective ratio (heightnumerator/(nx>>8))
//
	if (nx < MINDIST)
		nx = MINDIST;        // don't let divide overflow

    height = heightnumerator / (nx >> 8);
    
    if (height < 8)
        height = 8;

    return height;
}

//==========================================================================

/*
===================
=
= ScalePost
=
===================
*/

void ScalePost (void)
{
	int  height;
	byte *dest;
    byte ofs;
	byte mask;

	height = wallheight[postx] >> 3;    // fractional height (low 3 bits frac)

	dest = bufferofs + (postx >> 2);

	ofs = ((postx & 3) << 3) + postwidth - 1;
	outp (SC_INDEX+1,(byte)*((byte *)mapmasks1 + ofs));
	DrawPost (height,postsource,dest);

	mask = (byte)*((byte *)mapmasks2 + ofs);

	if (!mask)
		return;

	dest++;
	outp (SC_INDEX+1,mask);
	DrawPost (height,postsource,dest);

	mask = (byte)*((byte *)mapmasks3 + ofs);

	if (!mask)
		return;

	dest++;
	outp (SC_INDEX+1,mask);
	DrawPost (height,postsource,dest);
}


/*
====================
=
= HitVertWall
=
= tilehit bit 7 is 0, because it's not a door tile
= if bit 6 is 1 and the adjacent tile is a door tile, use door side pic
=
====================
*/

void HitVertWall (void)
{
    int wallpic;
    int texture;

    texture = ((yintercept - pwallofs) >> 4) & 0xfc0;

    if (xtilestep == -1)
    {
        texture = 0xfc0 - texture;
        xintercept += TILEGLOBAL;
    }

    if (lastside == 1 && lasttilehit == tilehit && !(tilehit & 0x40))
    {
        //
        // in the same wall type as last time, so check for optimized draw
        //
        if ((pixx & 3) && texture == lasttexture && postwidth < 8)
        {
            //
            // wide scale
            //
            postwidth++;
            wallheight[pixx] = wallheight[pixx - 1];
        }
        else
        {
            ScalePost();
            wallheight[pixx] = CalcHeight();
            postsource += texture - lasttexture;
            postwidth = 1;
            postx = pixx;
            lasttexture = texture;
        }
    }
    else
    {
        //
        // new wall
        //
        if (lastside != -1)				// if not the first scaled post
            ScalePost();       			// draw last post
    
        lastside = true;

        lasttilehit = tilehit;
        lasttexture = texture;
        wallheight[pixx] = CalcHeight();
        postx = pixx;
        postwidth = 1;
    
        if (tilehit & 0x40)
        {
            //
            // check for adjacent doors
            //
            if (tilemap[xtile - xtilestep][yinttile] & 0x80)
                wallpic = DOORWALL + 3;
            else
                wallpic = vertwall[tilehit & ~0x40];
        }
        else
            wallpic = vertwall[tilehit];
    
        postsource = PM_GetPage(wallpic) + texture;
    }
}


/*
====================
=
= HitHorizWall
=
= tilehit bit 7 is 0, because it's not a door tile
= if bit 6 is 1 and the adjacent tile is a door tile, use door side pic
=
====================
*/

void HitHorizWall (void)
{
    int wallpic;
    int texture;

    texture = ((xintercept - pwallofs) >> 4) & 0xfc0;

    if (ytilestep == -1)
        yintercept += TILEGLOBAL;
    else
        texture = 0xfc0 - texture;

    if (!lastside && lasttilehit == tilehit && !(tilehit & 0x40))
    {
        //
        // in the same wall type as last time, so check for optimized draw
        //
        if ((pixx & 3) && texture == lasttexture && postwidth < 8)
        {
            //
            // wide scale
            //
            postwidth++;
            wallheight[pixx] = wallheight[pixx - 1];
        }
        else
        {
            ScalePost();
            wallheight[pixx] = CalcHeight();
            postsource += texture - lasttexture;
            postwidth = 1;
            postx = pixx;
            lasttexture = texture;
        }
    }
    else
    {
        //
        // new wall
        //
        if (lastside != -1)				// if not the first scaled post
            ScalePost();       			// draw last post

        lastside = false;

        lasttilehit = tilehit;
        lasttexture = texture;
        wallheight[pixx] = CalcHeight();
        postx = pixx;
        postwidth = 1;
    
        if (tilehit & 0x40)
        {   
            //
            // check for adjacent doors
            //
            if (tilemap[xinttile][ytile - ytilestep] & 0x80)
                wallpic = DOORWALL + 2;
            else
                wallpic = horizwall[tilehit & ~0x40];
        }
        else
            wallpic = horizwall[tilehit];
    
        postsource = PM_GetPage(wallpic) + texture;
    }
}

//==========================================================================

/*
====================
=
= HitHorizDoor
=
====================
*/

void HitHorizDoor (void)
{
    int doorpage;
    int doornum;
    int texture;

    doornum = tilehit & 0x7f;
    texture = ((xintercept - doorposition[doornum]) >> 4) & 0xfc0;

    if (lasttilehit == tilehit)
    {
        //
        // in the same door as last time, so check for optimized draw
        //
        if ((pixx & 3) && texture == lasttexture && postwidth < 8)
        {
            //
            // wide scale
            //
            postwidth++;
            wallheight[pixx] = wallheight[pixx - 1];
        }
        else
        {
            ScalePost();
            wallheight[pixx] = CalcHeight();
            postsource += texture - lasttexture;
            postwidth = 1;
            postx = pixx;
            lasttexture = texture;
        }
    }
    else
    {
        //
        // new door
        //
        if (lastside != -1)				// if not the first scaled post
            ScalePost();       			// draw last post
    
        lastside = 2;
        lasttilehit = tilehit;
        lasttexture = texture;
        wallheight[pixx] = CalcHeight();
        postx = pixx;
        postwidth = 1;
    
        switch (doorobjlist[doornum].lock)
        {
            case dr_normal:
                doorpage = DOORWALL;
                break;
            case dr_lock1:
            case dr_lock2:
            case dr_lock3:
            case dr_lock4:
                doorpage = DOORWALL + 6;
                break;
            case dr_elevator:
                doorpage = DOORWALL + 4;
                break;
        }
    
        postsource = PM_GetPage(doorpage) + texture;
    }
}

//==========================================================================

/*
====================
=
= HitVertDoor
=
====================
*/

void HitVertDoor (void)
{
    int doorpage;
    int doornum;
    int texture;

    doornum = tilehit & 0x7f;
    texture = ((yintercept - doorposition[doornum]) >> 4) & 0xfc0;

    if (lasttilehit == tilehit)
    {
        //
        // in the same door as last time, so check for optimized draw
        //
        if ((pixx & 3) && texture == lasttexture && postwidth < 8)
        {
            //
            // wide scale
            //
            postwidth++;
            wallheight[pixx] = wallheight[pixx - 1];
        }
        else
        {
            ScalePost();
            wallheight[pixx] = CalcHeight();
            postsource += texture - lasttexture;
            postwidth = 1;
            postx = pixx;
            lasttexture = texture;
        }
    }
    else
    {
        //
        // new door
        //
        if (lastside != -1)				// if not the first scaled post
            ScalePost();                // draw last post
    
        lastside = 2;
        lasttilehit = tilehit;
        lasttexture = texture;
        wallheight[pixx] = CalcHeight();
        postx = pixx;
        postwidth = 1;
    
        switch (doorobjlist[doornum].lock)
        {
            case dr_normal:
                doorpage = DOORWALL;
                break;
            case dr_lock1:
            case dr_lock2:
            case dr_lock3:
            case dr_lock4:
                doorpage = DOORWALL + 6;
                break;
            case dr_elevator:
                doorpage = DOORWALL + 4;
                break;
        }
    
        postsource = PM_GetPage(doorpage + 1) + texture;
    }
}


//==========================================================================

word vgaCeiling[] =
{
#ifndef SPEAR
    0x1d1d,0x1d1d,0x1d1d,0x1d1d,0x1d1d,0x1d1d,0x1d1d,0x1d1d,0x1d1d,0xbfbf,
    0x4e4e,0x4e4e,0x4e4e,0x1d1d,0x8d8d,0x4e4e,0x1d1d,0x2d2d,0x1d1d,0x8d8d,
    0x1d1d,0x1d1d,0x1d1d,0x1d1d,0x1d1d,0x2d2d,0xdddd,0x1d1d,0x1d1d,0x9898,
    
    0x1d1d,0x9d9d,0x2d2d,0xdddd,0xdddd,0x9d9d,0x2d2d,0x4d4d,0x1d1d,0xdddd,
    0x7d7d,0x1d1d,0x2d2d,0x2d2d,0xdddd,0xd7d7,0x1d1d,0x1d1d,0x1d1d,0x2d2d,
    0x1d1d,0x1d1d,0x1d1d,0x1d1d,0xdddd,0xdddd,0x7d7d,0xdddd,0xdddd,0xdddd,
#else
    0x6f6f,0x4f4f,0x1d1d,0xdede,0xdfdf,0x2e2e,0x7f7f,0x9e9e,0xaeae,0x7f7f,
    0x1d1d,0xdede,0xdfdf,0xdede,0xdfdf,0xdede,0xe1e1,0xdcdc,0x2e2e,0x1d1d,0xdcdc,
#endif
};

/*
=====================
=
= VGAClearScreen
=
=====================
*/

void VGAClearScreen (void)
{
    unsigned ceiling = vgaCeiling[gamestate.episode*10+mapon];
    ceiling |= ceiling << 16;

//
// clear the screen
//
    _asm
    {
        cli
        mov edx,SC_INDEX
        mov eax,SC_MAPMASK+15*256        // write through all planes
        out dx,ax
        sti
        mov edx,SCREENBWIDE
        mov ebx,[viewwidth]
        shr ebx,2
        sub edx,ebx                      // edx = 40 - viewwidth/2

        shr ebx,2
        mov bh,byte ptr [centery]

        mov edi,[bufferofs]
        mov eax,[ceiling]
        xor ecx,ecx

toploop:
        mov cl,bl
        rep stosd
        add edi,edx
        dec bh
        jnz toploop

        mov bh,byte ptr [centery]
        mov eax,0x19191919

bottomloop:
        mov cl,bl
        rep stosd
        add edi,edx
        dec bh
        jnz bottomloop
    }
}

//==========================================================================

/*
=====================
=
= CalcRotate
=
=====================
*/

int CalcRotate (objtype *ob)
{
    int angle,viewangle;

    // this isn't exactly correct, as it should vary by a trig value,
    // but it is close enough with only eight rotations

    viewangle = player->angle + (centerx - ob->viewx)/8;

    if (ob->obclass == rocketobj || ob->obclass == hrocketobj)
        angle =  (viewangle-180)- ob->angle;
    else
        angle =  (viewangle-180)- dirangle[ob->dir];

    angle+=ANGLES/16;
    while (angle>=ANGLES)
        angle-=ANGLES;
    while (angle<0)
        angle+=ANGLES;

    if (ob->state->rotate == 2) // 2 rotation pain frame
        return 0;               // pain with shooting frame bugfix

    return angle/(ANGLES/8);
}


/*
=====================
=
= DrawScaleds
=
= Draws all objects that are visable
=
=====================
*/

#define MAXVISABLE  50

typedef struct
{
    short viewx;
    short viewheight;
    short shapenum;
} visobj_t;

visobj_t vislist[MAXVISABLE];
visobj_t *visptr,*visstep,*farthest;

void DrawScaleds (void)
{
    int       i,least,numvisable,height;
    byte      *visspot;
    statobj_t *statptr;
    objtype   *obj;

    visptr = &vislist[0];

//
// place static objects
//
    for (statptr = &statobjlist[0]; statptr != laststatobj; statptr++)
    {
        if ((visptr->shapenum = statptr->shapenum) == -1)
            continue;						// object has been deleted

        if (!spotvis[statptr->tilex][statptr->tiley])
            continue;                       // not visable

        if (TransformTile (statptr->tilex,statptr->tiley,&visptr->viewx,&visptr->viewheight) && statptr->flags & FL_BONUS)
        {
            GetBonus (statptr);

            if (statptr->shapenum == -1)
                continue;                   // object has been taken
        }

        if (!visptr->viewheight)
            continue;                       // too close to the object

        if (visptr < &vislist[MAXVISABLE-1])    // don't let it overflow
            visptr++;
    }

//
// place active objects
//
    for (obj = player->next; obj; obj = obj->next)
    {
        visptr->shapenum = obj->state->shapenum;

        if (!visptr->shapenum)
            continue;                       // no shape

        visspot = &spotvis[obj->tilex][obj->tiley];

        //
        // could be in any of the nine surrounding tiles
        //
        if (*visspot
        || (*(visspot - 1))
        || (*(visspot + 1))
        || (*(visspot - (mapwidth + 1)))
        || (*(visspot - mapwidth))
        || (*(visspot - (mapwidth - 1)))
        || (*(visspot + (mapwidth + 1)))
        || (*(visspot + mapwidth))
        || (*(visspot + (mapwidth - 1))))
        {
            obj->active = ac_yes;
            TransformActor (obj);
            if (!obj->viewheight)
                continue;						// too close or far away

            visptr->viewx = obj->viewx;
            visptr->viewheight = obj->viewheight;
            if (visptr->shapenum == -1)
                visptr->shapenum = obj->temp1;  // special shape

            if (obj->state->rotate)
                visptr->shapenum += CalcRotate (obj);

            if (visptr < &vislist[MAXVISABLE-1])    // don't let it overflow
                visptr++;
            obj->flags |= FL_VISABLE;
        }
        else
            obj->flags &= ~FL_VISABLE;
    }

//
// draw from back to front
//
    numvisable = visptr - &vislist[0];

    if (!numvisable)
        return;									// no visable objects

    for (i = 0; i < numvisable; i++)
    {
        least = 0x7fff;

        for (visstep = &vislist[0]; visstep < visptr; visstep++)
        {
            height = visstep->viewheight;

            if (height < least)
            {
                least = height;
                farthest = visstep;
            }
        }

        //
        // draw farthest
        //
        ScaleShape(farthest->viewx,farthest->shapenum,farthest->viewheight);

        farthest->viewheight = 0x7fff;
    }
}

//==========================================================================

/*
==============
=
= DrawPlayerWeapon
=
= Draw the player's hands
=
==============
*/

void DrawPlayerWeapon (void)
{
    int shapenum;
    int weaponscale[NUMWEAPONS] = {SPR_KNIFEREADY,SPR_PISTOLREADY,SPR_MACHINEGUNREADY,SPR_CHAINREADY};

#ifndef SPEAR
    if (gamestate.victoryflag)
    {
        if (player->state == &s_deathcam && (TimeCount&32) )
            SimpleScaleShape(viewwidth/2,SPR_DEATHCAM,viewheight+1);
    }
    else
#endif
    {
        if (gamestate.weapon != -1)
        {
            shapenum = weaponscale[gamestate.weapon]+gamestate.weaponframe;
            SimpleScaleShape(viewwidth/2,shapenum,viewheight+1);
        }
    
        if (demorecord || demoplayback)
            SimpleScaleShape(viewwidth/2,SPR_DEMO,viewheight+1);
    }
}


//==========================================================================


/*
=====================
=
= CalcTics
=
=====================
*/

void CalcTics (void)
{
    long newtime;

//
// calculate tics since last refresh for adaptive timing
//
    if (lasttimecount > TimeCount)
        TimeCount = lasttimecount;		// if the game was paused a LONG time

    do
    {
        newtime = TimeCount;
        tics = newtime-lasttimecount;
    } while (!tics);			// make sure at least one tic passes

    lasttimecount = newtime;

    if (tics>MAXTICS)
    {
        TimeCount -= (tics-MAXTICS);
        tics = MAXTICS;
    }
}


//==========================================================================


/*
========================
=
= FixOfs
=
========================
*/

void FixOfs (void)
{
    VW_ScreenToScreen (displayofs,bufferofs,viewwidth >> 3,viewheight);
}


//==========================================================================


/*
====================
=
= AsmRefresh
=
====================
*/

void AsmRefresh (void)
{
    short    angle;
    long     xstep,ystep;
    long     xinttemp,yinttemp;
    longword xpartial,ypartial;
	int      pwallposi,pwallposnorm,pwallposinv;
	boolean  passdoor;

    for (pixx = 0; pixx < viewwidth; pixx++)
    {
        angle = midangle + pixelangle[pixx];

        if (angle < 0) 
            angle += FINEANGLES;
        if (angle >= 3600) 
            angle -= FINEANGLES;

        if (angle < 900)
        {
            xtilestep = 1;
            ytilestep = -1;
            xstep = finetangent[900 - 1 - angle];
            ystep = -finetangent[angle];
            xpartial = xpartialup;
            ypartial = ypartialdown;
        }
        else if (angle < 1800)
        {
            xtilestep = -1;
            ytilestep = -1;
            xstep = -finetangent[angle - 900];
            ystep = -finetangent[1800 - 1 - angle];
            xpartial = xpartialdown;
            ypartial = ypartialdown;
        }
        else if (angle < 2700)
        {
            xtilestep = -1;
            ytilestep = 1;
            xstep = -finetangent[2700 - 1 - angle];
            ystep = finetangent[angle - 1800];
            xpartial = xpartialdown;
            ypartial = ypartialup;
        }
        else if (angle < 3600)
        {
            xtilestep = 1;
            ytilestep = 1;
            xstep = finetangent[angle - 2700];
            ystep = finetangent[3600 - 1 - angle];
            xpartial = xpartialup;
            ypartial = ypartialup;
        }

        yintercept = FixedMul(ystep,xpartial) + viewy;
        yinttile = (short)(yintercept >> TILESHIFT);
        xtile = focaltx + xtilestep;
        
        xintercept = FixedMul(xstep,ypartial) + viewx;
        xinttile = (short)(xintercept >> TILESHIFT);
        ytile = focalty + ytilestep;
        
        pwallofs = 0;

        //
        // Special treatment when player is in back tile of pushwall
        //
        if (tilemap[focaltx][focalty] == 64)
        {
            if (pwalldir == di_east && xtilestep == 1 || pwalldir == di_west && xtilestep == -1)
            {
                yinttemp = yintercept - ((ystep * (64 - pwallpos)) >> 6);

                if (yinttemp >> TILESHIFT == focalty)   // ray hits pushwall back?
                {
                    if (pwalldir == di_east)
                        xintercept = ((long)focaltx << TILESHIFT) + (pwallpos << 10);
                    else
                        xintercept = ((long)focaltx << TILESHIFT) - TILEGLOBAL + ((64 - pwallpos) << 10);

                    yintercept = yinttemp;
                    yinttile = (short)(yintercept >> TILESHIFT);
                    tilehit = pwalltile;
                    HitVertWall();
                    continue;
                }
            }
            else if (pwalldir == di_south && ytilestep == 1 || pwalldir == di_north && ytilestep == -1)
            {
                xinttemp = xintercept - ((xstep * (64 - pwallpos)) >> 6);

                if (xinttemp >> TILESHIFT == focaltx)   // ray hits pushwall back?
                {
                    if (pwalldir == di_south)
                        yintercept = ((long)focalty << TILESHIFT) + (pwallpos << 10);
                    else
                        yintercept = ((long)focalty << TILESHIFT) - TILEGLOBAL + ((64 - pwallpos) << 10);

                    xintercept = xinttemp;
                    xinttile = (short)(xintercept >> TILESHIFT);
                    tilehit = pwalltile;
                    HitHorizWall();
                    continue;
                }
            }
        }
        
        while (1)
        {
            if ((ytilestep == -1 && yinttile <= ytile) || (ytilestep == 1 && yinttile >= ytile))
                goto horizentry;
vertentry:
            //
            // get the wall value from tilemap
            //
            if (tilemap[xtile][ytile] && (xtile - xtilestep) == xinttile && (ytile - ytilestep) == yinttile)
            {
                //
                // exactly in the wall corner, so use the last tile
                //
                tilehit = lasttilehit;

                if (tilehit & 0x80)
                    passdoor = false;                                           // don't let the trace continue if it's a door
            }
            else
            {
                tilehit = tilemap[xtile][yinttile];

                if (tilehit & 0x80)
                    passdoor = true;
            }

            if (tilehit)
            {
                if (tilehit & 0x80)
                {
                    yinttemp = yintercept + (ystep >> 1);

                    if (yinttemp >> TILESHIFT != yinttile && passdoor)
                        goto passvert;

                    if ((word)yinttemp < doorposition[tilehit & 0x7f])
                        goto passvert;

                    yintercept = yinttemp;
                    xintercept = ((long)xtile << TILESHIFT) + 0x8000;
                    HitVertDoor ();
                }
                else if (tilehit == 64)
                {
				  	if (pwalldir == di_west || pwalldir == di_east)
					{
						if (pwalldir == di_west)
						{
							pwallposnorm = 63 - pwallpos;
							pwallposinv = pwallpos;
						}
						else
						{
							pwallposnorm = pwallpos;
							pwallposinv = 63 - pwallpos;
						}
						
                        if (pwalldir == di_east && xtile == pwallx && yinttile == pwally
						 || pwalldir == di_west && !(xtile == pwallx && yinttile == pwally))
						{
							yinttemp = yintercept + ((ystep * pwallposnorm) >> 6);
   	                        
                            if (yinttemp >> TILESHIFT != yinttile)
   	                            goto passvert;

     		                xintercept = ((long)xtile << TILESHIFT) + TILEGLOBAL - (pwallposinv << 10);
  	      	                yintercept = yinttemp;
							tilehit = pwalltile;                                       
               	            HitVertWall();
                        }
						else
						{
							yinttemp = yintercept + ((ystep * pwallposinv) >> 6);
   	                        
                            if (yinttemp >> TILESHIFT != yinttile)
   	                            goto passvert;

     		                xintercept = ((long)xtile << TILESHIFT) - (pwallposinv << 10);
  	      	                yintercept = yinttemp;
							tilehit = pwalltile;                    
               	            HitVertWall();
						}
					}
					else
					{
						if (pwalldir == di_north) 
                            pwallposi = 63 - pwallpos;
                        else
                            pwallposi = pwallpos;

						if (pwalldir == di_south && (word)yintercept < (pwallposi << 10)
						 || pwalldir == di_north && (word)yintercept > (pwallposi << 10))
						{
							if (yinttile == pwally && xtile == pwallx)
							{												
							    if (pwalldir == di_south && (word)yintercept + ystep < (pwallposi << 10)
								 || pwalldir == di_north && (word)yintercept + ystep > (pwallposi << 10))
								    goto passvert;
									
							    if (pwalldir == di_south)
								    yintercept = ((long)yinttile << TILESHIFT) + (pwallposi << 10);
								else
								    yintercept = ((long)yinttile << TILESHIFT) - TILEGLOBAL + (pwallposi << 10);
      	                        
                                xintercept -= ((xstep * (63 - pwallpos)) >> 6);
                                xinttile = (short)(xintercept >> TILESHIFT);
								tilehit = pwalltile;
                                HitHorizWall();
							}
							else
							{
								pwallofs = pwallposi << 10;
								xintercept = (long)xtile << TILESHIFT;
								tilehit = pwalltile;
                                HitVertWall();
							}
						}
						else
						{
							if (yinttile == pwally && xtile == pwallx)
							{
								pwallofs = pwallposi << 10;
								xintercept = (long)xtile << TILESHIFT;
								tilehit = pwalltile;
                                HitVertWall();
							}
							else
							{
								if (pwalldir == di_south && (word)yintercept + ystep > (pwallposi << 10)
							     || pwalldir == di_north && (word)yintercept + ystep < (pwallposi << 10))
								    goto passvert;

								if (pwalldir == di_south)
									yintercept = ((long)yinttile << TILESHIFT) - ((63 - pwallpos) << 10);
								else
									yintercept = ((long)yinttile << TILESHIFT) + ((63 - pwallpos) << 10);
 		      	               
                                xintercept -= ((xstep * pwallpos) >> 6);
                                xinttile = (short)(xintercept >> TILESHIFT);
								tilehit = pwalltile;
                                HitHorizWall();
							}
						}
					}
                }
                else
                {
                    xintercept = (long)xtile << TILESHIFT;
                    HitVertWall();
                }

                break;
            }
passvert:
            spotvis[xtile][yinttile] = 1;
            xtile += xtilestep;
            yintercept += ystep;
            yinttile = (short)(yintercept >> TILESHIFT);
        }

        continue;

        while (1)
        {
            if ((xtilestep == -1 && xinttile <= xtile) || (xtilestep == 1 && xinttile >= xtile))
                goto vertentry;
horizentry:
            //
            // get the wall value from tilemap
            //
            if (tilemap[xtile][ytile] && (xtile - xtilestep) == xinttile && (ytile - ytilestep) == yinttile)
            {
                //
                // exactly in the wall corner, so use the last tile
                //
                tilehit = lasttilehit;

                if (tilehit & 0x80)
                    passdoor = false;                                           // don't let the trace continue if it's a door
            }
            else
            {
                tilehit = tilemap[xinttile][ytile];

                if (tilehit & 0x80)
                    passdoor = true;
            }

            if (tilehit)
            {
                if (tilehit & 0x80)
                {
                    xinttemp = xintercept + (xstep >> 1);
                    
                    if (xinttemp >> TILESHIFT != xinttile && passdoor)
                        goto passhoriz;

                    if ((word)xinttemp < doorposition[tilehit & 0x7f])
                        goto passhoriz;

                    xintercept = xinttemp;
                    yintercept = ((long)ytile << TILESHIFT) + 0x8000;
                    HitHorizDoor();
                }
                else if (tilehit == 64)
                {
					if (pwalldir == di_north || pwalldir == di_south)
					{
						if (pwalldir == di_north)
						{
							pwallposnorm = 63 - pwallpos;
							pwallposinv = pwallpos;
						}
						else
						{
							pwallposnorm = pwallpos;
							pwallposinv = 63 - pwallpos;
						}
						
                        if (pwalldir == di_south && ytile == pwally && xinttile == pwallx
						 || pwalldir == di_north && !(ytile == pwally && xinttile == pwallx))
						{
							xinttemp = xintercept + ((xstep * pwallposnorm) >> 6);

   	                        if (xinttemp >> TILESHIFT != xinttile)
   	                            goto passhoriz;

     		                yintercept = ((long)ytile << TILESHIFT) + TILEGLOBAL - (pwallposinv << 10);
  	      	                xintercept = xinttemp;
							tilehit = pwalltile;
                            HitHorizWall();
						}
						else
						{
							xinttemp = xintercept + ((xstep * pwallposinv) >> 6);
   	                        
                            if (xinttemp >> TILESHIFT != xinttile)
   	                            goto passhoriz;

     		                yintercept = ((long)ytile << TILESHIFT) - (pwallposinv << 10);
  	      	                xintercept = xinttemp;
							tilehit = pwalltile;
                            HitHorizWall();
						}
					}
					else
					{
						if (pwalldir == di_west) 
                            pwallposi = 63 - pwallpos;
                        else
                            pwallposi = pwallpos;

						if (pwalldir == di_east && (word)xintercept < (pwallposi << 10)
						 || pwalldir == di_west && (word)xintercept > (pwallposi << 10))
						{
							if (xinttile == pwallx && ytile == pwally)
							{												
								if (pwalldir == di_east && (word)xintercept + xstep < (pwallposi << 10)
								 || pwalldir == di_west && (word)xintercept + xstep > (pwallposi << 10))
									goto passhoriz;

								if (pwalldir == di_east)
									xintercept = ((long)xinttile << TILESHIFT) + (pwallposi << 10);
								else
									xintercept = ((long)xinttile << TILESHIFT) - TILEGLOBAL + (pwallposi << 10);
 		      	                
                                yintercept -= ((ystep * (63 - pwallpos)) >> 6);
                                yinttile = (short)(yintercept >> TILESHIFT);
								tilehit = pwalltile;
                                HitVertWall();
							}
							else
							{
								pwallofs = pwallposi << 10;
								yintercept = (long)ytile << TILESHIFT;
								tilehit = pwalltile;
                                HitHorizWall();
							}
						}
						else
						{
							if (xinttile == pwallx && ytile == pwally)
							{
								pwallofs = pwallposi << 10;
								yintercept = (long)ytile << TILESHIFT;
								tilehit = pwalltile;
                                HitHorizWall();
							}
							else
							{
								if (pwalldir == di_east && (word)xintercept + xstep > (pwallposi << 10)
								 || pwalldir == di_west && (word)xintercept + xstep < (pwallposi << 10))
									goto passhoriz;

								if (pwalldir == di_east)
									xintercept = ((long)xinttile << TILESHIFT) - ((63 - pwallpos) << 10);
								else
									xintercept = ((long)xinttile << TILESHIFT) + ((63 - pwallpos) << 10);
 		      	                
                                yintercept -= ((ystep * pwallpos) >> 6);
                                yinttile = (short)(yintercept >> TILESHIFT);
								tilehit = pwalltile;
                                HitVertWall();
							}
						}
					}
                }
                else
                {
                    yintercept = (long)ytile << TILESHIFT;
                    HitHorizWall();
                }

                break;
            }
passhoriz:
            spotvis[xinttile][ytile] = 1;
            ytile += ytilestep;
            xintercept += xstep;
            xinttile = (short)(xintercept >> TILESHIFT);
        }
    }
}


/*
====================
=
= WallRefresh
=
====================
*/

void WallRefresh (void)
{
//
// set up variables for this view
//
    viewangle = player->angle;
    midangle = viewangle * (FINEANGLES / ANGLES);
    viewsin = sintable[viewangle];
    viewcos = costable[viewangle];
    viewx = player->x - FixedMul(focallength,viewcos);
    viewy = player->y + FixedMul(focallength,viewsin);

    focaltx = viewx >> TILESHIFT;
    focalty = viewy >> TILESHIFT;

    xpartialdown = viewx & (TILEGLOBAL - 1);
    xpartialup = TILEGLOBAL - xpartialdown;
    ypartialdown = viewy & (TILEGLOBAL - 1);
    ypartialup = TILEGLOBAL - ypartialdown;

    lastside = -1;			// the first pixel is on a new wall
    AsmRefresh ();
    ScalePost ();           // no more optimization on last post
}

//==========================================================================

/*
========================
=
= ThreeDRefresh
=
========================
*/

void ThreeDRefresh (void)
{
//
// clear out the traced array
//
    _asm
    {
        mov edi,OFFSET spotvis
        xor eax,eax
        mov ecx,1024					// 64*64 / 4
        rep stosd
    }

    spotvis[player->tilex][player->tiley] = 1;

    bufferofs += screenofs;

//
// follow the walls from there to the right, drawwing as we go
//
    if (!drawplanes)
        VGAClearScreen ();

    WallRefresh ();

    if (drawplanes)
        DrawPlanes ();

//
// draw all the scaled images
//
    DrawScaleds();          // draw scaled stuff
    DrawPlayerWeapon ();    // draw player's hands

//
// show screen and time last cycle
//
    if (fizzlein)
    {
        FizzleFade((long)bufferofs - SCREENSEG,(long)displayofs - SCREENSEG + screenofs,viewwidth,viewheight,20,false);
        fizzlein = false;

        lasttimecount = TimeCount = 0;          // don't make a big tic count
    }
#ifndef REMDEBUG
    else
    {
        if (fpscounter)
        {
            fontnumber = 0;
            SETFONTCOLOR (14,bordercol);
            PrintX = 4;
            PrintY = 1;
            US_PrintSigned (fps);
            US_Print (" fps");
        }
    }
#endif

    bufferofs -= screenofs;
    displayofs = bufferofs;

    _asm
    {
        cli
        mov ecx,[displayofs]
        mov edx,CRTC_INDEX      // CRTC address register
        mov al,CRTC_STARTHIGH   // start address high register
        out dx,al
        inc edx
        mov al,ch
        out dx,al   	        // set the high byte
        sti
    }

    bufferofs += SCREENSIZE;
    if ((long)bufferofs > SCREENSEG + PAGE3START)
        bufferofs = (byte *)SCREENSEG;

#ifndef REMDEBUG
    if (fpscounter)
    {
        fpsframes++;
        fpstime += tics;

        if (fpstime > 35)
        {
            fpstime -= 35;
            fps = fpsframes << 1;
            fpsframes = 0;
        }
    }
#endif

    frameon += tics;
}


//===========================================================================

