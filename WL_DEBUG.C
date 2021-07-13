// WL_DEBUG.C

#include "WL_DEF.H"
#pragma hdrstop

#ifdef DEBUGKEYS

/*
=============================================================================

						 LOCAL CONSTANTS

=============================================================================
*/

#define VIEWTILEX	(viewwidth/16)
#define VIEWTILEY	(viewheight/16)

/*
=============================================================================

						 GLOBAL VARIABLES

=============================================================================
*/


int DebugKeys (void);

/*
=============================================================================

						 LOCAL VARIABLES

=============================================================================
*/


int	maporgx;
int	maporgy;

void ViewMap (void);

//===========================================================================

#if 0

/*
==================
=
= DebugMemory
=
==================
*/

void DebugMemory (void)
{
    CenterWindow (16,7);

    US_CPrint ("Memory Usage");
    US_CPrint ("------------");
    US_Print ("Total     :");
    US_PrintUnsigned (mminfo.mainmem/1024);
    US_Print ("k\nFree      :");
    US_PrintUnsigned (MM_UnusedMemory()/1024);
    US_Print ("k\nWith purge:");
    US_PrintUnsigned (MM_TotalFree()/1024);
    US_Print ("k\n");
    VW_UpdateScreen();
    IN_Ack ();
}

//===========================================================================

#endif

/*
==================
=
= CountObjects
=
==================
*/

void CountObjects (void)
{
    int i,total,count,active,inactive,doors;
    objtype *obj;

    CenterWindow (16,7);
    active = inactive = count = doors = 0;

    US_Print ("Total statics :");
    total = laststatobj-&statobjlist[0];
    US_PrintUnsigned (total);

    US_Print ("\nIn use statics:");
    for (i=0;i<total;i++)
    {
        if (statobjlist[i].shapenum != -1)
            count++;
        else
            doors++;    //debug
    }
    US_PrintUnsigned (count);

    US_Print ("\nDoors         :");
    US_PrintUnsigned (doornum);

    for (obj=player->next;obj;obj=obj->next)
    {
        if (obj->active)
            active++;
        else
            inactive++;
    }

    US_Print ("\nTotal actors  :");
    US_PrintUnsigned (active+inactive);

    US_Print ("\nActive actors :");
    US_PrintUnsigned (active);

    VW_UpdateScreen();
    IN_Ack ();
}


//===========================================================================

/*
===================
=
= PictureGrabber
=
===================
*/

void PictureGrabber (void)
{
    char     *fname = "WSHOT000.BMP";
    int      file;
    byte     buffer[320];
    byte     *pal = &gamepal[0];
    int      i,x;
    longword col;
    byte     bmpheads[54] =
    {
        'B','M',0x8e,0x3f,0,0,0,0,0,0,0x36,4,0,0,
        40,0,0,0,0x40,1,0,0,200,0,0,0,1,0,8,0,0,0,0,0,
        0,0xfa,0,0,0xc4,0x0e,0,0,0xc4,0x0e,0,0,0,0,0,0,0,0,0,0
    };
         
    for (i = 0; i < 1000; i++)
    {
        fname[7] = (i % 10) + '0';
        fname[6] = ((i / 10) % 10) + '0';
        fname[5] = (i / 100) + '0';

        file = open(fname,O_BINARY | O_RDONLY);

        if (file == -1)
            break;

        close (file);
    }

    //
    // overwrites WSHOT999.BMP if all wshot files exist
    //
    file = open(fname,O_CREAT | O_BINARY | O_WRONLY, S_IREAD | S_IWRITE | S_IFREG);

    if (file == -1)
    {
        CenterWindow (22,3);
        US_Print ("Unable to create file:\n");
        US_Print (fname);
        VW_UpdateScreen();
        IN_Ack();
        return;
    }

    write (file,bmpheads,sizeof(bmpheads));

    //
    // 1024 bytes palette
    //
    for (i = 0; i < 256; i++)
    {
        col = (((((longword)pal[i * 3]) << 10) +
             (((longword)pal[(i * 3) + 1]) << 2)) << 8) +
             (((longword)pal[(i * 3) + 2]) << 2);

        write (file,&col,4);
    }

    //
    // 64000 bytes image data
    //
    for (i = 199; i >= 0; i--)
    {
        for (x = 0; x < 320; x++)
        {
            VGAREADMAP (x & 3);

            buffer[x] = *(bufferofs + ylookup[i] + (x >> 2));
        }

        write (file,buffer,320);
    }

    close (file);

    CenterWindow (18,2);
    US_PrintCentered ("Screenshot taken");
    VW_UpdateScreen();
    IN_Ack();
}


//===========================================================================


/*
================
=
= ShapeTest
=
================
*/

void ShapeTest (void)
{
extern  word    NumDigi;
extern  word    *DigiList;
static  char    buf[10];

    boolean         done;
    ScanCode        scan;
    int             i,j,k,x;
    byte            v;
    int             v2;
    longword        l;
    byte            *addr,*dp;
    short           sound;

    CenterWindow(20,16);
    VW_UpdateScreen();
    for (i = 0,done = false;!done;)
    {
        US_ClearWindow();
        sound = -1;

        US_Print(" Page #");
        US_PrintUnsigned(i);
        if (i < PMSpriteStart)
            US_Print(" (Wall)");
        else if (i < PMSoundStart)
            US_Print(" (Sprite)");
        else if (i == ChunksInFile - 1)
            US_Print(" (Sound Info)");
        else
            US_Print(" (Sound)");

        US_Print("\n Address: ");
        addr = PM_GetPage(i);
        sprintf(buf,"0x%08X",(longword)addr);
        US_Print(buf);

        if (addr)
        {
            if (i < PMSpriteStart)
            {
            //
            // draw the wall
            //
                bufferofs += 32*SCREENWIDTH;
                postx = 128;
                postwidth = 1;
                postsource = addr;
                for (x=0;x<64;x++,postx++,postsource+=64)
                {
                    wallheight[postx] = 256;
                    ScalePost ();
                }
                bufferofs -= 32*SCREENWIDTH;
            }
            else if (i < PMSoundStart)
            {
            //
            // draw the sprite
            //
                bufferofs += 32*SCREENWIDTH;
                SimpleScaleShape (160, i-PMSpriteStart, 64);
                bufferofs -= 32*SCREENWIDTH;
            }
            else if (i == ChunksInFile - 1)
            {
                US_Print("\n\n Number of sounds: ");
                US_PrintUnsigned(NumDigi);
                for (l = j = k = 0;j < NumDigi;j++)
                {
                    l += DigiList[(j * 2) + 1];
                    k += (DigiList[(j * 2) + 1] + (PMPageSize - 1)) / PMPageSize;
                }
                US_Print("\n Total bytes: ");
                US_PrintUnsigned(l);
                US_Print("\n Total pages: ");
                US_PrintUnsigned(k);
            }
            else
            {
                dp = addr;
                for (j = 0;j < NumDigi;j++)
                {
                    k = (DigiList[(j * 2) + 1] + (PMPageSize - 1)) / PMPageSize;
                    if ((i >= PMSoundStart + DigiList[j * 2]) && (i < PMSoundStart + DigiList[j * 2] + k))
                        break;
                }
                if (j < NumDigi)
                {
                    sound = j;
                    US_Print("\n Sound #");
                    US_PrintUnsigned(j);
                    US_Print("\n Segment #");
                    US_PrintUnsigned(i - PMSoundStart - DigiList[j * 2]);
                }
                for (j = 0;j < PageLengths[i];j += 32)
                {
                    v = dp[j];
                    v2 = (unsigned)v;
                    v2 -= 128;
                    v2 /= 4;
                    if (v2 < 0)
                        VWB_Vlin(WindowY + WindowH - 32 + v2,
                                WindowY + WindowH - 32,
                                WindowX + 8 + (j / 32),BLACK);
                    else
                        VWB_Vlin(WindowY + WindowH - 32,
                                WindowY + WindowH - 32 + v2,
                                WindowX + 8 + (j / 32),BLACK);
                }
            }
        }

        VW_UpdateScreen();

        while ((scan = LastScan) == 0)
            SD_Poll();

        IN_ClearKey(scan);
        switch (scan)
        {
            case sc_LeftArrow:
                if (i)
                    i--;
                break;
            case sc_RightArrow:
                if (++i >= ChunksInFile)
                    i--;
                break;
            case sc_W:  // Walls
                i = 0;
                break;
            case sc_S:  // Sprites
                i = PMSpriteStart;
                break;
            case sc_D:  // Digitized
                i = PMSoundStart;
                break;
            case sc_I:  // Digitized info
                i = ChunksInFile - 1;
                break;
            case sc_P:
                if (sound != -1)
                    SD_PlayDigitized(sound,8,8);
                break;
            case sc_Escape:
                done = true;
                break;
        }
    }
    SD_StopDigitized();
}


//===========================================================================


/*
================
=
= DebugKeys
=
================
*/

int DebugKeys (void)
{
    byte    start,end;
    boolean esc;
    int level,i;
    int main,extra,free;

    if (Keyboard[sc_B])     // B = border color
    {
        CenterWindow(20,3);
        PrintY+=6;
        US_Print(" Border color (0-56): ");
        VW_UpdateScreen();
        esc = !US_LineInput (px,py,str,NULL,true,2,0);
        if (!esc)
        {
            level = atoi (str);
            if (level>=0 && level<=99)
            {
                if (level < 30)
                    level += 31;
                else
                {
                    if (level > 56)
                        level = 31;
                    else
                        level -= 26;
                }
                        
                bordercol = (level * 4) + 3;

                if (bordercol == VIEWCOLOR)
                    DrawAllStatusBorder(bordercol);
                
                DrawAllPlayBorder();
                        
                return 0;
            }
        }
        return 1;
    }

    if (Keyboard[sc_C])     // C = count objects
    {
        CountObjects();
        return 1;
    }

    if (Keyboard[sc_D])     // D = FPS counter
    {
        CenterWindow (12,2);
        if (fpscounter)
            US_PrintCentered ("FPS Counter OFF");
        else
            US_PrintCentered ("FPS Counter ON");
        VW_UpdateScreen();
        IN_Ack();
        fpscounter ^= 1;
        return 1;
    }

    if (Keyboard[sc_E])     // E = quit level
    {
        if (tedlevel)
            Quit (NULL);
        playstate = ex_completed;
    }

    if (Keyboard[sc_F])     // F = facing spot
    {
		CenterWindow (14,6);
		US_Print ("X:");
		US_PrintUnsigned (player->x);
		US_Print ("\nY:");
		US_PrintUnsigned (player->y);
		US_Print ("\nA:");
		US_PrintUnsigned (player->angle);
		US_Print ("\nF:");
		US_PrintUnsigned (player->areanumber);
		VW_UpdateScreen();
		IN_Ack();
		return 1;
    }

    if (Keyboard[sc_G])     // G = god mode
    {
        CenterWindow (12,2);
        if (!godmode)
            US_PrintCentered ("God mode ON");
        else if (godmode == 1)
            US_PrintCentered ("God (no flash)");
        else
            US_PrintCentered ("God mode OFF");
    
        VW_UpdateScreen();
        IN_Ack();
        if (godmode != 2)
            godmode++;
        else
            godmode = 0;
        return 1;
    }

    if (Keyboard[sc_H])     // H = hurt self
    {
        IN_ClearKeysDown ();
        TakeDamage (16,NULL);
    }

    if (Keyboard[sc_I])     // I = item cheat
    {
        CenterWindow (12,3);
        US_PrintCentered ("Free items!");
        VW_UpdateScreen();
        GivePoints (100000);
        HealSelf (99);
        if (gamestate.bestweapon<wp_chaingun)
            GiveWeapon (gamestate.bestweapon+1);
        gamestate.ammo += 50;
        if (gamestate.ammo > 99)
            gamestate.ammo = 99;
        DrawAmmo ();
        IN_Ack ();
        return 1;
    }

    if (Keyboard[sc_K])     // K = give keys
    {
        CenterWindow(16,3);
        PrintY+=6;
        US_Print("  Give Key (1-4): ");
        VW_UpdateScreen();
        esc = !US_LineInput (px,py,str,NULL,true,1,0);
        if (!esc)
        {
            level = atoi (str);
            if (level>0 && level<5)
                GiveKey(level-1);
        }
        return 1;
    }

    if (Keyboard[sc_L])     // L = level ratios
    {
        end = LRpack;
        
        if (end == 8)   // wolf3d
        {
            CenterWindow(17,10);
            start = 0;
        }                        
        else            // sod
        {
            CenterWindow(17,12);
            start = 0;
            end = 10;
        }
again:
        for (i = start; i < end; i++)
        {
            US_PrintUnsigned(i+1);
            US_Print(" ");
            US_PrintUnsigned(LevelRatios[i].time/60);
            US_Print(":");
            if (LevelRatios[i].time%60 < 10)
                US_Print("0");
            US_PrintUnsigned(LevelRatios[i].time%60);
            US_Print(" ");
            US_PrintUnsigned(LevelRatios[i].kill);
            US_Print("% ");
            US_PrintUnsigned(LevelRatios[i].secret);
            US_Print("% ");
            US_PrintUnsigned(LevelRatios[i].treasure);
            US_Print("%\n");
        }

        VW_UpdateScreen();
        IN_Ack();

        if (end == 10 && gamestate.mapon > 9)
        {
            start = 10;
            end = 20;
            CenterWindow(17,12);
            goto again;
        }

        return 1;
    }

    if (Keyboard[sc_M])     // M = memory info
    {       
        CenterWindow (30,3);
        // put calculation stuff here
        main = 24;
        extra = 5;
        free = 32;
        US_Print (" Protected: approx. ");
        US_PrintUnsigned(extra);
        US_Print ("M used of ");
        US_PrintUnsigned(free);
        US_Print ("M\nMain/base: approx. ");
        US_PrintUnsigned(main);
        US_Print ("k used of 640k");
        VW_UpdateScreen();
        IN_Ack();
        return 1;
    }

    if (Keyboard[sc_N])     // N = no clip
    {
        noclip^=1;
        CenterWindow (18,3);
        if (noclip)
            US_PrintCentered ("No clipping ON");
        else
            US_PrintCentered ("No clipping OFF");
        VW_UpdateScreen();
        IN_Ack ();
        return 1;
    }

    if (Keyboard[sc_O])     // O = overhead
    {
        ViewMap();
        return 1;
    }

    if (Keyboard[sc_P])     // P = picture grabber
    {
        PictureGrabber();
        return 1;
    }

    if (Keyboard[sc_Q])     // Q = fast quit
        Quit (NULL);

    if (Keyboard[sc_S])     // S = slow motion
    {
        CenterWindow(30,3);
        PrintY+=6;
        US_Print(" Slow Motion steps (default 14): ");
        VW_UpdateScreen();
        esc = !US_LineInput(px,py,str,NULL,true,2,0);
        if (!esc)
        {
            level = atoi (str);
            if (level>=0 && level<=50)
                singlestep = level;
        }
        return 1;
    }

    if (Keyboard[sc_T])     // T = shape test
    {
        ShapeTest ();
        return 1;
    }

    if (Keyboard[sc_V])     // V = extra VBLs
    {
        CenterWindow(30,3);
        PrintY+=6;
        US_Print("  Add how many extra VBLs(0-8): ");
        VW_UpdateScreen();
        esc = !US_LineInput(px,py,str,NULL,true,1,0);
        if (!esc)
        {
            level = atoi (str);
            if (level>=0 && level<=8)
                extravbls = level;
        }
        return 1;
    }

    if (Keyboard[sc_W])     // W = warp to level
    {
        CenterWindow(26,3);
        PrintY+=6;
#ifndef SPEAR
        US_Print("  Warp to which level(1-10): ");
#else
        US_Print("  Warp to which level(1-21): ");
#endif
        VW_UpdateScreen();
        esc = !US_LineInput (px,py,str,NULL,true,2,0);
        if (!esc)
        {
            level = atoi (str);
#ifndef SPEAR
            if (level>0 && level<11)
#else
            if (level>0 && level<22)
#endif
            {
                gamestate.mapon = level-1;
                playstate = ex_warped;
            }
        }
        return 1;
    }

    if (Keyboard[sc_X])     // X = item cheat
    {
        CenterWindow (12,3);
        US_PrintCentered ("Extra stuff!");
        VW_UpdateScreen();
        // DEBUG: put stuff here
        IN_Ack ();
        return 1;
    }

    return 0;
}


/*
=============================================================================

						 OVERHEAD MAP

=============================================================================
*/

void DrawMapFloor (unsigned sx, unsigned sy, byte color)
{
    VWB_Bar (sx,sy,16,16,color);
}

void DrawMapWall (unsigned sx, unsigned sy, int wallpic)
{
    int  x,y;
    byte *src;

    src = PM_GetPage(wallpic);

    for (x = 0; x < 16; x++)
    {
     	VGAMAPMASK (1 << (x & 3));

        for (y = 0; y < 16; y++)
        {
            *(bufferofs + ylookup[sy + y] + ((sx + x) >> 2)) = *src;

            src += 4;
        }

        src += 192;
    }
}

void DrawMapDoor (unsigned sx, unsigned sy, int doornum)
{
    int doorpage;

    switch (doorobjlist[doornum].lock)
    {
        case dr_normal:
            doorpage = DOORWALL;
            break;
        case dr_lock1:
            doorpage = DOORWALL+7;
            break;
        case dr_lock2:
        case dr_lock3:
        case dr_lock4:
            doorpage = DOORWALL+6;
            break;
        case dr_elevator:
            doorpage = DOORWALL+5;
            break;
    }

    DrawMapWall (sx,sy,doorpage);
}

void DrawMapSprite (unsigned sx, unsigned sy, int shapenum)
{
    t_compshape *shape;
    byte        *src,*pixptr;
    short       start,end,source;
    int         x,xpos;
    word        *linecmds;

    src = PM_GetSpritePage(shapenum);
    shape = (t_compshape *)src;

    for (x = shape->leftpix; x <= shape->rightpix; x++)
    {
        xpos = sx + (x >> 2);
        VGAMAPMASK (1 << (xpos & 3));

        linecmds = (word *)&src[shape->dataofs[x - shape->leftpix]];

        while (*linecmds)
        {
            end = (*linecmds++) >> 1;
            source = *linecmds++;
            start = (*linecmds++) >> 1;

            for (pixptr = &src[source + start]; start != end; start++, pixptr++)
            {
                if (!(x & 3) && !(start & 3))
                    *(bufferofs + ylookup[sy + (start >> 2)] + (xpos >> 2)) = *pixptr;
            }
        }
    }
}


/*
===================
=
= OverheadRefresh
=
===================
*/

void OverheadRefresh (void)
{
    unsigned    x,y,endx,endy,sx,sy;
    unsigned    tile;
    int         startx,starty;
    int         shapenum;
    int         rotate[9] = {6,5,4,3,2,1,0,7,0};
    objtype     *obj;
    statobj_t   *spot;

    startx = (320 - viewwidth) >> 1;
    starty = (200 - STATUSLINES - viewheight) >> 1;

    endx = maporgx + VIEWTILEX;
    endy = maporgy + VIEWTILEY;

    for (y = maporgy; y < endy; y++)
    {
        for (x = maporgx; x < endx; x++)
        {
            sx = ((x - maporgx) << 4) + startx;
            sy = ((y - maporgy) << 4) + starty;

            tile = (unsigned)actorat[x][y];

            if (Keyboard[sc_F])
                DrawMapFloor (sx,sy,MAPSPOT(x,y,0) - AREATILE);
            else
                DrawMapFloor (sx,sy,0x19);

            if (tile)
            {
                if (tile < MAXWALLTILES)
                {
                    if (Keyboard[sc_Tab] && MAPSPOT(x,y,1) == PUSHABLETILE)
                        DrawMapFloor (sx,sy,15);
                    else
                        DrawMapWall (sx,sy,horizwall[tile]);
                }
                else if (tile < 256 && tile != 64)
                    DrawMapDoor (sx,sy,tile & 0x80);
                else if (tile == 64)
                {
                    for (spot = &statobjlist[0]; spot < laststatobj; spot++)
                    {
                        if (spot->shapenum != -1)
                        {
                            if (spot->tilex == x && spot->tiley == y)
                                DrawMapSprite (sx,sy,spot->shapenum);
                        }
                    }
                }
                else
                {
                    obj = (objtype *)tile;
                    shapenum = obj->state->shapenum;

                    if (obj->state->rotate)
                        shapenum += rotate[obj->dir];

                    DrawMapSprite (sx,sy,shapenum);
                }
            }
        }
    }

    VWB_Bar ((320 - viewwidth) >> 1,viewheight - 4,viewwidth,8,bordercol);
    VW_UpdateScreen ();

    if (Keyboard[sc_P])
        PictureGrabber ();
}


/*
===================
=
= ViewMap
=
===================
*/

void ViewMap (void)
{
    maporgx = player->tilex - VIEWTILEX/2;
    if (maporgx<0)
        maporgx = 0;
    if (maporgx>MAPSIZE-VIEWTILEX)
        maporgx=MAPSIZE-VIEWTILEX;
    maporgy = player->tiley - VIEWTILEY/2;
    if (maporgy<0)
        maporgy = 0;
    if (maporgy>MAPSIZE-VIEWTILEY)
        maporgy=MAPSIZE-VIEWTILEY;

    do
    {
//
// let user pan around
//
        PollControls ();
        if (controlx < 0 && maporgx>0)
            maporgx--;
        if (controlx > 0 && maporgx<mapwidth-VIEWTILEX)
            maporgx++;
        if (controly < 0 && maporgy>0)
            maporgy--;
        if (controly > 0 && maporgy<mapheight-VIEWTILEY)
            maporgy++;

        OverheadRefresh ();

    } while (!Keyboard[sc_Escape]);

    IN_ClearKeysDown ();
}

#endif
