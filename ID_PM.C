// ID_PM.C

#include "ID_HEADS.H"
#include "WL_DEF.H"
#pragma hdrstop

/*
=============================================================================

                            GLOBAL VARIABLES

=============================================================================
*/

word     ChunksInFile;
word     PMSpriteStart;
word     PMSoundStart;
longword *PageOffsets;
word     *PageLengths;
byte     *PMPages;


/*
===================
=
= PM_StartUp
=
= Start up the Page Manager
=
===================
*/

void PM_Startup (void)
{
    int  i;
    FILE *file;
    char fname[13] = {"VSWAP."};

    strcat (fname,extension);
    file = fopen(fname,"rb");

    if (!file)
        CA_CannotOpen (fname);

    //
    // read in header variables
    //
    fread (&ChunksInFile,sizeof(word),1,file);
    fread (&PMSpriteStart,sizeof(word),1,file);
    fread (&PMSoundStart,sizeof(word),1,file);

    //
    // read in the chunk offsets
    //
    PageOffsets = (longword *)malloc(ChunksInFile * sizeof(longword));
    fread (PageOffsets,sizeof(longword),ChunksInFile,file);

    //
    // read in the chunk lengths
    //
    PageLengths = (word *)malloc(ChunksInFile * sizeof(word));
    fread (PageLengths,sizeof(word),ChunksInFile,file);

    //
    // TODO: Doesn't support variable page lengths as used by the sounds (page length always <= 4096 there)
    //
    PMPages = (byte *)malloc(ChunksInFile << PMPageShift);

    for (i = 0; i < ChunksInFile; i++)
    {
        fseek (file,PageOffsets[i],SEEK_SET);
        fread (&PMPages[i << PMPageShift],PMPageSize,1,file);
    }

    fclose (file);
}


/*
===================
=
= PM_GetPage
=
= Returns the address of the page
=
===================
*/

byte *PM_GetPage (int pagenum)
{
    if (pagenum < 0 || pagenum > ChunksInFile)
    {
        sprintf (str,"PM_GetPage: Tried to access illegal page: %i",pagenum);
        Quit (str);
    }

    return &PMPages[pagenum << PMPageShift];
}


/*
===================
=
= PM_ShutDown
=
= Shut down the Page Manager
=
===================
*/

void PM_Shutdown (void)
{
   free (PageOffsets);
   free (PageLengths);
   free (PMPages);
}
