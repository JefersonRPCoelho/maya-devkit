//-
// ==========================================================================
// Copyright 2020 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk
// license agreement provided at the time of installation or download,
// or which otherwise accompanies this software in either electronic
// or hard copy form.
// ==========================================================================
//+

#if defined (__APPLE__)
#include <stdlib.h>
#else
#include <malloc.h>
#endif 

#include <string.h>
#include "viewCapturePPM.h"

/*
 *  Memory allocation and other macros :
 */

#define StrAlloc(n)     ((char *) malloc((unsigned)(n)))

#define PicAlloc()             ((Pic *) malloc((unsigned)(sizeof(Pic))))
#define PicFree(p)             ((void) free((p)->filename), \
				(void) free((char *)(p)))


/*******************************************************************\

    Pic     * PicOpen(filename, width, height)

    Purpose:
        This function opens an image file and writes the
 	appropriate header for the given resolution.

 	The image file is an uncompressed raw PPM file, with 8 bits
 	for each of red, green and blue.  Only the resolution
 	is variable.

    Parameters:
        char    * filename (in) : name of image file to open.
        short     width (in)    : number of pixels per scanline.
        short     height (in)   : number of scanlines for image.

    Returned:
        Pic     * im_file       : file pointer for the image file,
 			          NULL if an error occured.

    Notes:
        Any error conditions are indicated by writing to
 	`stderr', in addition to returning the NULL file
 	pointer.

\*******************************************************************/
Pic *PicOpen(const char *filename, short width, short height)
{
    Pic     *ppmFile;
    FILE    *fptr;


    ppmFile = (Pic *) NULL;

    if (width <= 0) {
	fprintf(stderr,
		"ERROR: PicOpen() - `%d' is an invalid scanline width.\n",
		width);

    } else if (height <= 0) {
	fprintf(stderr,
		"ERROR: PicOpen() - `%d' is an invalid number of scanlines.\n",
		height);

    } else if ( 0 == (fptr = fopen(filename, "wb"))) {
	fprintf(stderr,
		"ERROR: PicOpen() - couldn't open file `%s' for writing.\n",
		filename);

    } else if (0 == (ppmFile = PicAlloc())) {
	fprintf(stderr,
		"ERROR: PicOpen() - cannot allocate `Pic' structure.\n");

    } else {
        /* setup the structure and write the header to the file */
	ppmFile->width    = width;
	ppmFile->height   = height;
	ppmFile->scanline = 0;

	ppmFile->fptr     = fptr;
	ppmFile->filename = StrAlloc(strlen(filename) + 1);
	(void) strcpy(ppmFile->filename, filename);

        /* P6 is a Raw PPM signiture and must be the first 2 bytes */
	fprintf(ppmFile->fptr, "P6\n# A Raw PPM file\n# width\n%d\n# height\n"
                "%d\n# max component value\n255\n",  
                ppmFile->width,
                ppmFile->height);
    }

    return(ppmFile);
}
/*** THE END ***/

/*******************************************************************\
                                                        	 
    boolean    PicWriteLine(ppmFile, pixels)			 
                                                        	 
    Purpose:							 
        This function writes the given scanline to the given	 
 	image file.						 
                                                        	 
    Parameters:							 
        Pic     *ppmFile (in) : name of image file to write.	 
        Pixel   *pixels(in)   : scanline of rendered pixels.	 
                                                        	 
    Returned:							 
        boolean   status : TRUE - scanline written, else FALSE	 
                                                        	 
    Notes:							 
        The scanline will not be written if the given image	 
 	file has been completed.				 
                                                        	 
 	Two passes are necessary, since the IM file format	 
 	requires that the number of spans in each scanline	 
 	be specified prior to the scanline.			 
                                                        	 
\*******************************************************************/

boolean PicWriteLine(Pic *ppmFile, Pic_Pixel *pixels)
{
    int i;
    /*
     *  Don't do anything if the file is already complete:
     */

    if (ppmFile->scanline == ppmFile->height) {
	fprintf(stderr,
	    "WARNING: PicWriteLine() - `%s' is complete, scanline ignored.\n",
	    ppmFile->filename);

	return(FALSE);
    }

    for (i = 0; i < ppmFile->width; ++i) {
        putc((char)pixels[i].r, ppmFile->fptr);
        putc((char)pixels[i].g, ppmFile->fptr);
        putc((char)pixels[i].b, ppmFile->fptr);
    }

    /*
     *  Don't forget to increment the current scanline:
     */

    ++ ppmFile->scanline;
    return(TRUE);
}
/*** THE END ***/

/*******************************************************************\
                                                        	 
    void     PicClose(ppmFile)				 
                                                        	 
    Purpose:							 
        This function closes an image file.			 
                                                        	 
    Parameters:							 
        Pic     *ppmFile (in) : image file to be closed.	 
                                                        	 
    Notes:							 
        A warning will be issued if the image file is		 
 	incomplete, ie) all scanlines have not yet been		 
 	written.						 
                                                        	 
\*******************************************************************/
void PicClose(Pic *ppmFile)
{

    if (ppmFile->scanline < ppmFile->height) {
	fprintf(stderr,
	    "WARNING: PicClose() - only %d of %d scanlines written to `%s'.\n",
	    ppmFile->scanline,
	    ppmFile->height,
	    ppmFile->filename);
    }

    fclose(ppmFile->fptr);

    PicFree(ppmFile);
}

/*** THE END ***/
