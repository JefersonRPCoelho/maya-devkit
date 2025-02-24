#ifndef __INCLUDE_FLIB_H__
#define __INCLUDE_FLIB_H__

/*!**************************************************************************
**
**  File library
**
**  Name    : flib.h		header file for use with flib.a
**  Author  : A.Hervas
**  Version : Beta 1.01, Wed May 18th 1994
**
****************************************************************************/

#include <stdint.h>

typedef uint32_t __uint32_t;
typedef int64_t __int64_t;
typedef uint64_t __uint64_t;
typedef unsigned int uint;

#if defined(_WIN32)
	#define PIPE_BUF     512  //this value taken from msdev/include/Limits.h  --Do not define _POSIX_
						 	  //cause more problems than it solves. 

	#if defined(NT_PLUGIN) || defined(NT_APP)
		#include <maya/MTypes.h>
	#else // NT_PLUGIN || NT_APP
		#ifndef FCHECK
			#include "HAWExport.h"
		#else
			#ifndef FND_EXPORT
				#define FND_EXPORT
			#endif
			#ifndef IMAGE_EXPORT
				#define IMAGE_EXPORT
			#endif
		#endif
		#include "no-windows.h"
	#endif // NT_PLUGIN || NT_APP

#else // _WIN32
	#ifndef FND_EXPORT
		#define FND_EXPORT __attribute__ ((visibility("default")))
	#endif
	#ifndef IMAGE_EXPORT
		#define IMAGE_EXPORT __attribute__ ((visibility("default")))
	#endif
    #include <unistd.h>
#endif // _WIN32

#include <sys/types.h>
#include <sys/stat.h>

#include <signal.h>
#include <limits.h>
#include <fcntl.h>

#include <string.h>

#include <stdio.h>

#include <stdlib.h>
#include <errno.h>

#include <ctype.h>
#include <assert.h>
#include <stdarg.h>

/*
	NAME
	    FLintro - Introduction to libfl.a (file IO section)

	DESCRIPTION
		The functions in this library could be divided in three categories:
		- Standard IO functions (open, read, write, etc...)
		- Structured IO functions
		- Toolbox functions


	    1. Standard IO

		FLIB's low-level IO functions are meant to abstract files such
		that an application doesn't need to worry about the underlying
		physical storage (disk file that might be memory-mapped,
		socket, piped stream, etc...).

		The filename passed to the FLopen function may refer to more than
		a simple disk file. For example, "mmap:file" or "pipe:cat file"
		are valid filename strings. See FLopen for more details.

		Once a file is opened, FLread and/or FLwrite can be used to access
		its content. These functions provide a simple, uniform way of accessing
		file content, but also remove some limitations due to the way the
		file was opened. For example, writing in a pipe that was opened for
		reading (by using "pipe:cat file") is possible.

		Additionally, it is possible to use these low-level IO functions
		to replace the stdio.h file functions: open, fopen, read, fread,
		write, fwrite, etc...

		Functions for managing path names are also provided.

	    2. Structured IO

		Structured files managed by libfl are organized in blocks defined
		by an id and a block size, both 4 bytes long. The structure
		is based on the original IFF (Interchange File Format) files that
		were used on PCs, Macs, SGIs, etc... plus some extensions.

		The IDs are divided in two types: those that define the structure
		of the file, and those that contain data. The first category contains
		4 identifiers:

		FORM <size> <type>: marks the start of a group of data blocks,
							similar to structures in the C language.

		CAT  <size> <type>: defines an unsorted group of FORM. For example,
							this is used to store images or sounds.

		LIST <size> <type>: defines a sorted group of FORM.
							(eg: a sequence of images)

		PROP <size> <type>: marks the start of a group of data blocks
							containing properties shared between FORMs
							that are part of a LIST. These properties
							can be locally redefined in a way analoguous
							to local variables in the C language.
							For example, a PROP could be used to define
							the dimensions and number of planes of
							sequence of image.

		Data blocks are defined by:

		<id> <size> [data]

		Example: a single image could have the following structure:

		FORM 12304 IMAG
		  IHDR 200  ... Header of the image, dimension, number of planes, etc...
		  LINE 800      ... data of scanline 1 ...
		  LINE 800      ... data of scanline 2 ...
		  ...

		For a library (bunch of images in no specific order):

		CAT 64200 IMAG
		  FORM 12304 IMAG
		    IHDR 200
			...
		  FORM 12304 IMAG
		    ...

		And for a sequence of images (movie):

	    LIST 64200 IMAG
	      PROP 208 IMAG
	        IHDR 200		... Common header ...
	      FORM 12394 IMAG
	        ...
	      FORM 12304 IMAG
	        IHDR 200		... Local re-definition ...
	        ...

		In the FLIB documentation, the blocks of structure are commonly
		called "groups", the blocks of data are named "chunks".

		I/O on chunks are done through the FLget/FLput functions and
		their variants. The opening/closing of groups is done through
		another set of functions: FLbgnrgroup, FLendrgroup, etc...

		Note on the structure:
			Everything is always written in Big-Endian format (where
		the most important byte is always written first). This is the
		native format of the SGI MIPS, Motorola and PowerPC/Mac
		architectures. In contrast, the Intel/AMD architecture is
		little-endian. This library always byte-swaps data to big-endian
		automatically, except for IDs, which are normally specified as
		a chain of 4 characters and therefore encoded in big-endian format
		at compile-time.

		A parser can take advantage of the size information that is included
		at the beginning of a block to skip the data content if it desires.

		Groups and data blocks are always 2-bytes aligned. If a data block
		doesn't end at a two-bytes alignment, a padding byte is added to
		the end but is not counted in the size of the block following the id.

		Additional IDs are defined for groups aligned to 4 or 8 bytes:
		FOR4, FOR8, CAT4, CAT8, LIS4, LIS8.

		Data blocks inherit the alignment of their parent group. Therefore
		creating a block aligned on 2 bytes inside another group aligned on
		4 bytes would be illegal. The inverse (a 4-bytes aligned block
		inside a 2-bytes aligned block) is perfectly valid.

  	    3. Toolbox

		FLIB also provides tools to manage linked lists (FLxxxnode and
		FLxxxlist), buffers (FLmalloc and others) and external filters
		(FLfilter and FLexec).


		Error Management

		The I/O functions attempt to prevent underflow/overflow and
		end-of-file errors. Still, it is highly recommended that the
		calling application do not attempt to continue reading/writing
		when an error occur.

		For more details on the functions contained inside libfl,
		refer to the man pages [claforte: don't know where they are...]
		and examples provided with the library.
*/

/*
**	Chunk descriptors (ids and sizes) are written in big endian format.
**	This section contains some macros for swapping words and half words
**	when needed depending on host byte sex.
**
****************************************************************************/



/*
**	macro for creating an id from four characters; any 7 bits character
**	is legal but it is STRONGLY RECOMENDED to use only printable chars.
*/

#ifdef __cplusplus
extern "C" {
#endif

#define	FLmakeid(c1,c2,c3,c4)	((unsigned int)(c4)				| \
				((unsigned int)(c3) << 8)			| \
				((unsigned int)(c2) << 16)			| \
				((unsigned int)(c1) << 24))



/*	pretty standard swapping macros					*/

#define	FLswapword(w)	((((unsigned int)(w) & 0x000000ff) << 24)	| \
                         ((((unsigned int)(w) & 0x0000ff00) << 8))	| \
                         ((((unsigned int)(w) & 0x00ff0000) >> 8))	| \
                         ((unsigned int)(w) >> 24))

#define FLswaphalf(h)		((((h) & 0xff) << 8) | ((unsigned short)(h) >> 8))

// PDB--The FLswapfloat macro doesn't work so I'm replacing it with a
// more efficient assembly language version on Windows NT (courtesy
// of Garr).  If we ever port to another little-endian host, we'll
// have to come up with an equivalent macro on that host.
#if defined(_WIN64)
__inline void FLswapfloat(float fSrc, float *pfDst)
{
	long result;
	result = ((((*(long*)&fSrc) & 0x000000ff) << 24) |
					  (((*(long*)&fSrc) & 0x0000ff00) << 8) |
					  (((*(long*)&fSrc) & 0x00ff0000) >> 8) |
                      (((*(long*)&fSrc) & 0xff000000) >> 24));
	*pfDst = *(float*)&result;
}

__inline void FLswapdouble(double fSrc, double *pfDst)
{
	__int64 result;
	__int64 tmp = *(__int64*) &fSrc;
	result = (        ((tmp & 0x00000000000000ff) << 56) |
					  ((tmp & 0x000000000000ff00) << 40) |
					  ((tmp & 0x0000000000ff0000) << 24) |
					  ((tmp & 0x00000000ff000000) << 8)  |
					  ((tmp & 0x000000ff00000000) >> 8)  |
					  ((tmp & 0x0000ff0000000000) >> 24) |
					  ((tmp & 0x00ff000000000000) >> 40) |
                      ((tmp & 0xff00000000000000) >> 56)
			 );
	*pfDst = *(double*)&result;
}
__inline void FLswapint64(__uint64_t fSrc, __uint64_t *pfDst)
{
	__int64 result;
	__int64 tmp = *(__int64*) &fSrc;
	result = (        ((tmp & 0x00000000000000ff) << 56) |
					  ((tmp & 0x000000000000ff00) << 40) |
					  ((tmp & 0x0000000000ff0000) << 24) |
					  ((tmp & 0x00000000ff000000) << 8)  |
					  ((tmp & 0x000000ff00000000) >> 8)  |
					  ((tmp & 0x0000ff0000000000) >> 24) |
					  ((tmp & 0x00ff000000000000) >> 40) |
                      ((tmp & 0xff00000000000000) >> 56)
			 );
	*pfDst = result;
}
#elif defined (_M_IX86)
__inline void FLswapfloat(float fSrc, float *pfDst)
{
	_asm	mov	eax, dword ptr [fSrc]
	_asm	mov edx, dword ptr [pfDst]
	_asm	ror ax, 8
	_asm	ror eax, 16
	_asm	ror ax, 8
	_asm	mov dword ptr [edx], eax
}

__inline void FLswapdouble(double dSrc, double *pdDst)
{
	_asm	mov eax, dword ptr [dSrc]
	_asm	ror ax, 8
	_asm	ror eax, 16
	_asm	ror ax, 8
	_asm	mov ebx, dword ptr [dSrc+4]
	_asm	ror bx, 8
	_asm	mov edx, dword ptr [pdDst]
	_asm	ror ebx, 16
	_asm	ror bx, 8
	_asm	mov dword ptr [edx], ebx
	_asm	mov dword ptr [edx+4], eax
}

__inline void FLswapint64(__uint64_t dSrc, __uint64_t *pdDst)
{
	_asm	mov eax, dword ptr [dSrc]
	_asm	ror ax, 8
	_asm	ror eax, 16
	_asm	ror ax, 8
	_asm	mov ebx, dword ptr [dSrc+4]
	_asm	ror bx, 8
	_asm	mov edx, dword ptr [pdDst]
	_asm	ror ebx, 16
	_asm	ror bx, 8
	_asm	mov dword ptr [edx], ebx
	_asm	mov dword ptr [edx+4], eax
}

#elif defined(__linux__)
#include <sys/types.h>

#define FLswapfloat(s,d)		FLswapfloatR(*((unsigned int *)&(s)),(d))
#define FLswapdouble(s,d)		FLswapdoubleR(*((__uint64_t*)&(s)),(d))

static inline void FLswapfloatR(unsigned int fSrc, float *pfDst)
{
    union {
        float f;
        __uint32_t i;
    } value;

    value.i = be32toh(fSrc);
    *pfDst = value.f;
}

static inline void FLswapdoubleR(__uint64_t dSrc, double *pdDst)
{
    union {
        double d;
        __uint64_t i;
    } value;

    value.i = be64toh(dSrc);
    *pdDst = value.d;
}

static inline void FLswapint64(__uint64_t iSrc, __uint64_t *piDst)
{
    *piDst = be64toh(iSrc);
}

#elif defined(__APPLE__)

#include <libkern/OSByteOrder.h>

// Just use the routines Apple supplies.  They are done in assembly anyway..
//
#define FLswapfloat(SRC,DEST) \
	*((uint32_t*)DEST) = OSSwapInt32( *( (uint32_t*) &SRC ) )

#define FLswapdouble(SRC,DEST) \
	*((uint64_t*)DEST) = OSSwapInt64( *( (uint64_t*) &SRC ) )

#define FLswapint64(SRC,DEST) \
	*DEST = OSSwapInt64( SRC )

#endif

#define	FLuswapword(wp)		((unsigned int)(((unsigned char *)wp)[0])		| \
				((unsigned int)(((unsigned char *)wp)[1]) << 8)	| \
				((unsigned int)(((unsigned char *)wp)[2]) << 16)	| \
				((unsigned int)(((unsigned char *)wp)[3]) << 24))

#define	FLuswaphalf(hp)		((unsigned short)(((unsigned char *)hp)[0])		| \
				((unsigned short)(((unsigned char *)hp)[1]) << 8))

/*	little endian data don't need any swapping			*/

#define FLswapleword(w)		(w)
#define FLswaplehalf(h)		(h)

#define	FLuswapleword(wp)	((unsigned int)(((unsigned char *)wp)[3])		| \
				((unsigned int)(((unsigned char *)wp)[2]) << 8)	| \
				((unsigned int)(((unsigned char *)wp)[1]) << 16)	| \
				((unsigned int)(((unsigned char *)wp)[0]) << 24))

#define	FLuswaplehalf(hp)	((unsigned short)(((unsigned char *)hp)[1])		| \
				((unsigned short)(((unsigned char *)hp)[0]) << 8))

/*
**	This section defines the standard IFF ids along with some extensions.
**
****************************************************************************/

#define	FL_FORM		FLmakeid( 'F','O','R','M' )
#define	FL_CAT		FLmakeid( 'C','A','T',' ' )
#define	FL_LIST		FLmakeid( 'L','I','S','T' )
#define	FL_PROP		FLmakeid( 'P','R','O','P' )
#define	FL_NULL		0L

/*
**	we need a word-aligned version of the standard blocks;
**	by the way, ids for 64 bits alignment are also defined.
*/

#define	FL_FOR4		FLmakeid( 'F','O','R','4' )
#define	FL_FOR8		FLmakeid( 'F','O','R','8' )
#define	FL_CAT4		FLmakeid( 'C','A','T','4' )
#define	FL_CAT8		FLmakeid( 'C','A','T','8' )
#define	FL_LIS4		FLmakeid( 'L','I','S','4' )
#define	FL_LIS8		FLmakeid( 'L','I','S','8' )
#define	FL_PRO4		FLmakeid( 'P','R','O','4' )
#define	FL_PRO8		FLmakeid( 'P','R','O','8' )

/*
**	some defines to use when writing chunks of unknown size.
**	For a normal (seekable) object, the writer random access the
**	chunk's header to update the size.
*/

#define	FL_szUnknown	0x80000000
#define	FL_szFile	0x80000001
#define	FL_szFifo	0x80000002
#define	FL_szMask	0x7ffffffc

#define	FL_szInf	0xfffffff0
#define	FL_szSInf	0x7ffffff0

/*
**	some extensions to the IFF specs
**
**	PATH defines the search path for includes.
**	INCL is an include block.
**	EOVC is the end-of-variable-length-chunk marker.
**	GEND is the end-of-group marker.
**
**	Syntax :	PATH	#	<directory names>
**			INCL	#	<file names>
**			EOVC	szUnknown
**			GEND	0
**
**	PATH and INCL are data chuncks and thus inherite alignment.
**
**	EOVC and GEND are use in fifo files since the chunk and group sizes
**	can not be random accessed. Creating groups or chunks with
**	unspecified size on a fifo will give something like :
**
**	FORM	sz_Fifo		TYPE		; start of form
**		BLCK	sz_Fifo		data	; block 1
**		EOVC	sz_Unknown		; end of block 1
**		....				;
**		BLCK	sz_Fifo		data	; block 2
**		EOVC	sz_Unknown		; end of block 2
**		GEND	0			; no more block in this FORM
**
**	EOVC is a block/group end marker, while GEND is understood as a
**	request to close the current group, going up one level.
**
**	Using unknown sizes when writing a seekable file will produce a very
**	similar structure exept that no EOVC is written. This is very useful
**	when parsing a file under construction since the end of group can be
**	detected w/o any random access to the group header. The size field of
**	GEND is set to zero to allow other standard IFF parsers to skip it
**	silently. The EOVC's size field value will produce an error if read
**	by a standard parser (w/o fifo extensions).
**
**	Warning: for compatibility with the previous version of the IFF parser
**		 GEND is still followed by sz_Unknown (and EOVC sz_Unknown
**		 if the file is a fifo).
**
**	since there is no reliable way of skipping a block of unknown length
**	it is STRONGLY RECOMMENDED that writer and reader of a fifo agree on
**	the file's content.
**
**	note that the implemented parser is smart enough to locate EOVCs
**	in a file, but the skipping process is slow due to intensive read
**	and compares. and again, this can NOT be considered 100% reliable.
*/

#define	FL_PATH		FLmakeid( 'P','A','T','H' )
#define	FL_INCL		FLmakeid( 'I','N','C','L' )
#define	FL_GEND		FLmakeid( 'G','E','N','D' )
#define	FL_EOVC		FLmakeid( 'E','O','V','C' )

/*
**	more or less standard (registered) IFF ids
*/

#define	FL_AIFF		FLmakeid( 'A','I','F','F' )
#define	FL_AIFC		FLmakeid( 'A','I','F','C' )
#define	FL_AUTH		FLmakeid( 'A','U','T','H' )
#define	FL_NAME		FLmakeid( 'N','A','M','E' )
#define	FL_CPRT		FLmakeid( '(','c',')',' ' )
#define	FL_ANNO		FLmakeid( 'A','N','N','O' )
#define	FL_COMM		FLmakeid( 'C','O','M','M' )
#define	FL_SSND		FLmakeid( 'S','S','N','D' )
#define	FL_COMT		FLmakeid( 'C','O','M','T' )
#define	FL_DATE		FLmakeid( 'D','A','T','E' )
#define FL_DPI      FLmakeid( 'D','P','I',' ' )
#define	FL_BODY		FLmakeid( 'B','O','D','Y' )
#define FL_FVER		FLmakeid( 'F','V','E','R' )

/*
**	non standard
*/

#define	FL_FILE		FLmakeid( 'F','I','L','E' )
#define	FL_DATA		FLmakeid( 'D','A','T','A' )
#define	FL_MANY		FLmakeid( 'M','A','N','Y' )
#define	FL_USER		FLmakeid( 'U','S','E','R' )
#define	FL_WDGT		FLmakeid( 'W','D','G','T' )

/*
**	for the image format
*/

#define	FL_CIMG		FLmakeid( 'C','I','M','G' )
#define	FL_TBHD		FLmakeid( 'T','B','H','D' )
#define	FL_FLDS		FLmakeid( 'F','L','D','S' )
#define	FL_CMAP		FLmakeid( 'C','M','A','P' )
#define	FL_RAYT		FLmakeid( 'R','A','Y','T' )
#define	FL_TBMP		FLmakeid( 'T','B','M','P' )
#define	FL_RGBA		FLmakeid( 'R','G','B','A' )
#define	FL_ZBUF		FLmakeid( 'Z','B','U','F' )
#define	FL_ABUF		FLmakeid( 'A','B','U','F' )
#define	FL_ICON		FLmakeid( 'I','C','O','N' )

/*
**  obsolete
*/

#define	FL_RGB2		FLmakeid( 'R','G','B','2' )

/*
**	for font files
*/

#define	FL_FONT		FLmakeid( 'F','O','N','T' )
#define	FL_FHDR		FLmakeid( 'F','H','D','R' )
#define	FL_FCHR		FLmakeid( 'F','C','H','R' )

/*
**	for model files
*/

#define FL_MDLF		FLmakeid( 'M','D','L','F' )
#define FL_MHDR		FLmakeid( 'M','H','D','R' )
#define FL_SGRP		FLmakeid( 'S','G','R','P' )
#define FL_PGSH		FLmakeid( 'P','G','S','H' )
#define FL_SHEL		FLmakeid( 'S','H','E','L' )
#define FL_SHDR		FLmakeid( 'S','H','D','R' )
#define FL_VRTX		FLmakeid( 'V','R','T','X' )
#define FL_EDGE		FLmakeid( 'E','D','G','E' )
#define	FL_POLY		FLmakeid( 'P','O','L','Y' )
#define	FL_CURV		FLmakeid( 'C','U','R','V' )
#define	FL_SURF		FLmakeid( 'S','U','R','F' )
#define	FL_USER		FLmakeid( 'U','S','E','R' )


/*
**	map preprocessing blocks
*/

#define	FL_TBUF		FLmakeid( 'T','B','U','F' )
#define	FL_RPBM		FLmakeid( 'R','P','B','M' )
#define	FL_GPBM		FLmakeid( 'G','P','B','M' )
#define	FL_BPBM		FLmakeid( 'B','P','B','M' )
#define	FL_APBM		FLmakeid( 'A','P','B','M' )

/*
**	hypertext files
*/

#define	FL_HTXT		FLmakeid( 'H','T','X','T' )
#define	FL_PATN		FLmakeid( 'P','A','T','N' )
#define	FL_PBGD		FLmakeid( 'P','B','G','D' )
#define	FL_PAGE		FLmakeid( 'P','A','G','E' )
#define	FL_PDEF		FLmakeid( 'P','D','E','F' )
#define	FL_SHOW		FLmakeid( 'S','H','O','W' )
#define	FL_STYL		FLmakeid( 'S','T','Y','L' )
#define	FL_FTXT		FLmakeid( 'F','T','X','T' )
#define	FL_RECT		FLmakeid( 'R','E','C','T' )
#define	FL_CIRC		FLmakeid( 'C','I','R','C' )
#define	FL_CARC		FLmakeid( 'C','A','R','C' )
#define	FL_RCRV		FLmakeid( 'R','C','R','V' )
#define	FL_SIMG		FLmakeid( 'S','I','M','G' )
#define	FL_LINK		FLmakeid( 'L','I','N','K' )

/*
**	ASDG - Elastic Reality
*/

#define	FL_ASDG		FLmakeid( 'A','S','D','G' )
#define	FL_PROJ		FLmakeid( 'P','R','O','J' )
#define	FL_GUIO		FLmakeid( 'G','U','I','O' )
#define	FL_MRPH		FLmakeid( 'M','R','P','H' )
#define	FL_PREV		FLmakeid( 'P','R','E','V' )
#define	FL_OUTP		FLmakeid( 'O','U','T','P' )
#define	FL_COLR		FLmakeid( 'C','O','L','R' )
#define	FL_GRUP		FLmakeid( 'G','R','U','P' )
#define	FL_FRAM		FLmakeid( 'F','R','A','M' )
#define	FL_SHAP		FLmakeid( 'S','H','A','P' )

/*
**	Mask for group Ids
*/

#define	FL_GMSK		FLmakeid(0xff, 0xff, 0xff, 0)
#define	FL_FORx		FLmakeid('F','O','R',0)
#define	FL_CATx		FLmakeid('C','A','T',0)
#define	FL_LISx		FLmakeid('L','I','S',0)
#define	FL_PROx		FLmakeid('P','R','O',0)

/*
**	this section defines the status codes set and/or returned by the
**	functions of this library.
**
****************************************************************************/

#define	FL_OK		0L
#define	FL_End		1L
#define	FL_Done		2L
#define	FL_BadId	3L
#define	FL_Abort	4L
#define	FL_BadForm	5L
#define	FL_BadFile	6L
#define	FL_Broken	7L
#define	FL_Retry	8L
#define	FL_Partial	9L
#define	FL_NotYet	10L
#define	FL_NotFile	11L
#define	FL_BadRoot	12L
#define	FL_Long		13L
#define	FL_BadProp	14L
#define	FL_NoEovc	15L
#define	FL_BadSize	16L
#define	FL_BadPipe	17L
#define	FL_BadEovc	18L
#define	FL_BadEnd	19L
#define	FL_FileOnly	20L
#define	FL_BadName	21L
#define	FL_BadAlign	22L
#define	FL_BadCpr	23L
#define	FL_BadMode	24L
#define	FL_NoGroup	25L
#define	FL_NoChunk	26L
#define	FL_Writing	27L
#define	FL_UnSize	28L
#define	FL_NestWrite	29L
#define	FL_NestRead	30L
#define	FL_Group	31L
#define	FL_Chunk	32L

#define	FL_NoMem	33L
#define	FL_ReservedId	34L
#define	FL_NoMark	35L

#define	FL_OSError	40L
#define	FL_HError	41L
#define	FL_BadCall	42L
#define	FL_BadFilter	43L
#define	FL_Unknown	44L
#define	FL_BadType	45L

#define	FL_NoHost	46L

#define	FL_Break	47L
#define	FL_Internal	48L
#define	FL_Debug	49L

/*	error codes from extensions start here		*/

#define	FL_Extensions	99L

#define	FL_ImgBase	FL_Extensions

#define	IL_NoImage	(FL_ImgBase+1L)
#define	IL_NotImage	(FL_ImgBase+2L)
#define	IL_Unknown	(FL_ImgBase+3L)
#define	IL_HdrDone	(FL_ImgBase+4L)
#define	IL_NoHdr	(FL_ImgBase+5L)
#define	IL_BadHdr	(FL_ImgBase+6L)
#define	IL_BadSize	(FL_ImgBase+7L)
#define	IL_End		(FL_ImgBase+8L)
#define	IL_BadCprs	(FL_ImgBase+9L)
#define	IL_BadImg	(FL_ImgBase+10L)
#define	IL_Znotbound	(FL_ImgBase+11L)
#define	IL_NoCvt	(FL_ImgBase+12L)
#define	IL_NoZoom	(FL_ImgBase+13L)

#define	FL_ItBase	(IL_NoZoom)

#define	IT_NotMemory	(FL_ItBase+1L)
#define	IT_MixedDepth	(FL_ItBase+2L)

#define	FL_TxtBase	(FL_ItBase+20L)

#define	HL_NoText	(FL_TxtBase+1L)
#define	HL_NotText	(FL_TxtBase+2L)
#define	HL_NoPage	(FL_TxtBase+3L)
#define	HL_OutPage	(FL_TxtBase+4L)
#define	HL_NoItemLink	(FL_TxtBase+5L)
#define	HL_NoCvt	(FL_TxtBase+6L)

#define	FL_ModelBase	(HL_NoCvt)

#define	ML_NoModel	(FL_ModelBase+1L)
#define	ML_NotModel	(FL_ModelBase+2L)
#define	ML_Unknown	(FL_ModelBase+3L)
#define	ML_HdrDone	(FL_ModelBase+4L)
#define	ML_NoHdr	(FL_ModelBase+5L)
#define	ML_BadHdr	(FL_ModelBase+6L)
#define	ML_End		(FL_ModelBase+7L)
#define	ML_BadModel	(FL_ModelBase+8L)
#define	ML_NoCvt	(FL_ModelBase+9L)
#define	ML_IllCvt	(FL_ModelBase+10L)
#define	ML_NotYet	(FL_ModelBase+11L)
#define	ML_NoVLink	(FL_ModelBase+12L)
#define	ML_BadLoop	(FL_ModelBase+13L)
#define	ML_BadOrient	(FL_ModelBase+14L)
#define	ML_DblVLink 	(FL_ModelBase+15L)
#define	ML_BadVSubd	(FL_ModelBase+16L)
#define	ML_NoPNormal	(FL_ModelBase+17L)
#define	ML_IncTopo	(FL_ModelBase+18L)
#define	ML_IncTree	(FL_ModelBase+19L)
#define	ML_TrgError	(FL_ModelBase+20L)
#define	ML_BadSharp	(FL_ModelBase+21L)

#define	FL_UIBase	(FL_ModelBase+64L)

#define	UI_NotUIFile	(FL_UIBase+1L)
#define	UI_NoUIDef	(FL_UIBase+2L)
#define	UI_NoCvt	(FL_UIBase+3L)
#define	UI_NoParent	(FL_UIBase+4L)
#define	UI_NoFontCvt	(FL_UIBase+5L)
#define	UI_NotFont	(FL_UIBase+6L)
#define	UI_InvDevice	(FL_UIBase+7L)
#define	UI_InvPopDev	(FL_UIBase+8L)
#define	UI_DevNotQ	(FL_UIBase+9L)

#define	FL_EEBase	255

#define FL_NoSymbol	(FL_EEBase+1L)
#define	FL_Zerodivide	(FL_EEBase+2L)
#define	FL_Overflow	(FL_EEBase+3L)
#define	FL_Underflow	(FL_EEBase+4L)
#define	FL_Unbalanced	(FL_EEBase+5L)
#define	FL_Syntax	(FL_EEBase+6L)
#define	FL_BadAssign	(FL_EEBase+7L)
#define	FL_ConstAssign	(FL_EEBase+8L)
#define	FL_ConstExpect	(FL_EEBase+9L)
#define	FL_NoColon	(FL_EEBase+10L)
#define	FL_NoIf		(FL_EEBase+11L)
#define	FL_NoWhile	(FL_EEBase+12L)
#define	FL_PMissing	(FL_EEBase+13L)
#define	FL_CMissing	(FL_EEBase+14L)
#define	FL_BadNbArg	(FL_EEBase+15L)
#define	FL_CInvalid	(FL_EEBase+16L)
#define	FL_NCusedinC	(FL_EEBase+17L)
#define	FL_Baducode	(FL_EEBase+18L)
#define	FL_QMissing	(FL_EEBase+19L)
#define	FL_KMissing	(FL_EEBase+20L)
#define	FL_StrTooLong	(FL_EEBase+21L)
#define	FL_NoStrEnd	(FL_EEBase+22L)

#define FL_ResourceBase	FL_NoStrEnd

#define RL_NoResource   (FL_ResourceBase+1L)
#define RL_NotResource  (FL_ResourceBase+2L)
#define RL_Unknown      (FL_ResourceBase+3L)
#define RL_HdrDone      (FL_ResourceBase+4L)
#define RL_NoHdr        (FL_ResourceBase+5L)
#define RL_BadHdr       (FL_ResourceBase+6L)
#define RL_End          (FL_ResourceBase+7L)
#define RL_BadResource  (FL_ResourceBase+8L)
#define RL_NoCvt        (FL_ResourceBase+9L)

#define	FL_LastError	(RL_NoCvt+1L)

/*	file main opening mode		*/

#define	FL_Read		0x00000001
#define	FL_Write	0x00000002
#define	FL_Edit		0x00000004
#define	FL_Mode		0x0000000f

/*	type of file			*/

#define	FL_Fifo		0x00000010
#define	FL_Sock		0x00000020	/* not yet implemented	*/
#if 0
#define	FL_Pipe		0x00000040	/* obsolete		*/
#endif
#define	FL_Mem		0x00000080	/* file is a pseudo-file in memory ('mem:')	*/
#define	FL_Mapped	0x00000100	/* file is memory mapped ('mmap:')			*/
#define	FL_Local	0x00000200
#define	FL_Tty		0x00000400
#define	FL_Std		0x00000800
#define	FL_Tmp		0x00001000
#define	FL_Remote	0x00002000
#define	FL_Type		0x00003ff0

#define	FL_FtMemory	(FL_Mem | FL_Mapped)
#define	FL_FtNoseek	(FL_Fifo | FL_Sock | FL_Tty)

/*	type of system blocks. See flsetid.c for descriptions.		*/

#define	FL_ICuser	0x00010000
#define	FL_ICsys	0x00020000
#define	FL_ICroot	0x00040000
#define	FL_ICpropOk	0x00080000
#define	FL_ICform	0x00100000
#define	FL_ICcat	0x00200000
#define	FL_IClist	0x00400000
#define	FL_ICprop	0x00800000
#define	FL_ICgroup	0x01000000
#define	FL_ICmask	0x01ff0000

/*	internal control flags		*/

#define	FL_Sys		0x01ffffff
#define	FL_Sync		0x10000000
#define	FL_SysWrite	0x20000000
#define	FL_GHread	0x40000000

// When set, a seek to the current location is needed before reading. Set
// after a write.
#define	FL_SeekRead	0x04000000
// When set, a seek to the current location is needed before writing. Set
// after a read.
#define	FL_SeekWrite	0x08000000
#define	FL_ToSeek	(FL_SeekRead | FL_SeekWrite)

/*	maximum file name length (including path) and null	*/
//	#define	FL_MaxName	(PATH_MAX + NAME_MAX + 2)
#if defined (__linux__) || defined (__APPLE__)
#define	FL_MaxName	(PATH_MAX + NAME_MAX + 2)
#elif defined(__APPLE__)
#define	FL_MaxName	(_MAX_PATH + FILENAME_MAX + 2)
#elif defined(_WIN32)
#define	FL_MaxName	(_MAX_PATH + _MAX_FNAME + 2)
#else
#	error "__FILE__:__LINE__ Unknown O/S"
#endif

/*	maximum size of a mapped object in write mode: 128Mb	*/

#define	FL_MaxMSize	134217728

#define FLmakeversion(v,r)  (((v) << 16) | (r))

/*
**	type definitions
**
****************************************************************************/

typedef	int		FLid;
typedef	int		(*FLfunc)();

typedef	struct _FLnode
{
    struct _FLnode *	next;
    struct _FLnode *	prev;
    unsigned int		type;
    char *		name;

} FLnode;

typedef struct _FLlist
{
    struct _FLnode *	head;
    struct _FLnode *	dummy;
    struct _FLnode *	tail;

} FLlist;

typedef FLlist	    FLmkey;

typedef	struct _FLchunk
{
    FLid		id;
    unsigned		size;

} FLchunk;

typedef	struct _FLgroup
{
    FLchunk		chunk;
    FLid		type;

} FLgroup;

typedef	struct _FLparser
{
    FLfunc		f_Form;
    FLfunc		f_List;
    FLfunc		f_Leaf;

} FLparser;

typedef	struct _FLcontext
{
    FLnode		node;

    FLgroup		group;
    unsigned int		sofar;
    unsigned int		loc;
    unsigned int		align;
    unsigned int		level;
    unsigned int		bound;
    char *		ipath;

} FLcontext;

#if defined(_WIN32)
	typedef HANDLE process_t;
	typedef int    pid_t;
#else
    typedef pid_t process_t;
#endif

#define kMaxSizeEncoding 103

#define FLFILE_MAX_BUFFER_SIZE (5*1024*1024)
typedef	struct _FLfile
{
    FLnode		node;

    FILE *		fp;
    int			size;	/* file size								*/
    int			rwsize;	/* furthest location read or written (?)	*/

    FLcontext *		context;
    FLcontext		root;
    FLparser		parser;

    char *		path;
    char *		bname;

    void *		shared;
    unsigned int		shrwsize;

    FLmkey		memory;
    FLlist		marks;
    FILE *		wdelay;
	process_t	pid;		/* pid of filter's feeder */
    void *		includes;
    FLid		userdata;
    char *		unrb;
    int			unrs;
    int			extend;

    // The following members are used to buffer data prior to writing it
    // to disk. This is an optimization added to reduce the number of disk
    // writes. See bug 195602.
    //
    char *		buffer;		/* the buffered data */
    int			bufsize;	/* the current amount of buffered data */
    int			bufloc;		/* the current location within the buffer */
    int			bufmaxsize;	/* the allocated size of the buffer */

} FLfile;

/* private flags */

#define	FL_Ctrl_Reorder		0x00000001
#define	FL_Ctrl_LocalMask	0x000000f0
#define	FL_Ctrl_LocalMmap	0x00000000
#define	FL_Ctrl_LocalFile	0x00000010
#define	FL_Ctrl_LocalNone	0x000000f0
#define	FL_Ctrl_ForceMap	0x00000100
#define	FL_Ctrl_NoPack		0x00000200
#define	FL_Ctrl_NoUnpack	0x00000400

/* flags for FLconfig */

#define FLC_Local	1
#define	FLC_AutoMap	2
#define	FLC_Pack	3
#define	FLC_Unpack	4

#define	FL_LOCALNONE	0
#define	FL_LOCALMMAP	1
#define	FL_LOCALFILE	2

#ifdef FL_SPROC

typedef struct _FLprda
{
    int	    flerrno;
    int	    fldelay;
    int	    flflags;
    int	    floserror;
    void *  flpath;
    void *  fleedict;
    void *  fleecexp;

    /*	extension stuff */

    int	    ilconfig;
    void *  ilzdef;

} FLprda;

#define	flprda		((FLprda *)&(PRDA->usr_prda))
#define	flerrno		(flprda->flerrno)
#define	fldelay		(flprda->fldelay)
#define	flretry		(flprda->flretry)
#define	flflags		(flprda->flflags)
#define	floserror	(flprda->oserror)
#define	flpath		(flprda->flpath)
#define	ilconfig	(flprda->ilconfig)

#else

extern	FND_EXPORT int	flerrno;
extern	FND_EXPORT int	fldelay;
extern	FND_EXPORT int	flflags;
extern	IMAGE_EXPORT int	ilconfig;

#endif

extern	FND_EXPORT	const unsigned char	flbitset[256];	    /* # of bits set in a byte	    */
extern	FND_EXPORT	const unsigned char	flrevbit[256];	    /* reversed bytes		    */
extern	FND_EXPORT	const unsigned char	flhbitset[256];	    /* highest bit set in a byte    */
extern	FND_EXPORT	FLlist		flfilelist;

/*
**	function prototypes
**
****************************************************************************/

/*	Lists and nodes management functions	*/

extern	FND_EXPORT	FLlist *	FLnewlist(void);
extern	FND_EXPORT	void		FLinitlist(FLlist *);
extern	FND_EXPORT	void		FLclearlist(FLlist *);
extern	FND_EXPORT	void		FLfreelist(FLlist *);
extern	FND_EXPORT	void		FLmergelist(FLlist *, FLlist *);

extern	FND_EXPORT	FLnode *	FLnewnode(const char *, unsigned, unsigned);
extern	FND_EXPORT	FLnode *	FLnewcnode(const char *, unsigned, unsigned);
extern	FND_EXPORT	void 		FLinitnode(FLnode *, const char *, unsigned);
extern	FND_EXPORT	int		FLclearnode(FLnode *);
extern	FND_EXPORT	int		FLfreenode(FLnode *);
extern	FND_EXPORT	void		FLheadnode(FLlist *, FLnode *);
extern	FND_EXPORT	void		FLtailnode(FLlist *, FLnode *);
extern	FND_EXPORT	void		FLinsnode(FLnode *, FLnode *);
extern	FND_EXPORT	int		FLdelnode(FLnode *);
extern	FND_EXPORT	void		FLrenamenode(FLnode *, const char *);

extern	FND_EXPORT	FLnode *	FLscanlist(FLlist *, int (*)(), void *);
extern	FND_EXPORT	FLnode *	FLrscanlist(FLlist *, int (*)(), void *);
extern	FND_EXPORT	FLnode *	FLscannode(FLnode *, int (*)(), void *);
extern	FND_EXPORT	FLnode *	FLrscannode(FLnode *, int (*)(), void *);

extern	FND_EXPORT	FLnode *	FLfscanlist(FLlist *, int (*)(), void *);
extern	FND_EXPORT	FLnode *	FLfrscanlist(FLlist *, int (*)(), void *);
extern	FND_EXPORT	FLnode *	FLfscannode(FLnode *, int (*)(), void *);
extern	FND_EXPORT	FLnode *	FLfrscannode(FLnode *, int (*)(), void *);

extern	FND_EXPORT	FLnode *	FLgetnodebyname(const FLlist *, const char *);
extern	FND_EXPORT	FLnode *	FLrgetnodebyname(const FLlist *, const char *);

extern	FND_EXPORT	FLnode *	FLgetnodebytype(const FLlist *, unsigned);
extern	FND_EXPORT	FLnode *	FLrgetnodebytype(const FLlist *, unsigned);

/*	"Remembers" support			*/

extern	FND_EXPORT	FLmkey *	FLnewmkey(void);
extern	FND_EXPORT	void		FLfreemkey(FLmkey *);
extern	FND_EXPORT	void		FLinitmkey(FLmkey *);
extern	FND_EXPORT	void		FLmergemkey(FLmkey *, FLmkey *);
extern	FND_EXPORT void *		FLmalloc(unsigned, FLmkey *);
extern	FND_EXPORT void *		FLcalloc(unsigned, FLmkey *);
extern	FND_EXPORT	void *		FLrealloc(void *, unsigned, FLmkey *);
extern	FND_EXPORT	void *		FLgrow(void *, unsigned, FLmkey *);
extern	FND_EXPORT	void *		FLrgrow(void *, unsigned, FLmkey *);
extern	FND_EXPORT	int		FLfree(void *);
extern	FND_EXPORT	void		FLclean(FLmkey *);

extern	FND_EXPORT	FLnode *	FLnodealloc(const char *, uint, uint, FLmkey *);
extern	FND_EXPORT	int		FLminfo(FLmkey *, int *);
extern	FND_EXPORT	int		FLfminfo(FLfile *, int *);

/*	Error related functions			*/

extern	FND_EXPORT	int		FLoserror();
extern	FND_EXPORT	int		FLsetoserror(int);

extern	FND_EXPORT int		FLerror();
extern	FND_EXPORT	int		FLseterror(int);

#ifdef NDEBUG
#define	FLerrset(i)	(flerrno = (int)(i))
#else
#define	FLerrset(i)	FLseterror(i)
#endif

extern	FND_EXPORT	void		FLcatchint(const char *);
extern	FND_EXPORT	void		FLcatchcore(const char *);
extern	FND_EXPORT void		FLcatchall(const char *);
extern	FND_EXPORT	void		FLcatchfunc(void (*)(int), int);
extern	FND_EXPORT int		FLsilentcatch(int);

extern	FND_EXPORT void		FLperror(const char *);
extern	FND_EXPORT void		FLprgerror(const char *);
extern	FND_EXPORT void		FLprgname(const char *);
extern	FND_EXPORT const char *	FLstrerror(int);
extern	FND_EXPORT	const char *	FLstrsignal(int);

/*	Basic file functions			*/

extern	FND_EXPORT FLfile *	FLopen(const char *, const char *);
extern	FND_EXPORT FLfile *	FLopenCreatorType(const char *, const char *, long, long);
extern	FND_EXPORT	FLfile *	FLreopen(const char *, const char *, FLfile *);
extern	FND_EXPORT	void *		FLsopen(const char *, const char *, uint);
extern	FND_EXPORT int		FLclose(FLfile *);
extern	FND_EXPORT	int		FLqclose(FLfile *);
extern	FND_EXPORT int		FLflush(FLfile *);
extern	FND_EXPORT 	void	FLflushBuffer(FLfile*);
extern	FND_EXPORT	void		FLflushall();
extern	FND_EXPORT int		FLseek(FLfile *, int, int);
extern	FND_EXPORT int		FLtell(const FLfile *);
extern	FND_EXPORT	void		FLsetdelay(int);
extern	FND_EXPORT	void		FLconfig(int, int);
extern	FND_EXPORT void		FLsettmp(FLfile *, int);
extern	FND_EXPORT int		FListty(const char *);
extern	FND_EXPORT	int		FListtyfile(const FLfile *);
extern	FND_EXPORT void		FLinitializeBuffer(FLfile *, int);

extern	FND_EXPORT	int		FLmakename(char *, const char *, float);
extern	FND_EXPORT	int		FLframetostr(char *, float);
extern	FND_EXPORT	int		FLinsframe(char *, size_t, const char *, float);
extern	FND_EXPORT int		FLreplaceframe(char *, size_t, const char *, float);

extern	FND_EXPORT void		FLgetext(const char *, char *, char *, char *);
extern	FND_EXPORT int		FLcheckext(const char *, const char *);


extern	FND_EXPORT	FLfile *	FLfilter(const char *, const char *, FLfile *);
extern	FND_EXPORT	FLfile *	FLpopen(const char *, const char *);
#if (defined(__linux__) || defined(__APPLE__))
extern	FND_EXPORT	int		FLexec(const char *);
extern	FND_EXPORT	int		FLspawn(const char *, int);
#elif defined(_WIN32)
extern	FND_EXPORT	HANDLE		FLexec(const char *, HANDLE, HANDLE);
extern	FND_EXPORT	HANDLE		FLspawn(const char *, int, HANDLE, HANDLE);
#else
#	error "__FILE__:__LINE__ Unknown O/S"
#endif
extern	FND_EXPORT	int		FLsystem(const char *);
extern	FND_EXPORT  void		FLexit(int);
extern	FND_EXPORT	void		FLabort();
extern	process_t	FLfork();

extern	FND_EXPORT	void		FLtmpname(char *, const char *);

/*	raw IO functions			*/

extern	FND_EXPORT int		FLread(FLfile *, void *, unsigned);
extern	FND_EXPORT int		FLunread(FLfile *, const void *, unsigned);
extern	FND_EXPORT const void *	FLbgnread(FLfile *, unsigned);

extern	FND_EXPORT int		FLwrite(FLfile *, const void *, unsigned);
extern	FND_EXPORT void *	FLbgnwrite(FLfile *, unsigned);
extern	FND_EXPORT int		FLendwrite(FLfile *, unsigned);

extern	FND_EXPORT	int		FLfdwrite(int, const void *, unsigned);
extern	FND_EXPORT	int		FLfdread(int, void *, unsigned);

/*	simple edition				*/

extern	FND_EXPORT	void *		FLinsbytes(FLfile *, int);
extern	int		FLgetaux(const char *, FLid, void **, int);
extern	int		FLputaux(const char *, FLid, int, const void *, int);

/*	Structured IO				*/

extern	FND_EXPORT int		FLbgnget(FLfile *, FLid *, unsigned *);
extern	FND_EXPORT int		FLget(FLfile *, void *, unsigned);
extern	FND_EXPORT int		FLunget(FLfile *, const void *, unsigned);
extern	FND_EXPORT int		FLendget(FLfile *);
extern	FND_EXPORT const void *	FLsget(FLfile *, unsigned);

extern	FND_EXPORT int		FLbgnput(FLfile *, FLid, unsigned);
extern	FND_EXPORT int		FLput(FLfile *, const void *, unsigned);
extern	FND_EXPORT int		FLendput(FLfile *);

//
// These methods are for encrypted file formats, to allow us to
// encode the data sizes of the iff chunks to make them harder to
// parse using a standard iff file reader.
//
#define FLsetSizeEncoding FLsetPathAdjust
#define FLencodeSize      FLrgwbgroup

extern  FND_EXPORT void		FLsetSizeEncoding(unsigned); /* used for encoding/decoding of size data */
extern  unsigned	FLencodeSize(unsigned, FLid);    /* used for decoding of size data */

extern	FND_EXPORT void *		FLreadchunk(FLfile *, FLid *, unsigned *);
extern	FND_EXPORT const void *	FLgetchunk(FLfile *, FLid *, unsigned *);

extern	FND_EXPORT int		FLputchunk(FLfile *, FLid, unsigned, const void *);
extern	FND_EXPORT int		FLputchunkTyped(FLfile *, FLid, uint, const void *, __uint32_t);
extern	FND_EXPORT void *	FLbgnwbchunk(FLfile *, FLid, unsigned);
extern	FND_EXPORT int		FLendwbchunk(FLfile *, unsigned);

extern	FND_EXPORT int		FLbgnrgroup(FLfile *, FLid *, FLid *);
extern	FND_EXPORT int		FLendrgroup(FLfile *);

extern	FND_EXPORT int		FLbgnwgroup(FLfile *, FLid, FLid);
extern	FND_EXPORT int		FLendwgroup(FLfile *);

extern	FND_EXPORT int		FLskipgroup(FLfile *);

/*  >>>>>	    End of temporary name defs		<<<<<<  */

/*	formatted IO				*/

extern	FND_EXPORT char *		FLgets(FLfile *, char *, int);
extern	FND_EXPORT int			FLputs(FLfile *, const char *);
extern	FND_EXPORT int			FLprintf(FLfile *, const char *, ...);
extern	FND_EXPORT int			FLscanf(FLfile *, const char *, ...);

/*	File finding and path control		*/

extern	FND_EXPORT void			FLsetwork(const char *, const char *);
extern	FND_EXPORT const char *	FLgetwork(const char **);

extern	FND_EXPORT const char *	FLfindfile(const char *, const char *);
extern	FND_EXPORT const char *	FLfindcmd(const char *);
extern	FND_EXPORT const char *	FLfinddriver(const char *);

extern	FND_EXPORT void 		FLmakepath(char *, const char *);
extern	FND_EXPORT void 		FLsetpath(const char *);
extern	FND_EXPORT void			FLaddpath(const char *);
extern	FND_EXPORT void *		FLbuildpath(const char *);
extern	FND_EXPORT void			FLfreepath(void *);
extern	FND_EXPORT void 		FLswitchpath(void *);
extern	FND_EXPORT void *		FLswappath(void *);
extern	FND_EXPORT void			FLsetreorder(int);
extern	FND_EXPORT const char *	FLfilename(const FLfile *, char *, char *);

/*	File parsing routines			*/

extern	FND_EXPORT int		FLparse(FLfile *);
extern	FND_EXPORT void		FLgetparser(FLfile *, FLfunc *, FLfunc *, FLfunc *);
extern	FND_EXPORT void		FLsetparser(FLfile *, FLfunc, FLfunc, FLfunc);
extern	FND_EXPORT void		FLsetform(FLfile *, FLfunc);
extern	FND_EXPORT void		FLsetlist(FLfile *, FLfunc);
extern	FND_EXPORT void		FLsetleaf(FLfile *, FLfunc);

/*	Markers					*/

extern	FND_EXPORT int		FLsetmark(FLfile *, int);
extern	FND_EXPORT int		FLdelmark(FLfile *, int);
extern	FND_EXPORT int		FLjmpmark(FLfile *, int);
extern	FND_EXPORT void		FLclearmarks(FLfile *);

/*	Math expression evaluations		*/

extern	FND_EXPORT int		FLputvar(const char *, float);
extern	FND_EXPORT float *	FLgetvaraddr(const char *);
#define	FLgetvar(s)	(*FLgetvaraddr(s))
extern	FND_EXPORT int		FLputconst(const char *, float);
extern	FND_EXPORT void		FLbindfunc(const char *,float (*)(int, void *),int);

extern	FND_EXPORT intptr_t FLeval(const char *, const char * (*)(int), float *);
extern	FND_EXPORT void *	FLucode(const char *);
extern	FND_EXPORT void		FLfreeucode(void *);
extern	FND_EXPORT int		FLuexec(const void *);
extern	FND_EXPORT void *	FLloaducode(const char *);

extern	FND_EXPORT float	FLatof(const char *);
extern	FND_EXPORT int		FLatoi(const char *);

/*	Splay trees			*/

extern	FND_EXPORT void *		FLnewsplay(int (*)(const void *, const void *));
extern	FND_EXPORT void 		FLdelsplay(void *);
extern	FND_EXPORT void 		FLsplayadd(void *, const void *, void *);
extern	FND_EXPORT void * 		FLsplayfind(void *, const void *);

/*	Miscellaneous tools			*/

extern	FND_EXPORT int			FLwait(int);
extern	FND_EXPORT const char *	FLbasename(const char *);

/*	Useful macros				*/

#ifdef NDEBUG
#define FLassert(EX, msg)   ((void)0)
#else
#define FLassert(EX, msg)   ((EX) ? (void)0 : _assert(msg,__FILE__,__LINE__))
#endif

#define	FLcreateform(f, t)	FLbgnwgroup(f, FL_FOR4, t)
#define	FLcreatelist(f, t)	FLbgnwgroup(f, FL_LIS4, t)
#define	FLcreatecat(f, t)	FLbgnwgroup(f, FL_CAT4, t)
#define	FLcreateprop(f, t)	FLbgnwgroup(f, FL_PRO4, t)

#define	FLbsbyte(b)		((int)flbitset[(unsigned char)(b)])
#define	FLbshalf(h)		(FLbsbyte((unsigned short)(h) >> 8) +		\
				 FLbsbyte((unsigned short)(h) & 0xff))
#define	FLbsword(w)		(FLbshalf((unsigned int)(w) >> 16) +		\
				 FLbshalf((unsigned int)(w) & 0xffff))

#define	FLrevbyte(b)		((int)flrevbit[(unsigned char)(b)])
#define	FLrevhalf(h)		(FLrevbyte((unsigned short)(h) >> 8) +		\
				 (FLrevbyte((unsigned short)(h) & 0xff)) << 8)
#define	FLrevword(w)		(FLrevhalf((unsigned int)(w) >> 16) +		\
				 (FLrevhalf((unsigned int)(w) & 0xffff) << 16)

#define	FLhbbyte(b)		((int)flhbitset[(unsigned char)(b)])

#ifdef lint
#define	FLgetvalist(arg, fmt)	(arg = (char *)&(fmt))
#else
#define	FLgetvalist(arg, fmt)	va_start(arg, fmt)
#endif

/*
**  Warning, this works only if <a> is a power of two
*/
#define	FLalign(a,s)		(((s) + ((a)-1)) & ~((a)-1))

/*
**  changing these two into macros would not save anything
*/

#define	FLhbhalf(h)		FLhbset((uint)h)
#define	FLhbword(w)		FLhbset((uint)w)

/*	Private stuff - might become public but for very
	low level readers/writers - for experts or gurus	*/

extern	FND_EXPORT	int		FLsetid(FLfile *, FLid, unsigned);
extern	FND_EXPORT	void		FLnewcontext(FLfile *);
extern	FND_EXPORT	void		FLfreecontext(FLfile *);
extern	FND_EXPORT	FLcontext *	FLgetcontext();
extern	FND_EXPORT	int		FLputcontext(FLcontext *);

// These functions are not exported as they should only be used internally
// by the FLIB code.
//
extern				int		FLbufferedWrite(FLfile * fp, const void *buf, unsigned nbyte);
extern				int		FLbufferedSeek(FLfile * fp, int where, int whence);

#define	FLcalign(c,s)	(((s) + (c)->align) & ~(c)->align)
#define	FLavail(c)	((c)->group.chunk.size - (c)->sofar)
#define	FLparent(c)	((FLcontext *)(c->node.prev))

/*	really privates, used by functions or macros		*/

extern	FND_EXPORT	FLfile *	FLpfilter(const char *, const char *, FLfile *);
extern	FND_EXPORT	int		FLhbset(uint i);

extern	void		FLinterrupt();
extern	FND_EXPORT	void		FLrambocloseall();
extern	FND_EXPORT	void		FLramboexit(int);
extern	FND_EXPORT	void		FLramboabort();

extern	FND_EXPORT	void		FLcatchsigcld(int);
extern	FND_EXPORT	void		FLcatchsigpipe();
extern	FND_EXPORT	void		FLpushoserror();
extern	FND_EXPORT	void		FLpoposerror();

extern	FND_EXPORT	int			FLmultiread(const char *name,
						const char **filters, FLfile **fp, int size);

/*	support for TIFF files					*/

extern	FND_EXPORT	void *		FLopentiff(FLfile *fp);

#ifdef __cplusplus
}
#endif

#endif

// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================
