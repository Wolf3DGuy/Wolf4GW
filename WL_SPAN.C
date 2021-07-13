// WL_SPAN.C

#include "WL_DEF.H"
#pragma hdrstop

#define	MAXVIEWHEIGHT	200

short	spanstart[MAXVIEWHEIGHT/2];

fixed	basedist[MAXVIEWHEIGHT/2];

byte    planepics[PMPageSize * 2];

int 	mr_rowofs;
int 	mr_count;
fixed	mr_xstep;
fixed	mr_ystep;
fixed	mr_xfrac;
fixed	mr_yfrac;
byte	*mr_dest;


/*
==============
=
= DrawSpans
=
= Height ranges from 0 (infinity) to centery (nearest)
=
==============
*/

void DrawSpans (int x1, int x2, int height)
{
	int		prestep;
	int	    startx,plane,startplane;
	fixed   length,stepscale;
	fixed	startxfrac,startyfrac;
	byte    *toprow;

	toprow = bufferofs + ylookup[centery - 1 - height];
	mr_rowofs = ylookup[(height << 1) + 1];

	length = basedist[height];
	stepscale = length / scale;

    mr_xstep = FixedMul(stepscale,viewsin);
    mr_ystep = -FixedMul(stepscale,viewcos);

    mr_xstep <<= 2;
    mr_ystep <<= 2;

    startxfrac = viewx + FixedMul(length,viewcos);
    startyfrac = -viewy + FixedMul(length,viewsin);

//
// draw two spans simultaneously
//
	plane = startplane = x1 & 3;
	prestep = centerx - x1;

	do
	{
		VGAMAPMASK (1 << plane);

		mr_xfrac = startxfrac - ((mr_xstep >> 2) * prestep);
		mr_yfrac = startyfrac - ((mr_ystep >> 2) * prestep);

		startx = x1 >> 2;
		mr_dest = &toprow[startx];
		mr_count = ((x2 - plane) >> 2) - startx + 1;
		x1++;
		prestep--;

		if (mr_count)
		{
            _asm
            {
                push ebp

                mov  ebp,[mr_rowofs]
                mov  ecx,[mr_count]

//
// build composite position
//
            	mov  eax,[mr_yfrac]
            	shl  eax,16
            	add  eax,[mr_xfrac]

//
// build composite step
//
            	mov  edx,[mr_ystep]
            	shl  edx,16
            	add  edx,[mr_xstep]

                mov  edi,[mr_dest]
                mov  esi,OFFSET planepics
//
// eax	packed x / y fractional values
// ebx	texture offset and pixel values
// ecx  loop counter
// edx	packed x / y step values
// esi  pictures
// edi	write pointer
// ebp	toprow to bottomrow delta
//
            pixelloop:
            	shld ebx,eax,22		    // shift y units in
            	shld ebx,eax,7		    // shift x units in and one extra bit
            	and  ebx,0x1ffe		    // mask off extra top bits and 0 low bit

            	add  eax,edx		    // frac += fracstep

            	mov  ebx,[esi+ebx]  	    // get two source pixels
            	mov  [edi],bl     	    // write ceiling pixel
            	mov  [edi+ebp],bh           // write floor pixel

            	inc  edi
            	dec  ecx
            	jnz  pixelloop

                pop  ebp
            }
        }

		plane = (plane + 1) & 3;

	} while (plane != startplane);
}


/*
===================
=
= SetPlaneViewSize
=
===================
*/

void SetPlaneViewSize (void)
{
	int y;

	for (y = 0; y < centery; y++)
    {
		if (y > 0)
			basedist[y] = ((GLOBAL1 / 2) * scale) / y;
	}
}


/*
===================
=
= LoadPlanes
=
===================
*/

void LoadPlanes (void)
{
    int  x;
    int  ceilingpage = 0,floorpage = 0;
    byte *src,*dest;

    switch (gamestate.mapon + (10 * gamestate.episode))
    {
        case 45:
            ceilingpage = 56;
            floorpage = 1;
            break;

        case 56:
            ceilingpage = 28;
            floorpage = 1;
            break;

        default:
            ceilingpage = 0;
            floorpage = 1;
    }

	src = PM_GetPage(ceilingpage);
	dest = &planepics[0];

	for (x = 0; x < PMPageSize; x++)
	{
		*dest = *src++;
		dest += 2;
	}

	src = PM_GetPage(floorpage);
	dest = &planepics[1];

	for (x = 0; x < PMPageSize; x++)
	{
		*dest = *src++;
		dest += 2;
	}
}
            

/*
===================
=
= DrawPlanes
=
===================
*/

void DrawPlanes (void)
{
	int	height,lastheight;
	int	x;

//
// loop over all columns
//
	lastheight = centery;

	for (x = 0; x < viewwidth; x++)
	{
		height = wallheight[x] >> 3;

		if (height < lastheight)
		{	
            //
            // more starts
            //
			do
			{
				spanstart[--lastheight] = x;
			} while (lastheight > height);
		}
		else if (height > lastheight)
		{	
            //
            // draw spans
            //
			if (height > centery)
				height = centery;

			while (lastheight < height)
			{
				DrawSpans (spanstart[lastheight],x - 1,lastheight);

				lastheight++;
            }
		}
	}

	height = centery;

	while (lastheight < height)
	{
		DrawSpans (spanstart[lastheight],x - 1,lastheight);

		lastheight++;
    }
}
