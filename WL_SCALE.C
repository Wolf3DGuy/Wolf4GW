// WL_SCALE.C

#include "WL_DEF.H"
#pragma hdrstop


/*
=============================================================================

						  GLOBALS

=============================================================================
*/

int			maxscale,maxscaleshl2;

/*
=============================================================================

						  LOCALS

=============================================================================
*/


/*
==========================
=
= SetupScaling
=
==========================
*/

void SetupScaling (int maxscaleheight)
{
	maxscaleheight/=2;			// one scaler every two pixels

	maxscale = maxscaleheight-1;
	maxscaleshl2 = maxscale<<2;
}

//
// Draw Column vars
//
fixed    dc_iscale;
fixed    dc_invscale;
fixed    sprtopoffset;
int      dc_yl;
int      dc_yh;

#define SFRACUNIT  0x10000
#define TEXTUREMID 0x200000

word     *linecmds;


/*
=======================
=
= ScaleMaskedPost
=
=======================
*/

void ScaleMaskedPost (byte *src, byte *dest)
{
    int      length;
	short    end,source,start;
	fixed    topscreen;
	fixed    bottomscreen;

	while (*linecmds)
	{
		end = (*(linecmds++)) >> 1;
        source = *(linecmds++);
		start = (*(linecmds++)) >> 1;

		length = end - start;
		topscreen = sprtopoffset + (dc_invscale * start);
		bottomscreen = topscreen + (dc_invscale * length);
		dc_yl = (topscreen + SFRACUNIT) >> 16;
		dc_yh = ((bottomscreen - 1) >> 16) + 1;

		if (dc_yh > viewheight)
			dc_yh = viewheight;

		if (dc_yl < 0)
			dc_yl = 0;

		if (dc_yl <= dc_yh)
            DrawMaskedPost (start,&src[source + start],dest);
	}
}


/*
=======================
=
= ScaleMaskedWidePost
=
=======================
*/

void ScaleMaskedWidePost (int x, int width, byte *src)
{
	byte *dest;
    byte ofs;
	byte mask;

    dest = bufferofs + (x >> 2);

	ofs = ((x & 3) << 3) + width - 1;
	outp (SC_INDEX+1,(byte)*((byte *)mapmasks1 + ofs));
	ScaleMaskedPost (src,dest);

	mask = (byte)*((byte *)mapmasks2 + ofs);

	if (!mask)
		return;

	dest++;
	outp (SC_INDEX+1,mask);
	ScaleMaskedPost (src,dest);

	mask = (byte)*((byte *)mapmasks3 + ofs);

	if (!mask)
		return;

	dest++;
	outp (SC_INDEX+1,mask);
	ScaleMaskedPost (src,dest);
}


/*
=======================
=
= ScaleShape
=
= Draws a compiled shape at [scale] pixels high
=
= each vertical line of the shape has a pointer to segment data:
= 	end of segment pixel*2 (0 terminates line) used to patch rtl in scaler
= 	top of virtual line with segment in proper place
=	start of segment pixel*2, used to jsl into compiled scaler
=	<repeat>
=
=======================
*/

void ScaleShape (int xcenter, int shapenum, int height)
{
	t_compshape *shape;
	byte        *src;
	fixed       frac;
	int         scale;
	int         width;
	int         x1,x2;
	int  		texturecolumn;
	int         lastcolumn;
	int         startx;
	int         xcent;
	int         swidth;

    scale = height >> 3;    // low 3 bits are fractional

    if (!scale || scale > maxscale)
        return;                     // too close or far away

	src = PM_GetSpritePage(shapenum);
	shape = (t_compshape *)src;

	dc_invscale = height << 8;
    xcent = (xcenter << 16) - (scale << 16);

//
// calculate edges of the shape
//
	x1 = (xcent + (shape->leftpix * dc_invscale)) >> 16;

	if (x1 >= viewwidth)
	    return;              // off the right side

	x2 = (xcent + ((shape->rightpix + 1) * dc_invscale)) >> 16;

	if (x2 < 0)
	    return;              // off the left side

	dc_iscale = (256 << 16) / height;
    sprtopoffset = (centery << 16) - FixedMul(TEXTUREMID,dc_invscale);
 
//
// store information in a vissprite
//
	if (x1 < 0)
	{
		frac = -x1 * dc_iscale;
		x1 = 0;
	}
	else
		frac = 0;

    if (x2 > viewwidth)
        x2 = viewwidth;

    swidth = shape->rightpix - shape->leftpix;

	if (height > 256)
	{
		width = 1;
		startx = 0;
		lastcolumn = -1;

		for (; x1 < x2; x1++, frac += dc_iscale)
		{
			if (wallheight[x1] >= height)
			{
				if (lastcolumn >= 0)
				{
					linecmds = (word *)&src[shape->dataofs[lastcolumn]];

					ScaleMaskedWidePost (startx,width,src);

					width = 1;
					lastcolumn = -1;
				}

				continue;
			}

			texturecolumn = frac >> 16;

			if (texturecolumn > swidth)
				texturecolumn = swidth;

			if ((x1 & 3) && texturecolumn == lastcolumn && width < 8)
			{
                width++;
				continue;
			}
			else
			{
				if (lastcolumn >= 0)
				{
					linecmds = (word *)&src[shape->dataofs[lastcolumn]];

					ScaleMaskedWidePost (startx,width,src);
					width = 1;
				    startx = x1;
				    lastcolumn = texturecolumn;
				}
                else
                {
				    startx = x1;
				    lastcolumn = texturecolumn;
                }
			}
		}

		if (lastcolumn != -1)
		{
			linecmds = (word *)&src[shape->dataofs[lastcolumn]];

			ScaleMaskedWidePost (startx,width,src);
		}
	}
	else
	{
		for (; x1 < x2; x1++, frac += dc_iscale)
		{
			if (wallheight[x1] >= height)
				continue;

			VGAMAPMASK (1 << (x1 & 3));

			texturecolumn = frac >> 16;

			if (texturecolumn > swidth)
				texturecolumn = swidth;

			linecmds = (word *)&src[shape->dataofs[texturecolumn]];

			ScaleMaskedPost (src,bufferofs + (x1 >> 2));
		}
	}
}



/*
=======================
=
= SimpleScaleShape
=
= NO CLIPPING, height in pixels
=
= Draws a compiled shape at [scale] pixels high
=
= each vertical line of the shape has a pointer to segment data:
= 	end of segment pixel*2 (0 terminates line) used to patch rtl in scaler
= 	top of virtual line with segment in proper place
=	start of segment pixel*2, used to jsl into compiled scaler
=	<repeat>
=
= Setup for call
= --------------
= GC_MODE			read mode 1, write mode 2
= GC_COLORDONTCARE  set to 0, so all reads from video memory return 0xff
= GC_INDEX			pointing at GC_BITMASK
=
=======================
*/

void SimpleScaleShape (int xcenter, int shapenum, int height)
{
	t_compshape	*shape;
	byte        *src;
	fixed       frac;
	int         scale;
	int         width;
	int         x1,x2;
	int	    	texturecolumn;
	int         lastcolumn;
	int         startx;
	int         xcent;
	int         swidth;

    scale = height >> 1;

	src = PM_GetSpritePage(shapenum);
	shape = (t_compshape *)src;

	dc_invscale = height << 10;
	
	xcent = (xcenter << 16) - (scale << 16);

//
// calculate edges of the shape
//
	x1 = ((xcent + (shape->leftpix * dc_invscale)) >> 16);

	if (x1 >= viewwidth)
		return;               // off the right side

	x2 = (xcent + ((shape->rightpix + 1) * dc_invscale)) >> 16;

	if (x2 < 0)
	    return;               // off the left side

	dc_iscale = (64 << 16) / height;
	sprtopoffset = (centery << 16) - FixedMul(TEXTUREMID,dc_invscale);

//
// store information in a vissprite
//
	if (x1 < 0)
		x1 = 0;

	frac = (TEXTUREMID - (shape->leftpix << 16)) + ((x1 - centerx) * dc_iscale);

    if (x2 > viewwidth)
        x2 = viewwidth;

	swidth = shape->rightpix - shape->leftpix;

	if (height > 64)
	{
		width = 1;
		startx = 0;
		lastcolumn = -1;

		for (; x1 < x2; x1++, frac += dc_iscale)
		{
			texturecolumn = frac >> 16;

			if (texturecolumn > swidth)
				texturecolumn = swidth;

			if ((x1 & 3) && texturecolumn == lastcolumn && width < 8)
			{
				width++;
				continue;
			}
			else
			{
				if (lastcolumn >= 0)
				{
					linecmds = (word *)&src[shape->dataofs[lastcolumn]];

					ScaleMaskedWidePost (startx,width,src);

					width = 1;
					startx = x1;
					lastcolumn = texturecolumn;
				}
				else
				{
					startx = x1;
					lastcolumn = texturecolumn;
				}
			}
		}

		if (lastcolumn != -1)
		{
			linecmds = (word *)&src[shape->dataofs[lastcolumn]];

			ScaleMaskedWidePost (startx,width,src);
		}
	}
	else
	{
		for (; x1 < x2; x1++, frac += dc_iscale)
	    {
			VGAMAPMASK (1 << (x1 & 3));

			texturecolumn = frac >> 16;

			if (texturecolumn > swidth)
				texturecolumn = swidth;

			linecmds = (word *)&src[shape->dataofs[texturecolumn]];

			ScaleMaskedPost (src,bufferofs + (x1 >> 2));
		}
	}
}

//
// bit mask tables for drawing scaled strips up to eight pixels wide
//
// down here so the STUPID inline assembler doesn't get confused!
//
byte	mapmasks1[4][8] = {
{1 ,3 ,7 ,15,15,15,15,15},
{2 ,6 ,14,14,14,14,14,14},
{4 ,12,12,12,12,12,12,12},
{8 ,8 ,8 ,8 ,8 ,8 ,8 ,8} };

byte	mapmasks2[4][8] = {
{0 ,0 ,0 ,0 ,1 ,3 ,7 ,15},
{0 ,0 ,0 ,1 ,3 ,7 ,15,15},
{0 ,0 ,1 ,3 ,7 ,15,15,15},
{0 ,1 ,3 ,7 ,15,15,15,15} };

byte	mapmasks3[4][8] = {
{0 ,0 ,0 ,0 ,0 ,0 ,0 ,0},
{0 ,0 ,0 ,0 ,0 ,0 ,0 ,1},
{0 ,0 ,0 ,0 ,0 ,0 ,1 ,3},
{0 ,0 ,0 ,0 ,0 ,1 ,3 ,7} };
