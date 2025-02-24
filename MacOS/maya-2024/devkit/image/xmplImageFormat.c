/*
 *+***********************************************************************
 *
 *  Module:
 *	xmplImageFormat.c
 *
 *  Purpose:
 *	This is the sample source code for an image DSO.
 *  The extension '.xmpl' is used for demonstration purposes.
 *
 *  Author/Date:
 */

/*
//
//-
// ==========================================================================
// Copyright 1995,2006,2008 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk
// license agreement provided at the time of installation or download,
// or which otherwise accompanies this software in either electronic
// or hard copy form.
// ==========================================================================
//+
//
*/


#ifdef _WIN32
#define STRDUP _strdup
#include <windows.h>
#else
#define STRDUP strdup
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <nl_types.h>
#endif /* _WIN32 */
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#ifndef _WIN32
#include <sys/times.h>
#endif /* _WIN32 */
#include <sys/stat.h>
#ifndef _WIN32
#include <pwd.h>
#endif /* _WIN32 */
#include <time.h>
#include "private/xmplImageFormat.h"


/*
 *  Entry points for DSO.  imageFormatString can be NULL if this is
 *  to be used with Composer only.
 */

char		*program		= "Wavefront";
char		*version		= IMF_PROTOCOL_CURRENT;
char		*type			= "image";
char		*imageKey		= "xmpl";
char		*imageExtension		= ".xmpl";
char		*imageName		= "ExampleImage";
char		*imageFormatString	= "%s.%04.4d%s";
char		*imageNameSyntax	= "Name.####.Ext";
int		imageAddExtension	= TRUE;
int		imageUsage		= IMF_C_GENERIC;
int		imageOrientation	= IMF_C_ORIENT_BOT_LEFT;
int		imageNumberOfLuts	= 0;
U_INT		imageBitsPerLut		= 0x00000000;
int		imageNumberOfChannels	= 3;
U_INT		imageBitsPerChannel	= 0x00000080;
int		imageNumberOfMattes	= 1;
U_INT		imageBitsPerMatte	= 0x00000080;
int		imageNumberOfZChannels	= 0;
U_INT		imageBitsPerZChannel	= 0x00000000;
int		imageSupportsActiveWindow= FALSE;
U_INT		imageAccess		= IMF_C_READ_RANDOM | IMF_C_WRITE;



/*
 *  This is an example of a user-written image file format.
 *  This is a minimal RLA-like format.
 *	* dump mode only
 *	* 8-bit integer rgb channels
 *	* optional 8-bit integer matte channel
 *	* support single images only within a file
 *	* no auxiliary, minimal documentation strings
 *
 *  This "xmpl" format might be thought of as a minimal RLA-like
 *  format.
 *
 *  A user-written format can have any disk storage layout, but must
 *  read and write to the 'canonical' internal Wavefront format.
 *
 *  Note that the conversion between formats does not (usually) preserve all
 *  fields.
 */


/*
 *  Structure that contains the required data for handling the file.
 */

typedef	struct		xmpl_file
{
    FILE		*xf_fp;		/* pointer to the file		*/
    int			xf_cur_scan;	/* current scanline		*/
    int			xf_num_chans;	/* number of channels		*/
    int			xf_size_mult;	/* size of the stored values	*/
    WINDOW_I		xf_im_win;	/* image window			*/
    POINTER		*xf_buff_ptr;	/* pointers to the line buffers */
					/* for read			*/
} XMPL_FILE;


/*
 *  Structure for the file header.
 */

#define	XMPL_MAGIC	0x7718

typedef	struct		xmpl_hdr
{
    U_SHORT		xh_magic;	/* Magic number			*/
    WINDOW_I		xh_im_win;	/* Left, right, bottom and top	*/
					/* image dimensions		*/
    WINDOW_I		xh_act_win;	/* Left, right, bottom and top	*/
					/* active window dimensions	*/
    short		xh_frame;	/* Which frame # was this image	*/
					/* when it was written		*/
    short		xh_chan_type;	/* Type for the colour channels	*/
    short		xh_chan_bits;	/* Number of bits in each color	*/
    short		xh_num_im_chan;	/* Number of color channels	*/
    short		xh_num_matte_chan;
					/* Number of matte channels	*/
    float		xh_gamma;	/* The gamma of the image	*/
    int			xh_usage;	/*				*/
    COLOR_XYZ_3F	xh_red_pri;	/* The CIE XYZ coordinates of	*/
				    	/* red primary			*/
    COLOR_XYZ_3F	xh_green_pri;	/* The CIE XYZ coordinates of	*/
				    	/* green primary		*/
    COLOR_XYZ_3F	xh_blue_pri;	/* The CIE XYZ coordinates of	*/
				    	/* blue primary			*/
    COLOR_XYZ_3F	xh_white_pt;	/* The CIE XYZ coordinates of	*/
				    	/* white point			*/
    char		xh_name[128];	/* The name of the image when	*/
				    	/* it was writen		*/
    char		xh_aspect[32];	/* The name of the aspect ratio	*/
				    	/* format of the image		*/
    char		xh_chan_format[32];
				    	/* The name of the color channel*/
					/* format of the image		*/
} XMPL_HDR;


/*
 *  Local functions and variables.
 */

static	int	xmpl_close(IMF_OBJECT	* imf);
static	FILE	*xmpl_open( IMF_INFO *info, char *access );
static	int	xmpl_scan_read(POINTER data, int scan, POINTER** line_buff);
static	int	xmpl_scan_write(POINTER data, int scan, POINTER* line_buff);

static	char	*xmpl_unknown = "unknown";

/*
 *+***********************************************************************
 *
 *  Synopsis:
 *	int		imageInit( void )
 *
 *  Purpose:
 *	Optional routine whicb is called once when the application
 *  first opens the plug-in. You can do things like:
 *
 *	- have a special license check so that the plug-in will
 *	  only be usable if you it passes your test.
 *	- internationalise the global symbols (e.g. imageName could
 *	  read "Bill's Image Format" in English, "Formatte d'Image
 *	  Guillaume" in French, etc. You could base this on the
 *	  locale.
 *	- modify or set some of the gobal symbols based on local
 *	  support such as enable writing only if IRIX 6.4 or higher
 *	  (i.e. by setting the imageAccess bitfield).
 *
 *  Parameters:
 *	None.
 *
 *  Returns:
 *	int	FALSE		: Error during initialisation.
 *	int	TRUE		: Successful initialisation.
 *
 *-***********************************************************************
 */


int	imageInit( void )
{
    ERR_printf( ERR__INFO, "imageInit, hello from the plug-in!\n" );

    return( TRUE );
}

/*
 *+***********************************************************************
 *
 *  Synopsis:
 *	BOOLEAN	imageIsFile( fn, fp )
 *
 *  Purpose:
 *	Checks if the specified image file is the type supported by this
 *  plug-in. Note that the application may choose one of two methods to
 *  call this routine, either via file pointer to an open file, or via
 *  the file name (in which case you must open and close the file
 *  yourself.
 *
 *	You should define this entry point if you are developing your
 *  plug-in for Maya.
 *
 *  Parameters:
 *	char	*fn	: (in)	Name of image file.
 *	FILE	*fp	: (in)	Pointer to open file. If NULL, you must
 *				open the file yourself using the `name'
 *				parameter.
 *
 *  Returns:
 *	int	FALSE	: This file is not of the type supported by this
 *			  plug-in.
 *	int	TRUE	: This file is of the type supported by this plug
 *			  -in.
 *
 *-***********************************************************************
 */


BOOLEAN		imageIsFile( char *fn, FILE *fp )
{
    U_SHORT	magic;		/* Magic number read from file		*/
    BOOLEAN	was_open;	/* TRUE if mode where file opened by app*/


    was_open = ( fp != NULL );

    if ( fp )
    {
	/*
	 *  The application is calling us in the mode where the file is
	 *  pre-opened.
	 */

	rewind( fp );
    }
    else
    {
	/*
	 *  The application is calling us in the mode where we must open
	 *  the file ourselves.
	 */

#ifdef _WIN32
	if ( ( fp = fopen( fn, "rb" ) ) == NULL )
#else
	if ( ( fp = fopen( fn, "r" ) ) == NULL )
#endif /* _WIN32 */
	{
	    ERR_printf( ERR__ERROR, "imageIsFile, can't open `%s'. %s\n",
		    fn, strerror( errno ) );
	    return( FALSE );
	}
    }


    /*
     *  Read the first 2 bytes, which is the magic number.
     */

    if ( fread( (U_SHORT *) &magic, sizeof( magic ), 1, fp ) != 1 )
    {
	ERR_printf( ERR__ERROR, "imageIsFile, can't read `%s'. %s\n",
		fn, strerror( errno ) );
    if ( !was_open )
    {
        fclose( fp );
    }
    return( FALSE );
    }
    
    if ( !was_open )
    {
	fclose( fp );
    }


    /*
     *  Return TRUE if the magic number matches. You should probably
     *  be a bit more sophisticated here.
     */

    return( magic == XMPL_MAGIC );
}

/*
 *+***********************************************************************
 *
 *  Synopsis:
 *	int		imageReadOpen( imf )
 *
 *  Purpose:
 *	Open an example IMF format image for reading.
 *
 *  Parameters:
 *	IMF_OBJECT	*imf	: (mod)	The image file object
 *
 *  Returns:
 *	int	FALSE		: Error during open.
 *	int	TRUE		: Successful completion of open.
 *
 *-***********************************************************************
 */


int		imageReadOpen( IMF_OBJECT *imf )
{
    FMT_ASPECT_INFO	*fmt_info;	/* Entry in aspect table	*/
    XMPL_HDR		hdr;		/* Header we're writing to file	*/
    IMF_INFO		*info;		/* Sub-struct inside `imf'	*/
#ifndef _WIN32
    static char		*month[] =
	    {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
	    };
    char		date[256];	/* Resuling date file last chngd*/
    struct  passwd	*passwd_ptr;	/* For extracting file owner id	*/
    struct  stat	stat_buf;	/* For extracting date on file	*/
    struct  tm		*tm;		/* For extracting date on file	*/
#endif /* _WIN32 */
    XMPL_FILE		*xmpl_file;	/* Our pertinent info on file	*/
    XMPL_FILE		**xmpl_file_ptr;/* Our pertinent info on file	*/


    ERR_printf( ERR__INFO, "imageReadOpen, hello from the plug-in!\n" );


    /*
     *  Allocate the local data structure for the file.
     */

    xmpl_file_ptr = NULL;
    xmpl_file = NULL;

    if ( ( xmpl_file_ptr = (XMPL_FILE **) malloc( sizeof( XMPL_FILE * ) ) )
	    == NULL )
    {
#ifndef _WIN32
	imf__err = IMF_C_MEM_ALLOC;
#endif	/* _WIN32 */
	imf->data = NULL;
	goto open_err;
    }
    imf->data = (POINTER *) xmpl_file_ptr;


    /*
     *  Assign pointers into the appropriate points in the buffer.
     */

    if ( ( xmpl_file = (XMPL_FILE *) malloc( sizeof( XMPL_FILE ) ) )
	    == NULL )
    {
#ifndef _WIN32
	imf__err = IMF_C_MEM_ALLOC;
#endif	/* _WIN32 */
	xmpl_file_ptr[0] = NULL;
	goto open_err;
    }
    xmpl_file_ptr[0] = xmpl_file;
    xmpl_file->xf_buff_ptr = NULL;

    imf->info.count = 1;	    /* force count to be one */
    if ( ( imf->info.image = (IMF_IMAGE *) malloc( sizeof( IMF_IMAGE ) ) )
	    == NULL )
    {
#ifndef _WIN32
	imf__err = IMF_C_MEM_ALLOC;
#endif	/* _WIN32 */
	goto open_err;
    }


    /*
     *  Initialize the image array structures.
     */

    (void) imf__init_ifd( imf );


    /*
     *  Complete filling in the imf_data structure - load pointers to
     *  the write scan and the close routines.
     */

    imf->scan = (IMF_scanProc) xmpl_scan_read;
    imf->close = (IMF_closeProc) xmpl_close;
    if ( ( xmpl_file->xf_fp = xmpl_open( &imf->info,"r" ) ) == NULL )
    {
	goto open_err;
    }


    /*
     *  Read the file header and load the header information. Load the
     *  information into the XMPL_FILE structure and into the IMF_INFO
     *  structure of the image object.
     *
     *  NOTE:
     *	    If you copy you image files between machines with different
     *  alignment requirements or type sizes, you will need to read each
     *  element of the structure separately.  Also if the machines have
     *  different byte ordering or float point format, you will need to
     *  correct those element for the differences.
     */

    if ( fread( (char *) &hdr, sizeof( XMPL_HDR ), 1, xmpl_file->xf_fp )
	    != 1 )
    {
#ifndef _WIN32
	imf__err = IMF_C_READ_ERR;
#endif	/* _WIN32 */
	ERR_printf( ERR__ERROR,
		"imageReadOpen, can't read header. %s\n", strerror( errno ) );
	goto open_err;
    }
    else if ( hdr.xh_magic != XMPL_MAGIC )
    {
#ifndef _WIN32
	imf__err = IMF_C_READ_ERR;
#endif	/* _WIN32 */
	ERR_printf( ERR__ERROR,
		"imageReadOpen, not a `%s' image file. Got magic number %d\n",
		imageName, hdr.xh_magic );
	goto open_err;
    }


    /*
     *  Fill in info fields
     */

    info = &imf->info;
    IF_NULL_GOTO_ERROR( info->name = STRDUP( hdr.xh_name ), no_mem );
    IF_NULL_GOTO_ERROR( info->desc = STRDUP( "None" ), no_mem );
    IF_NULL_GOTO_ERROR( info->program = STRDUP( xmpl_unknown ), no_mem );
    IF_NULL_GOTO_ERROR( info->machine = STRDUP( xmpl_unknown ), no_mem );
#ifndef _WIN32
    if ( stat( info->handle, &stat_buf ) == 0 )
    {
	if ( ( passwd_ptr = getpwuid( stat_buf.st_uid ) ) != NULL )
	{
  	    IF_NULL_GOTO_ERROR( info->user = STRDUP( passwd_ptr->pw_name ),
		    no_mem );
	}
	else
	{
	    IF_NULL_GOTO_ERROR( info->user = STRDUP( xmpl_unknown ),
		    no_mem );
	}
	tm = localtime( &stat_buf.st_mtime );
  	(void) sprintf( date, "%s %d %02d:%02d %4d", month[tm->tm_mon],
		tm->tm_mday, tm->tm_hour, tm->tm_min, 1900 + tm->tm_year );
	IF_NULL_GOTO_ERROR( info->date = STRDUP( date ), no_mem );
    }
    else 
#endif /* _WIN32 */
    {
	IF_NULL_GOTO_ERROR( info->user = STRDUP( xmpl_unknown ), no_mem );
	IF_NULL_GOTO_ERROR( info->date = STRDUP( xmpl_unknown ), no_mem );
    }
    IF_NULL_GOTO_ERROR( info->time = STRDUP( xmpl_unknown ), no_mem );
    IF_NULL_GOTO_ERROR( info->filter = STRDUP( xmpl_unknown ), no_mem );
    IF_NULL_GOTO_ERROR( info->compress = STRDUP( "none" ), no_mem );

    info->frame = hdr.xh_frame;
    info->red_pri = hdr.xh_red_pri;
    info->green_pri = hdr.xh_green_pri;
    info->blue_pri = hdr.xh_blue_pri;
    info->white_pt = hdr.xh_white_pt;
    info->image[0].window = hdr.xh_im_win;
    info->image[0].active = hdr.xh_act_win;
    if ( ( fmt_info = FMT_find( hdr.xh_aspect ) ) != NULL )
    {
	info->image[0].aspect = *fmt_info;
	IF_NULL_GOTO_ERROR( info->image[0].aspect.name
		= STRDUP( fmt_info->name ), no_mem );
    }
    else
    {
	IF_NULL_GOTO_ERROR( info->image[0].aspect.name
		= STRDUP( hdr.xh_aspect ), no_mem );
	info->image[0].aspect.width
		= hdr.xh_im_win.right - hdr.xh_im_win.left +1;
	info->image[0].aspect.height
		= hdr.xh_im_win.top - hdr.xh_im_win.bottom +1;
	info->image[0].aspect.ratio
		= info->image[0].aspect.width
		/ (float) info->image[0].aspect.height;
    }
    info->image[0].curve.gamma = hdr.xh_gamma;
    info->image[0].curve.usage = hdr.xh_usage;

    info->image[0].chan_bits = hdr.xh_chan_bits;
    info->image[0].chan_count = hdr.xh_num_im_chan;
    IF_NULL_GOTO_ERROR( info->image[0].chan_format
	    = STRDUP( hdr.xh_chan_format ), no_mem );
    info->image[0].chan_type = hdr.xh_chan_type;


    /*
     *  Assumes matte channel is same bits & type as rgb channels.
     */

    info->image[0].matte_bits = hdr.xh_chan_bits;
    info->image[0].matte_count = hdr.xh_num_matte_chan;
    IF_NULL_GOTO_ERROR( info->image[0].matte_format
	    = STRDUP( xmpl_unknown ), no_mem );
    info->image[0].matte_type = hdr.xh_chan_type;

    info->image[0].aux_bits = hdr.xh_chan_bits;
    info->image[0].aux_count = 0;
    IF_NULL_GOTO_ERROR( info->image[0].aux_format
	    = STRDUP( xmpl_unknown ), no_mem );
    info->image[0].aux_type = hdr.xh_chan_type;

    xmpl_file->xf_size_mult
	    = xmpl_file->xf_num_chans
	    = hdr.xh_num_im_chan + hdr.xh_num_matte_chan;
    xmpl_file->xf_im_win = hdr.xh_im_win;
    xmpl_file->xf_cur_scan = hdr.xh_im_win.bottom;

    if ( ( xmpl_file->xf_buff_ptr = IMF_chan_alloc( info->image,
	    hdr.xh_im_win.right - hdr.xh_im_win.left + 1,
	    info->key, (int *) NULL ) ) != NULL )
    {
	/*
	 *  Successful completion of open for read, print out the
	 *  details of the file we opened, and then return TRUE.
	 */

	ERR_printf( ERR__INFO, "imageReadOpen, returning success!!!" );
	ERR_printf( ERR__INFO, "\txh_magic = 0x%x",hdr.xh_magic );
	ERR_printf( ERR__INFO, "\txh_im_win = (%d,%d,%d,%d)",
		hdr.xh_im_win.left, hdr.xh_im_win.right,
		hdr.xh_im_win.bottom, hdr.xh_im_win.top );
	ERR_printf( ERR__INFO, "\txh_act_win = (%d,%d,%d,%d)",
		hdr.xh_act_win.left, hdr.xh_act_win.right,
		hdr.xh_act_win.bottom, hdr.xh_act_win.top );
	ERR_printf( ERR__INFO, "\txh_frame = %d", hdr.xh_frame );
	ERR_printf( ERR__INFO, "\txh_chan_type = %d", hdr.xh_chan_type );
	ERR_printf( ERR__INFO, "\txh_chan_bits = %d", hdr.xh_chan_bits );
	ERR_printf( ERR__INFO, "\txh_num_im_chan = %d",
		hdr.xh_num_im_chan );
	ERR_printf( ERR__INFO, "\txh_num_matte_chan = %d",
		hdr.xh_num_matte_chan );
	ERR_printf( ERR__INFO, "\txh_gamma = %f", hdr.xh_gamma );
	ERR_printf( ERR__INFO, "\txh_usage = %d", hdr.xh_usage );
	ERR_printf( ERR__INFO, "\txh_red_pri = {%f,%f,%f}",
		hdr.xh_red_pri.x,
		hdr.xh_red_pri.y,
		hdr.xh_red_pri.z );
	ERR_printf( ERR__INFO, "\txh_green_pri = {%f,%f,%f}",
		hdr.xh_green_pri.x,
		hdr.xh_green_pri.y,
		hdr.xh_green_pri.z );
	ERR_printf( ERR__INFO, "\txh_blue_pri = {%f,%f,%f}",
		hdr.xh_blue_pri.x,
		hdr.xh_blue_pri.y,
		hdr.xh_blue_pri.z );
	ERR_printf( ERR__INFO, "\txh_white_pt = {%f,%f,%f}",
		hdr.xh_white_pt.x,
		hdr.xh_white_pt.y,
		hdr.xh_white_pt.z );
	ERR_printf( ERR__INFO, "\txh_name = `%s'", hdr.xh_name );
	ERR_printf( ERR__INFO, "\txh_aspect = `%s'", hdr.xh_aspect );
	ERR_printf( ERR__INFO, "\txh_chan_format = `%s'",
		hdr.xh_chan_format );

	return( TRUE );
    }

#ifndef _WIN32
    imf__err = IMF_C_MEM_ALLOC;
#endif	/* _WIN32 */

no_mem:
#ifndef _WIN32
    imf__err = IMF_C_READ_ERR;
#endif	/* _WIN32 */
    ERR_printf( ERR__ERROR,
	    "imageReadOpen, insufficient memory when reading file. %s\n",
	    strerror( errno ) );
    return( FALSE );

open_err:
    if ( xmpl_file->xf_fp != NULL )
    {
	(void) fclose( xmpl_file->xf_fp );
	xmpl_file->xf_fp = NULL;
    }

    return( FALSE );
}

/*
 *+***********************************************************************
 *
 *  Synopsis:
 *	int	imageWriteOpen( imf )
 *
 *  Purpose:
 *	Open an image file for write.
 *
 *  Parameters:
 *	IMF_OBJECT	*imf	: (mod)	The image file object
 *
 *  Returns:
 *	int	FALSE		: An error occurred when trying to open
 *				  the image for write.
 *	int	TRUE		: Successfully opened the image for write.
 *
 *-***********************************************************************
 */


int		imageWriteOpen( IMF_OBJECT *imf )
{
    XMPL_HDR	hdr;			/* Header read from file	*/
    XMPL_FILE	*xmpl_file = 0;		/* Our pertinent info on file	*/
    XMPL_FILE	**xmpl_file_ptr = 0;	/* Our pertinent info on file	*/


    /*
     *  Various tests to ensure incoming imf image is reasonable
     */

    if ( imf->info.count != 1 )
    {
	ERR_printf( ERR__WARNING,
		"imageWriteOpen, Writing first image only of '%d' images\n",
		imf->info.count );
	imf->info.count = 1;
    }
    if ( imf->info.image[0].chan_bits != 8 )
    {
	ERR_printf( ERR__ERROR,
		"imageWriteOpen, Xmpl format cannot write %d-bit channels\n",
		imf->info.image[0].chan_bits );
#ifndef _WIN32
	imf__err = IMF_C_NO_SUPPORT;
#endif	/* _WIN32 */
	goto open_err;
    }
    if ( imf->info.image[0].chan_type != IMF_C_INTEGER )
    {
	ERR_printf( ERR__ERROR,
		"imageWriteOpen, Xmpl format cannot write type '%d' channels\n",
		imf->info.image[0].chan_type );
#ifndef _WIN32
	imf__err = IMF_C_NO_SUPPORT;
#endif	/* _WIN32 */
	goto open_err;
    }


    /*
     *  Allocate the local data structure for the file.
     */

    xmpl_file_ptr = NULL;
    xmpl_file = NULL;

    if ( ( imf->data = (POINTER *) malloc(
	    imf->info.count * sizeof( XMPL_FILE * ) ) ) == NULL )
    {
#ifndef _WIN32
	imf__err = IMF_C_MEM_ALLOC;
#endif	/* _WIN32 */
	goto open_err;
    }

    xmpl_file_ptr = (XMPL_FILE **) imf->data;
    if ( ( *xmpl_file_ptr = (XMPL_FILE *) malloc(
	    imf->info.count * sizeof( XMPL_FILE ) ) ) == NULL )
    {
#ifndef _WIN32
	imf__err = IMF_C_MEM_ALLOC;
#endif	/* _WIN32 */
	goto open_err;
    }
    xmpl_file = *xmpl_file_ptr;
    xmpl_file->xf_buff_ptr = NULL;


    /*
     *  Complete filling in the imf_data structure - load pointers to
     *  the write scan and the close routines.
     */

    imf->scan =  (IMF_scanProc) xmpl_scan_write;
    imf->close = (IMF_closeProc) xmpl_close;
    if ( ( xmpl_file->xf_fp = xmpl_open( &imf->info, "w") ) == NULL )
    {
	goto open_err;
    }


    /*
     *  Fill in the XMPL_FILE data structure -- allocate and define local
     *  buffers, resolutions etc. as required for this file type.
     */

    xmpl_file->xf_im_win = imf->info.image[0].window;
    xmpl_file->xf_cur_scan = xmpl_file->xf_im_win.bottom;


    /*
     *  Aux channel is not written.
     */

    xmpl_file->xf_num_chans = imf->info.image[0].chan_count
	    + imf->info.image[0].matte_count;


    /*
     *  Build the header and write the header for the file.
     */

    hdr.xh_magic = XMPL_MAGIC;
    hdr.xh_im_win = imf->info.image[0].window;
    hdr.xh_act_win = imf->info.image[0].active;
    hdr.xh_frame = imf->info.frame;
    hdr.xh_chan_bits = xmpl_file->xf_size_mult
	    = imf->info.image[0].chan_bits;
    hdr.xh_num_im_chan = imf->info.image[0].chan_count;
    hdr.xh_num_matte_chan = imf->info.image[0].matte_count;
    hdr.xh_gamma = imf->info.image[0].curve.gamma;
    hdr.xh_usage = imf->info.image[0].curve.usage;
    hdr.xh_red_pri = imf->info.red_pri;
    hdr.xh_green_pri = imf->info.green_pri;
    hdr.xh_blue_pri = imf->info.blue_pri;
    hdr.xh_white_pt = imf->info.white_pt;
    hdr.xh_chan_type = imf->info.image[0].chan_type;
    (void) strncpy( hdr.xh_name, imf->info.handle, 128 );
    hdr.xh_name[127] = '\0';
    (void) strncpy( hdr.xh_aspect, imf->info.image[0].aspect.name, 32 );
    hdr.xh_aspect[31] = '\0';
    (void) strncpy( hdr.xh_chan_format, imf->info.image[0].chan_format, 32 );
    hdr.xh_chan_format[31] = '\0';

    if ( fwrite( (char *) &hdr, sizeof( XMPL_HDR ), 1, xmpl_file->xf_fp )
	    != 1 )
    {
#ifndef _WIN32
	imf__err = IMF_C_WRITE_ERR;
#endif	/* _WIN32 */
	ERR_printf( ERR__ERROR, "imageWriteOpen, can't write. %s\n",
		strerror( errno ) );
	goto open_err;
    }


    /*
     *  Successful compeltion of open for write, return TRUE.
     */

    return( TRUE );

open_err:
    if ( xmpl_file != NULL && xmpl_file->xf_fp != NULL )
    {
	(void) fclose( xmpl_file->xf_fp );
	xmpl_file->xf_fp = NULL;
    }

    return( FALSE );
}

/*
 *+***********************************************************************
 *
 *  Synopsis:
 *	static	FILE	*xmpl_open( info, access )
 *
 *  Purpose:
 *	A front end to the "real" IMF_open which knows how to use the
 *  handle,key and extensions provided in the info fields.
 *
 *  Parameters:
 *	IMF_INFO	*info	: (in)	Info on what file to open
 *	char		*access	: (in)	Read or write access flag,
 *					"r" or "w".
 *
 *  Returns:
 *	FILE	NULL		: Could not open the file because of error.
 *	FILE	*fp		: The file open was successful.
 *
 *-***********************************************************************
 */


static FILE	*xmpl_open( IMF_INFO *info, char *access )
{
    char	*filename;		/* Name of file to open		*/
    FILE	*fp;			/* File pointer			*/


    if ( strcmp( access, "r" ) == 0 )
    {
	if ( info->handle_complete )
	{
	    filename = info->handle;
	    if ( ( fp = fopen( filename,
#ifdef _WIN32
		    "rb"
#else  /* _WIN32 */
		   "r"
#endif /* _WIN32 */
		    ) ) == NULL )
	    {
#ifndef _WIN32
		imf__err = IMF_C_CANNOT_OPEN;
#endif	/* _WIN32 */
		goto open_err;
	    }
	}
	else
	{
	    filename = imf__build_handle( NULL, info->handle, info->ext );
	    if ( ( fp = fopen( filename,
#ifdef _WIN32
		    "rb"
#else  /* _WIN32 */
		   "r"
#endif /* _WIN32 */
		    ) ) == NULL )
	    {
		filename = imf__build_handle( getenv( "WF_IMG_DIR" ),
			info->handle, info->ext );
		if ( ( fp = fopen( filename,
#ifdef _WIN32
			"rb"
#else  /* _WIN32 */
		       "r"
#endif /* _WIN32 */
			) ) == NULL )
		{
#ifndef _WIN32
		    imf__err = IMF_C_CANNOT_OPEN;
#endif	/* _WIN32 */
		    goto open_err;
		}
	    }
	}
    }
    else
    {
	filename = info->handle_complete ? info->handle
		: imf__build_handle( getenv( "WF_IMG_DIR" ), info->handle,
		    info->ext );
	if ( ( fp = fopen( filename,
#ifdef _WIN32
		"wb"
#else  /* _WIN32 */
	       "w"
#endif /* _WIN32 */
		) ) == NULL )
	{
#ifndef _WIN32
	    imf__err = IMF_C_CANNOT_OPEN;
#endif	/* _WIN32 */
	    goto open_err;
	}
    }

    return( fp );

open_err:
    ERR_printf( ERR__ERROR, "xmpl_open, can't open file %s. %s\n",
	    filename, strerror( errno ) );

    return( NULL );
}

/*
 *+***********************************************************************
 *
 *  Synopsis:
 *	static	int	xmpl_close( imf )
 *
 *  Purpose:
 *	Closes an image file.
 *
 *  Parameters:
 *	IMF_OBJECT	*imf	: (in)	Image file pointer
 *
 *  Returns:
 *	BOOLEAN	IMF_C_NORMAL	: Always.
 *
 *-***********************************************************************
 */


static int	xmpl_close( IMF_OBJECT *imf )
{
    XMPL_FILE	*xmpl_file;		/* Our pertinent info on file	*/


    /*
     *  Free all of the space allocated with the open routine.
     */

    xmpl_file = *(XMPL_FILE **) imf->data;
    (void) IMF_chan_free( xmpl_file->xf_buff_ptr );
    xmpl_file->xf_buff_ptr = NULL;
    (void) fclose( xmpl_file->xf_fp );
    (void) imf__free_obj( imf );

    return( IMF_C_NORMAL );
}

/*
 *+***********************************************************************
 *
 *  Synopsis:
 *	static	int	xmpl_scan_read( data, scan, line_buff )
 *
 *  Purpose:
 *	Read a scanline from an image file ans decode the line.
 *
 *  Parameters:
 *	POINTER	data		: (in)	pointer to the image file data
 *	int	scan		: (in)	Scan to be read
 *	POINTER	**line_buff	: (in)	Pointer to the decoded scanline
 *
 *  Returns:
 *	int	IMF_C_BAD_SCAN	: Reading scanline out side of image.
 *	int	IMF_C_NORMAL	: Scanline read was successful.
 *	int	IMF_C_READ_ERR	: Read error on file.
 *
 *-***********************************************************************
 */


static	int	xmpl_scan_read( POINTER data, int scan, POINTER** line_buff )
{
    int		ct;			/* Channel counter		*/
    long	offset;			/* Offset into file		*/
    int		scan_bytes;		/* Bytes per row in the file	*/
    int		width;			/* Width of a row in the file	*/
    XMPL_FILE	*xmpl_file;		/* Our pertinent info on file	*/


    xmpl_file = (XMPL_FILE *) data;
    scan_bytes = xmpl_file->xf_im_win.right - xmpl_file->xf_im_win.left + 1;
    width = scan_bytes * xmpl_file->xf_size_mult;
    offset = scan * width + sizeof( XMPL_HDR );


    /*
     *  Check for a valid scanline -- position file for read is required.
     */

    if ( scan >= xmpl_file->xf_im_win.bottom
	    && scan <= xmpl_file->xf_im_win.top )
    {
	/*
	 *  Load the address of the vector of pointers to the line buffers
	 *  into line buff.
	 */

	*line_buff = xmpl_file->xf_buff_ptr;
	if ( scan != xmpl_file->xf_cur_scan )
	{
	    (void) fseek( xmpl_file->xf_fp, offset, SEEK_SET );
	}
	for ( ct=0; ct < xmpl_file->xf_num_chans; ct++ )
	{
	    /*
	     *  Read the data for the channel and load into one of the
	     *  line buffers.
	     */

	    if ( fread( (char *) (*line_buff)[ct], scan_bytes, 1,
		    xmpl_file->xf_fp ) != 1 )
	    {
		ERR_printf( ERR__ERROR,
			"xmpl_scan_read, can't read. %s\n",
			strerror( errno ) );
		return( IMF_C_READ_ERR );
	    }
	}

	xmpl_file->xf_cur_scan = scan + 1;

	return( IMF_C_NORMAL );
    }

    ERR_printf( ERR__ERROR, "xmpl_scan_read, bad scan, %d\n", scan );
    return( IMF_C_BAD_SCAN );
}

/*
 *+***********************************************************************
 *
 *  Synopsis:
 *	static	int	xmpl_scan_write( data, scan, line_buff )
 *
 *  Purpose:
 *	Encode a scanline and write to an image file.
 *
 *  Parameters:
 *	POINTER	data		: (in)	Rle image data pointer
 *	int	scan		: (in)	Scan to be written
 *	POINTER	*line_buff	: (in)	Pointer to the scan data
 *
 *  Returns:
 *	int	IMF_C_BAD_SCAN	: Writing scanline out of order.
 *	int	IMF_C_NORMAL	: Scanline write was successful.
 *	int	IMF_C_WRITE_ERR	: Write error on file.
 *
 *  Notes:
 *      Scans must be written to the file sequentially.
 *
 *-***********************************************************************
 */


static	int	xmpl_scan_write(  POINTER data, int scan,
			POINTER *line_buff )
{
    int		ct;			/* Channel counter		*/
    int		scan_bytes;		/* Number of bytes per scanline	*/
    XMPL_FILE	*xmpl_file;		/* Our pertinent info on file	*/


    xmpl_file = (XMPL_FILE *) data;
    scan_bytes = xmpl_file->xf_im_win.right - xmpl_file->xf_im_win.left + 1;

    if ( scan == xmpl_file->xf_cur_scan
	    && scan <= xmpl_file->xf_im_win.top )
    {
	for ( ct = 0; ct < xmpl_file->xf_num_chans; ct++ )
	{
	    /*
	     *  Loop through the channels and write the data to the image
	     *  file.  The data for the channel is pointed to by
	     *  line_buff[ct].
	     */

	    if ( fwrite( (char *) line_buff[ct], scan_bytes, 1,
		    xmpl_file->xf_fp ) != 1 )
	    {
		ERR_printf( ERR__ERROR,
			"xmpl_scan_write, can't write. %s\n",
			strerror( errno ) );
		return( IMF_C_WRITE_ERR );
	    }
	}


	/*
	 *  Increment the current scan counter.
	 */

	xmpl_file->xf_cur_scan++;

	return( IMF_C_NORMAL );
    }

    ERR_printf( ERR__ERROR, "xmpl_scan_write, bad scan, %d\n", scan );
    return( IMF_C_BAD_SCAN );
}
