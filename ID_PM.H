//
//	ID_PM.H
//	Header file for Id Engine's Page Manager
//

#ifndef	__ID_PM__
#define	__ID_PM__

#define PMPageShift 12
#define PMPageSize  (1 << PMPageShift)

#define	PM_GetSoundPage(v)	PM_GetPage(PMSoundStart + (v))
#define	PM_GetSpritePage(v)	PM_GetPage(PMSpriteStart + (v))

extern word     ChunksInFile;
extern word     PMSpriteStart;
extern word     PMSoundStart;
extern longword *PageOffsets;
extern word     *PageLengths;

void PM_Startup (void);
void PM_Shutdown (void);
byte *PM_GetPage (int pagenum);

#endif
