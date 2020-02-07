/*
Copyright (c) 1996. 1997, 1998, 1999.  The Regents of the University of California
(Regents). All Rights Reserved.

Permission to use, copy, modify, and distribute this software and its
documentation, without fee and without a signed licensing agreement, is hereby
granted, provided that the above copyright notice, this paragraph and the
following two paragraphs appear in all copies, modifications, and
distributions.  Contact The Office of Technology Licensing, UC Berkeley, 2150
Shattuck Avenue, Suite 510, Berkeley, CA 94720-1620, (510) 643-7201, for
commercial licensing opportunities.

Written by Matt Wright, Amar Chaudhary, and Sami Khoury, The Center for New
Music and Audio Technologies, University of California, Berkeley.

     IN NO EVENT SHALL REGENTS BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT,
     SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST
     PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS
     DOCUMENTATION, EVEN IF REGENTS HAS BEEN ADVISED OF THE POSSIBILITY OF
     SUCH DAMAGE.

     REGENTS SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT
     LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
     FOR A PARTICULAR PURPOSE. THE SOFTWARE AND ACCOMPANYING
     DOCUMENTATION, IF ANY, PROVIDED HEREUNDER IS PROVIDED "AS IS".
     REGENTS HAS NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES,
     ENHANCEMENTS, OR MODIFICATIONS.

 sdif.h

 API for working with SDIF.

 SDIF spec: http://www.cnmat.berkeley.edu/SDIF/

 See sdif.c for revision history.
 version 2.3

 #include <stdio.h> before this file for FILE *.
 (A forthcoming C++ version of this libraray will not require
  stdio to be included.)

 This part of the SDIF library never allocates any memory; the memory
 required for all procedures must be passed in by the user of the library.
 See sdif-mem.h for our "officially recommended" data structures for
 SDIF in memory and procedures for dealing with them.

 This SDIF library is at the moment neither reentrant nor thread-safe.

*/

#ifndef CSOUND_SDIF_H
#define CSOUND_SDIF_H

#include "sysdep.h"

/****************************************************/
/* Create 32-bit and 64-bit int and float typedefs. */
/****************************************************/

typedef uint16_t  sdif_unicode;
typedef int32_t   sdif_int32;
typedef uint32_t  sdif_uint32;
typedef float     sdif_float32;
typedef double    sdif_float64;

#if defined(_WIN32) || defined(_WINDOWS)
  #ifndef _WINDOWS_
    #include <windows.h>
  #endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Return value convention: Most of the procedures in this library return a
   value of type SDIFresult, with 0 for success and nonzero for various error
   conditions.  Exceptions are noted in the comment describing each procedure.

   All errors encountered by the library produce an informative error message,
   but the library never prints these error messages (because the best way to
   display an error to a user is platform-specific).  Instead, the function
   SDIF_GetErrorString returns the string corresponding to any given error
   code.
*/

/* update sdif.c to reflect any changes made to these error values. */
typedef enum {
    ESDIF_SUCCESS=0,
    ESDIF_SEE_ERRNO=1,
    ESDIF_BAD_SDIF_HEADER=2,
    ESDIF_BAD_FRAME_HEADER=3,
    ESDIF_SKIP_FAILED=4,
    ESDIF_BAD_MATRIX_DATA_TYPE=5,
    ESDIF_BAD_SIZEOF=6,
    ESDIF_END_OF_DATA=7,  /* Not necessarily an error */
    ESDIF_BAD_MATRIX_HEADER=8,
    ESDIF_OBSOLETE_FILE_VERSION=9,
    ESDIF_OBSOLETE_TYPES_VERSION=10,
    ESDIF_WRITE_FAILED=11,
    ESDIF_READ_FAILED=12,
    ESDIF_OUT_OF_MEMORY=13,  /* Used only by sdif-mem.c */
    ESDIF_DUPLICATE_MATRIX_TYPE_IN_FRAME=14
} SDIFresult;

/****************************************************/
/***** These data structures match the SDIF spec ****/
/****************************************************/

/* the header for the entire SDIF data. */
typedef struct {
    char  SDIF[4];          /* must be 'S', 'D', 'I', 'F' */
    sdif_int32 size;        /* size of header frame, not including SDIF or size. */
    sdif_int32 SDIFversion;
    sdif_int32 SDIFStandardTypesVersion;
} SDIF_GlobalHeader;

/* the header for each frame of SDIF data. */
typedef struct {
    char         frameType[4];    /* should be a registered frame type */
    sdif_int32   size;            /* # bytes in this frame, not including
                                     frameType or size */
    sdif_float64 time;            /* time corresponding to frame */
    sdif_int32   streamID;        /* frames that go together have the same ID */
    sdif_int32   matrixCount;     /* number of matrices in frame */
} SDIF_FrameHeader;

/* the header for each matrix of SDIF data. */
typedef struct {
    char matrixType[4];
    sdif_int32 matrixDataType;
    sdif_int32 rowCount;
    sdif_int32 columnCount;
} SDIF_MatrixHeader;

/* Version numbers for SDIF_GlobalHeader associated with this library */
#define SDIF_SPEC_VERSION 3
#define SDIF_LIBRARY_VERSION 1

/* codes for data types used in matrices.
   these must be kept in sync with the array in sdif.c. */
typedef enum {
    SDIF_FLOAT32 = 0x0004,
    SDIF_FLOAT64 = 0x0008,
    SDIF_INT32 = 0x0104,
    SDIF_UINT32 = 0x0204,
    SDIF_UTF8 = 0x0301,
    SDIF_BYTE = 0x0401,
    SDIF_NO_TYPE = -1
} SDIF_MatrixDataType;

typedef enum {
    SDIF_FLOAT = 0,
    SDIF_INT = 1,
    SDIF_UINT = 2,
    SDIF_TEXT = 3,
    SDIF_ARBITRARY = 4
} SDIF_MatrixDataTypeHighOrder;

/****************************************************/
/*****     Procedures in the library             ****/
/****************************************************/

/* SDIF_Init --
   You must call this before any of the other SDIF procedures. */
SDIFresult SDIF_Init(void);

/* SDIF_GetErrorString --
   Returns the string representation of the given error code. */
char *SDIF_GetErrorString(SDIFresult errorcode);

/************* Opening and closing files *************/

/* SDIF_OpenWrite --
   Opens "filename" for writing and writes the global SDIF header (but does
   not flush).  The resulting FILE* is written into *resultp. */
SDIFresult SDIF_OpenWrite(const char *filename, FILE **resultp);

/* SDIF_BeginWrite --
   Same as SDIF_OpenWrite() except that it takes a FILE * already opened
   for binary writing. */
SDIFresult SDIF_BeginWrite(FILE *output);

/* SDIF_CloseWrite -- */
SDIFresult SDIF_CloseWrite(FILE *f);

/* SDIF_OpenRead --
   Opens "filename" for reading and reads and parses the header.  The return
   value will indicate any problem in the header.  After calling this the file
   pointer will be advanced to the beginning of the first frame. */
SDIFresult SDIF_OpenRead(const char *filename, FILE **resultp);

/* SDIF_BeginRead --
   Same as SDIF_OpenRead() except that it takes a FILE * already opened
   for binary reading. */
SDIFresult SDIF_BeginRead(FILE *input);

/* SDIF_CloseRead -- */
SDIFresult SDIF_CloseRead(FILE *f);

/************* Global Header *************/

/* SDIF_FillGlobalHeader --
   Writes "SDIF" into "h" and initializes the size and version members. */
void SDIF_FillGlobalHeader(SDIF_GlobalHeader *h);

/* SDIF_WriteGlobalHeader --
   Writes "h" to "f". */
SDIFresult SDIF_WriteGlobalHeader(const SDIF_GlobalHeader *h, FILE *f);

/************* Frames *************/

/* SDIF_ReadFrameHeader --
   Reads a frame header from "f" and writes it to "fh".  If you've reached the
   end of the file or stream (which is not an error), this will return
   ESDIF_END_OF_DATA.
 */
SDIFresult SDIF_ReadFrameHeader(SDIF_FrameHeader *fh, FILE *f);

/* SDIF_WriteFrameHeader --
   Writes "fh" to "f". */
SDIFresult SDIF_WriteFrameHeader(const SDIF_FrameHeader *fh, FILE *f);

/* SDIF_SkipFrame --
   Assuming that you just read an SDIF_FrameHeader and want to
   ignore the contents of the frame (e.g., because your program
   doesn't recognize its frameType), call this procedure to skip
   over the frame data, so you'll be ready to read the next
   SDIF_FrameHeader from the input.

   The arguments are a pointer to the SDIF_FrameHeader you just read
   (which includes the size count) and the open FILE *.  */
SDIFresult SDIF_SkipFrame(const SDIF_FrameHeader *fh, FILE *f);

/************* Matrices *************/

/* SDIF_ReadMatrixHeader --
   Fills "m" with the matrix header read from "f". */
SDIFresult SDIF_ReadMatrixHeader(SDIF_MatrixHeader *m, FILE *f);

/* SDIF_WriteMatrixHeader --
   Writes "m" to "f". */
SDIFresult SDIF_WriteMatrixHeader(const SDIF_MatrixHeader *m, FILE *f);

/* SDIF_GetMatrixDataTypeSize --
   Find the size in bytes of the data type indicated by "d" */
#define SDIF_GetMatrixDataTypeSize(d) ((d) & 0xff)

/* SDIF_GetMatrixDataSize --
   Returns the size in bytes of the matrix described by the given
   SDIF_MatrixHeader, including possible byte padding. */
int32_t SDIF_GetMatrixDataSize(const SDIF_MatrixHeader *m);

/* SDIF_PaddingRequired --
   Returns the number of padding bytes required after the matrix data
   according to the given SDIF_MatrixHeader. */
int32_t SDIF_PaddingRequired(const SDIF_MatrixHeader *m);

/* SDIF_SkipMatrix --
   Assuming that you just read an SDIF_MatrixHeader and want to ignore the
   contents of this matrix (e.g., because your program doesn't recognize its
   matrixType), call this procedure to skip over the matrix data.  It will leave
   the file pointer pointing at the next matrix after the one you skipped,
   or pointing at the next frame header if the one we skipped was the last one
   in the frame.  */
SDIFresult SDIF_SkipMatrix(const SDIF_MatrixHeader *head, FILE *f);

/* SDIF_ReadMatrixData --
   Assuming that you just read an SDIF_MatrixHeader and want to read the
   matrix data itself into a block of memory that you've allocated, call
   this procedure to do so.  Handles big/little endian issues.  */
SDIFresult SDIF_ReadMatrixData(void *putItHere, FILE *f,
                               const SDIF_MatrixHeader *head);

/************* Stream IDs *************/

/* SDIF_UniqueStreamID --
   Each call to this procedure returns a locally unique stream ID: 1, 2, 3...
   This procedure is not thread-safe. */
sdif_int32 SDIF_UniqueStreamID(void);

/************* 4-byte character arrays *************/

/* SDIF_Char4Eq --
   Checks two 4-byte character arrays for equality.  Returns zero if they
   differ, nonzero if they're the same. */
int32_t SDIF_Char4Eq(const char *thisone, const char *thatone);

/* SDIF_Copy4Bytes --
   Copies 4 bytes (e.g., "SDIF") into a 4-byte char array. */
void SDIF_Copy4Bytes(char *target, const char *string);

/* SDIF_Read and SDIF_Write --

   Abstract away big endian/little endian in reading/writing 1, 2, 4, and 8
   byte data.

   These procedures all take arguments just like fwrite() and fread(), except
   that the size of the objects you're writing is determined by which function
   you call instead of an explicit argument.  Also, they do little-endian
   conversion when necessary. */

#ifndef LITTLE_ENDIAN
#  define LITTLE_ENDIAN 1
#endif

#if defined(__POWERPC__) || defined(__PPC__) || defined(__ppc__)
#  undef LITTLE_ENDIAN
#endif

SDIFresult SDIF_Write1(const void *block, size_t n, FILE *f);
SDIFresult SDIF_Write2(const void *block, size_t n, FILE *f);
SDIFresult SDIF_Write4(const void *block, size_t n, FILE *f);
SDIFresult SDIF_Write8(const void *block, size_t n, FILE *f);

SDIFresult SDIF_Read1(void *block, size_t n, FILE *f);
SDIFresult SDIF_Read2(void *block, size_t n, FILE *f);
SDIFresult SDIF_Read4(void *block, size_t n, FILE *f);
SDIFresult SDIF_Read8(void *block, size_t n, FILE *f);

#ifdef __cplusplus
}
#endif

#endif /* __SDIF_H */

