// ID_VL.H

// wolf compatability

//===========================================================================


#define SC_INDEX			0x3C4
#define SC_RESET			0
#define SC_CLOCK			1
#define SC_MAPMASK			2
#define SC_CHARMAP			3
#define SC_MEMMODE			4

#define CRTC_INDEX			0x3D4
#define CRTC_H_TOTAL		0
#define CRTC_H_DISPEND		1
#define CRTC_H_BLANK		2
#define CRTC_H_ENDBLANK		3
#define CRTC_H_RETRACE		4
#define CRTC_H_ENDRETRACE 	5
#define CRTC_V_TOTAL		6
#define CRTC_OVERFLOW		7
#define CRTC_ROWSCAN		8
#define CRTC_MAXSCANLINE 	9
#define CRTC_CURSORSTART 	10
#define CRTC_CURSOREND		11
#define CRTC_STARTHIGH		12
#define CRTC_STARTLOW		13
#define CRTC_CURSORHIGH		14
#define CRTC_CURSORLOW		15
#define CRTC_V_RETRACE		16
#define CRTC_V_ENDRETRACE 	17
#define CRTC_V_DISPEND		18
#define CRTC_OFFSET			19
#define CRTC_UNDERLINE		20
#define CRTC_V_BLANK		21
#define CRTC_V_ENDBLANK		22
#define CRTC_MODE			23
#define CRTC_LINECOMPARE 	24


#define GC_INDEX			0x3CE
#define GC_SETRESET			0
#define GC_ENABLESETRESET 	1
#define GC_COLORCOMPARE		2
#define GC_DATAROTATE		3
#define GC_READMAP			4
#define GC_MODE				5
#define GC_MISCELLANEOUS 	6
#define GC_COLORDONTCARE 	7
#define GC_BITMASK			8

#define ATR_INDEX			0x3c0
#define ATR_MODE			16
#define ATR_OVERSCAN		17
#define ATR_COLORPLANEENABLE 18
#define ATR_PELPAN			19
#define ATR_COLORSELECT		20

#define	STATUS_REGISTER_1    0x3da

#define PEL_WRITE_ADR		0x3c8
#define PEL_READ_ADR		0x3c7
#define PEL_DATA			0x3c9


//===========================================================================

#define SCREENSEG		0xa0000

#define SCREENWIDTH		80			// default screen width in bytes
#define MAXSCANLINES	200			// size of ylookup table

#define CHARWIDTH		2
#define TILEWIDTH		4

//===========================================================================

extern  byte        *bufferofs;			// all drawing is reletive to this
extern  byte        *displayofs;	    // last setscreen coordinates

extern	unsigned    linewidth;
extern	unsigned    ylookup[MAXSCANLINES];

extern	boolean		screenfaded;
extern	unsigned    bordercolor;

//===========================================================================

//
// VGA hardware routines
//

void VGAWRITEMODE(byte x);
#pragma aux VGAWRITEMODE = \
		"cli" \
		"mov edx,0x03ce" \
		"mov al,5" \
		"out dx,al" \
		"inc edx" \
		"in al,dx" \
		"and al,252" \
		"or al,ah" \
		"out dx,al" \
		"sti" \
		parm [ah] \
		modify exact [edx al]

void VL_WaitVBL(int num);
#pragma aux VL_WaitVBL = \
		"mov	edx,03dah" \
		"VBLActive:"\
		"in al,dx"\
		"test al,8"\
		"jnz VBLActive"\
		"noVBL:"\
		"in al,dx"\
		"test al,8"\
		"jz noVBL"\
		"loop VBLActive"\
		parm [ecx] \
		modify exact [edx al ecx]
/*
void VGAMAPBIT(byte x);
#pragma aux VGAMAPBIT = \
        "mov eax,0x0102"\
        "mov cl,x"\
        "shl ah,cl"\
        "mov edx,0x3c4"\
        "out dx,ax"\
        "parm [bl]\
        modify exact [edx ax]
*/
void VGAREADMAP(byte x);
#pragma aux VGAREADMAP = \
		"mov edx,0x3ce"\
		"mov al,4"\
		"mov ah,bl"\
		"out dx,ax"\
		parm [bl]\
		modify exact [edx ax]

void VGAWRITEMAP(byte x);
#pragma aux VGAWRITEMAP =      \
        "mov    eax,01H"       \
        "mov    edx,03c5h"     \
        "shl    eax,cl"        \
        "out    dx,al"         \
        parm    [ecx]          \
        modify exact [eax edx]

void VGAMAPMASK(byte x);
#pragma aux VGAMAPMASK = \
		"mov edx,0x3c4"\
		"mov al,2"\
		"mov ah,bl"\
		"out dx,ax"\
		parm [bl]\
		modify exact [edx ax]

void SetTextCursor(byte x, byte y);
#pragma aux SetTextCursor = \
		"xor	bh,bh" \
		"mov	ah,2" \
		"int 0x10" \
		parm [dl] [dh] \
		modify exact [bh eax]

void SetScreen (int offset);
#pragma aux SetScreen = \
		"cli" \
		"mov	edx,0x3d4" \
		"mov	al,0x0c" \
		"out	dx,al" \
		"inc	edx" \
		"mov	al,ah" \
		"out	dx,al" \
		"sti" \
		parm [eax] \
		modify exact [edx ax]

void my_outp(int port, byte data);
#pragma aux my_outp = \
        "out dx,al" \
        parm [edx] [al] \
        modify exact []

byte my_inp(int port);
#pragma aux my_inp = \
        "in al,dx" \
        parm [edx] \
        value [al] \
        modify exact [al]

#define outportb outp
#define inportb(x) ((byte)inp(x))

#define outp(a,b) my_outp(a,b)
#define inp(a) my_inp(a)

#define VL_SetScreen(crtc,pan) SetScreen(crtc)

void VL_Shutdown (void);

void VL_SetTextMode (void);
void VL_DePlaneVGA (void);
void VL_SetVGAPlaneMode (void);
void VL_ClearVideo (byte color);

void VL_SetLineWidth (unsigned width);

void VL_FillPalette (int red, int green, int blue);
void VL_SetColor	(int color, int red, int green, int blue);
void VL_GetColor	(int color, int *red, int *green, int *blue);
void VL_SetPalette (byte *palette);
void VL_GetPalette (byte *palette);
void VL_FadeOut (int start, int end, int red, int green, int blue, int steps);
void VL_FadeIn (int start, int end, byte *palette, int steps);

void VL_Plot (int x, int y, int color);
void VL_Hlin (unsigned x, unsigned y, unsigned width, int color);
void VL_Vlin (int x, int y, int height, int color);
void VL_Bar (int x, int y, int width, int height, int color);

void VL_MungePic (byte *source, unsigned width, unsigned height);
void VL_DrawPicBare (int x, int y, byte *pic, int width, int height);
void VL_MemToLatch (byte *source, int width, int height, unsigned dest);
void VL_ScreenToScreen (byte *source, byte *dest,int width, int height);
void VL_MemToScreen (byte *source, int width, int height, int x, int y);
void VL_MaskedToScreen (byte *source, int width, int height, int x, int y);
void VL_LatchToScreen (unsigned source, int width, int height, int x, int y);
