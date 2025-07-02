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

  sdif.c

  Utilities for formatting data into SDIF

  SDIF spec: http://www.cnmat.berkeley.edu/SDIF/

 Matt Wright, 12/4/96

 5/12/97 version 1.1 by Matt and Amar, incorporating little-endian issues

 1/12/98 version 1.2 by Amar, conforms to revised SDIF spec

 11/5/1998 version 1.3 by Sami, changed error reporting, included stdio.h,
        and added SDIF_BeginRead().

 11/13/1998 version 2.0 by Sami, renamed functions and types to use "sdif"
        and "SDIF" prefixes, changed internal error reporting, added
        documentation in sdif.h, and incremented major version because
        programs which included sdif.h will no longer compile without changes.

 11/16/1998 version 2.0.1 by Sami, added sdif_int32, sdif_uint32, and
        sdif_unicode typedefs.  Added SDIF_GetMatrixDataTypeSize().

 12/1/1998 version 2.0.2 by Sami, created SDIF_Matrix, SDIF_Frame, and
        SDIF_Stream types and a set of functions for creating and manipulating
        them.  This was done as the foundation for continued work on SDIF
        streaming.

 1/25/99 version 2.1 by Matt: added SDIF_Init() to allow memory allocation
    in Max/MSP; lots of other little fixes.

 9/20/99 version 2.2 by Matt: Incorporating changes to the format
    from my 6/99 visit to IRCAM.  Moved memory stuff to sdif-mem.[ch].

 10/1/99 version 2.3 by Matt: minor fixes for public release.
 10/12/99 Version 2.4 by Matt: changed return value convention

 18:2:2000 RWD:  added old version structs and functions for use with sdif2mp4.
          NB struct defs are now as plain struct, pointers declared explicitly
 4:8:2000 RWD: made all func calls test explicitly against ESDIF_SUCCESS
*/

#define REALASSERT
#ifdef REALASSERT
#include <assert.h>
#else
#define assert(x) /* Do nothing */
#endif

#include "sysdep.h"
#include <errno.h>
#include "sdif.h"

/* prototypes for functions used only in this file. */
static int32_t SizeofSanityCheck(void);
static SDIFresult SkipBytes(FILE *f, int32_t numBytes);

/* error handling stuff. */
static char *error_string_array[] = {
    "Everything's cool",
    "This program should display strerror(errno) instead of this string",
    "Bad SDIF header",
    "Frame header's size is too low for time tag and stream ID",
    "fseek() failed while skipping over data",
    "Unknown matrix data type encountered in SDIF_WriteFrame().",
    (char *) NULL,   /* this will be set by SizeofSanityCheck() */
    "End of data",
    "Bad SDIF matrix header",
    "Obsolete SDIF file from an old version of SDIF",
    "Obsolete version of the standard SDIF frame and matrix types",
    "I/O error: couldn't write",
    "I/O error: couldn't read",
    "Out of memory",
    "Frame has two matrices with the same MatrixType"
};

char *SDIF_GetErrorString(SDIFresult error_code)
{
    return error_string_array[error_code];
}

SDIFresult SDIF_Init(void)
{
    if (!SizeofSanityCheck()) {
      return ESDIF_BAD_SIZEOF;
    }
    return ESDIF_SUCCESS;
}

SDIFresult SDIF_OpenWrite(const char *filename, FILE **resultp)
{
    FILE *result;
    SDIFresult r;

    if ((result = fopen(filename, "wb")) == NULL) {
      return ESDIF_SEE_ERRNO;
    }
    if ((r = SDIF_BeginWrite(result))!= ESDIF_SUCCESS) {
      fclose(result);
      return r;
    }
    *resultp = result;
    return ESDIF_SUCCESS;
}

SDIFresult SDIF_BeginWrite(FILE *output)
{
    SDIF_GlobalHeader h;

    SDIF_FillGlobalHeader(&h);
    return SDIF_WriteGlobalHeader(&h, output);
}

SDIFresult SDIF_CloseWrite(FILE *f)
{
    fflush(f);
    if (fclose(f) == 0) {
      return ESDIF_SUCCESS;
    }
    else {
      return ESDIF_SEE_ERRNO;
    }
}

SDIFresult SDIF_OpenRead(const char *filename, FILE **resultp)
{
    FILE *result = NULL;
    SDIFresult r;

    if ((result = fopen(filename, "rb")) == NULL) {
      return ESDIF_SEE_ERRNO;
    }

    if ((r = SDIF_BeginRead(result))!=ESDIF_SUCCESS) {
      fclose(result);
      return r;
    }

    *resultp = result;
    return ESDIF_SUCCESS;
}

SDIFresult SDIF_BeginRead(FILE *input)
{
    SDIF_GlobalHeader sgh;
    SDIFresult r;

    /* make sure the header is OK. */
    if ((r = SDIF_Read1(sgh.SDIF, 4, input))!=ESDIF_SUCCESS)
      return r;
    if (!SDIF_Char4Eq(sgh.SDIF, "SDIF"))
      return ESDIF_BAD_SDIF_HEADER;
    if ((r = SDIF_Read4(&sgh.size, 1, input))!=ESDIF_SUCCESS)
      return r;
    if (sgh.size % 8 != 0)
      return ESDIF_BAD_SDIF_HEADER;
    if (sgh.size < 8)
      return ESDIF_BAD_SDIF_HEADER;
    if ((r = SDIF_Read4(&sgh.SDIFversion, 1, input))!=ESDIF_SUCCESS)
      return r;
    if ((r = SDIF_Read4(&sgh.SDIFStandardTypesVersion, 1, input))!=ESDIF_SUCCESS)
      return r;

    if (sgh.SDIFversion != 3) {         /*RWD was < 3 */
      return ESDIF_OBSOLETE_FILE_VERSION;
    }

    if (sgh.SDIFStandardTypesVersion < 1) {
      return ESDIF_OBSOLETE_TYPES_VERSION;
    }

    /* skip size-8 bytes.  (We already read the first two version numbers,
       but maybe there's more data in the header frame.) */

    if (sgh.size == 8) {
      return ESDIF_SUCCESS;
    }

    if (SkipBytes(input, sgh.size-8)!=ESDIF_SUCCESS) {
      return ESDIF_BAD_SDIF_HEADER;
    }

    return ESDIF_SUCCESS;
}

SDIFresult SDIF_CloseRead(FILE *f)
{
    if (fclose(f) == 0) {
      return ESDIF_SUCCESS;
    }
    else {
      return ESDIF_SEE_ERRNO;
    }
}

void SDIF_FillGlobalHeader(SDIF_GlobalHeader *h)
{
    assert(h != NULL);
    SDIF_Copy4Bytes(h->SDIF, "SDIF");
    h->size = 8;
    h->SDIFversion = SDIF_SPEC_VERSION;
    h->SDIFStandardTypesVersion = SDIF_LIBRARY_VERSION;
}

SDIFresult SDIF_WriteGlobalHeader(const SDIF_GlobalHeader *h, FILE *f)
{
    SDIFresult r;

    assert(h != NULL);
    assert(f != NULL);
#ifdef LITTLE_ENDIAN
    if ((r = SDIF_Write1(&(h->SDIF), 4, f))!=ESDIF_SUCCESS)
      return r;
    if ((r = SDIF_Write4(&(h->size), 1, f))!=ESDIF_SUCCESS)
      return r;
    if ((r = SDIF_Write4(&(h->SDIFversion), 1, f))!=ESDIF_SUCCESS)
      return r;
    if ((r = SDIF_Write4(&(h->SDIFStandardTypesVersion), 1, f))!=ESDIF_SUCCESS)
      return r;
    return ESDIF_SUCCESS;
#else
    return (fwrite(h, sizeof(*h), 1, f) == 1) ?ESDIF_SUCCESS:ESDIF_WRITE_FAILED;
#endif
}

SDIFresult SDIF_ReadFrameHeader(SDIF_FrameHeader *fh, FILE *f)
{
#ifdef LITTLE_ENDIAN
    SDIFresult r;

    if (SDIF_Read1(&(fh->frameType),4,f) != ESDIF_SUCCESS) {     /*RWD*/
      if (feof(f)) {
        return ESDIF_END_OF_DATA;
      }
      return ESDIF_READ_FAILED;
    }

    if ((r = SDIF_Read4(&(fh->size),1,f))!=ESDIF_SUCCESS)
      return r;
    if ((r = SDIF_Read8(&(fh->time),1,f))!=ESDIF_SUCCESS)
      return r;
    if ((r = SDIF_Read4(&(fh->streamID),1,f))!=ESDIF_SUCCESS)
      return r;
    if ((r = SDIF_Read4(&(fh->matrixCount),1,f))!=ESDIF_SUCCESS)
      return r;
    return ESDIF_SUCCESS;
#else
    size_t amount_read;

    amount_read = fread(fh, sizeof(*fh), 1, f);
    if (amount_read == 1)
      return ESDIF_SUCCESS;
    if (amount_read == 0) {
      /* Now that fread failed, maybe we're at EOF. */
      if (feof(f)) {
        return ESDIF_END_OF_DATA;
      }
    }
    return ESDIF_READ_FAILED;
#endif /* LITTLE_ENDIAN */
}

SDIFresult SDIF_WriteFrameHeader(const SDIF_FrameHeader *fh, FILE *f)
{

#ifdef LITTLE_ENDIAN
    SDIFresult r;

    if ((r = SDIF_Write1(&(fh->frameType),4,f))!=ESDIF_SUCCESS)
      return r;
    if ((r = SDIF_Write4(&(fh->size),1,f))!=ESDIF_SUCCESS)
      return r;
    if ((r = SDIF_Write8(&(fh->time),1,f))!=ESDIF_SUCCESS)
      return r;
    if ((r = SDIF_Write4(&(fh->streamID),1,f))!=ESDIF_SUCCESS)
      return r;
    if ((r = SDIF_Write4(&(fh->matrixCount),1,f))!=ESDIF_SUCCESS)
      return r;
#ifdef _WIN32
    fflush(f);
#endif
    return ESDIF_SUCCESS;
#else

    return (fwrite(fh, sizeof(*fh), 1, f) == 1)?ESDIF_SUCCESS:ESDIF_WRITE_FAILED;

#endif
}

SDIFresult SDIF_SkipFrame(const SDIF_FrameHeader *head, FILE *f)
{
    /* The header's size count includes the 8-byte time tag, 4-byte
       stream ID and 4-byte matrix count that we already read. */
    int32_t bytesToSkip = head->size - 16;

    if (bytesToSkip < 0) {
      return ESDIF_BAD_FRAME_HEADER;
    }

    return SkipBytes(f, bytesToSkip);
}

SDIFresult SDIF_ReadMatrixHeader(SDIF_MatrixHeader *m, FILE *f)
{
#ifdef LITTLE_ENDIAN
    SDIFresult r;
    if ((r = SDIF_Read1(&(m->matrixType),4,f))!=ESDIF_SUCCESS)
      return r;
    if ((r = SDIF_Read4(&(m->matrixDataType),1,f))!=ESDIF_SUCCESS)
      return r;
    if ((r = SDIF_Read4(&(m->rowCount),1,f))!=ESDIF_SUCCESS)
      return r;
    if ((r = SDIF_Read4(&(m->columnCount),1,f))!=ESDIF_SUCCESS)
      return r;
    return ESDIF_SUCCESS;
#else
    if (fread(m, sizeof(*m), 1, f) == 1) {
      return ESDIF_SUCCESS;
    }
    else {
      return ESDIF_READ_FAILED;
    }
#endif

}

SDIFresult SDIF_WriteMatrixHeader(const SDIF_MatrixHeader *m, FILE *f)
{
#ifdef LITTLE_ENDIAN
    SDIFresult r;
    if ((r = SDIF_Write1(&(m->matrixType),4,f))!=ESDIF_SUCCESS)
      return r;
    if ((r = SDIF_Write4(&(m->matrixDataType),1,f))!=ESDIF_SUCCESS)
      return r;
    if ((r = SDIF_Write4(&(m->rowCount),1,f))!=ESDIF_SUCCESS)
      return r;
    if ((r = SDIF_Write4(&(m->columnCount),1,f))!=ESDIF_SUCCESS)
      return r;
    return ESDIF_SUCCESS;
#else
    return (fwrite(m, sizeof(*m), 1, f) == 1) ? ESDIF_SUCCESS:ESDIF_READ_FAILED;
#endif
}

int32_t SDIF_GetMatrixDataSize(const SDIF_MatrixHeader *m)
{
    int32_t size;
    size = SDIF_GetMatrixDataTypeSize(m->matrixDataType) *
           m->rowCount * m->columnCount;

    if ((size % 8) != 0) {
      size += (8 - (size % 8));
    }

    return size;
}

int32_t SDIF_PaddingRequired(const SDIF_MatrixHeader *m)
{
    int32_t size;
    size = SDIF_GetMatrixDataTypeSize(m->matrixDataType) *
           m->rowCount * m->columnCount;

    if ((size % 8) != 0) {
      return (8 - (size % 8));
    }
    else {
      return 0;
    }
}

SDIFresult SDIF_SkipMatrix(const SDIF_MatrixHeader *head, FILE *f)
{
    int32_t size = SDIF_GetMatrixDataSize(head);

    if (size < 0) {
      return ESDIF_BAD_MATRIX_HEADER;
    }

    return SkipBytes(f, size);
}

SDIFresult
SDIF_ReadMatrixData(void *putItHere, FILE *f, const SDIF_MatrixHeader *head)
{
    size_t datumSize = (size_t) SDIF_GetMatrixDataTypeSize(head->matrixDataType);
    size_t numItems = (size_t) (head->rowCount * head->columnCount);

#ifdef LITTLE_ENDIAN
    switch (datumSize) {
    case 1:
      return SDIF_Read1(putItHere, numItems, f);
    case 2:
      return SDIF_Read2(putItHere, numItems, f);
    case 4:
      return SDIF_Read4(putItHere, numItems, f);
    case 8:
      return SDIF_Read8(putItHere, numItems, f);
    default:
      return ESDIF_BAD_MATRIX_DATA_TYPE;
    }
    /* This is never reached */
    return ESDIF_SUCCESS;
#else
    if (fread(putItHere, datumSize, numItems, f) == numItems) {
      return ESDIF_SUCCESS;
    }
    else {
      return ESDIF_READ_FAILED;
    }
#endif
}

sdif_int32 SDIF_UniqueStreamID(void)
{
    static int32_t id;
    return ++id;
}

int32_t SDIF_Char4Eq(const char *ths, const char *that)
{
    return ths[0] == that[0] && ths[1] == that[1] &&
           ths[2] == that[2] && ths[3] == that[3];
}

void SDIF_Copy4Bytes(char *target, const char *string)
{
    target[0] = string[0];
    target[1] = string[1];
    target[2] = string[2];
    target[3] = string[3];
}

#ifdef LITTLE_ENDIAN
#define BUFSIZE 4096
static  char    p[BUFSIZE];
#endif

SDIFresult SDIF_Write1(const void *block, size_t n, FILE *f)
{
    return (fwrite (block,1,n,f) == n) ? ESDIF_SUCCESS : ESDIF_WRITE_FAILED;
}

SDIFresult SDIF_Write2(const void *block, size_t n, FILE *f)
{
#ifdef LITTLE_ENDIAN
    SDIFresult r;
    const char *q = block;
    int32_t i;
    size_t m = 2*n;

    if ((n << 1) > BUFSIZE) {
      /* Too big for buffer */
      int32_t num = BUFSIZE >> 1;
      if ((r = SDIF_Write2(block, num, f))!=ESDIF_SUCCESS)
        return r;
      return SDIF_Write2(((char *) block) + num, n-num, f);
    }

    for (i = 0; i < m; i += 2) {
      p[i] = q[i+1];
      p[i+1] = q[i];
    }

    return (fwrite(p,2,n,f)==n) ? ESDIF_SUCCESS : ESDIF_WRITE_FAILED;

#else
    return (fwrite (block,2,n,f) == n) ? ESDIF_SUCCESS : ESDIF_WRITE_FAILED;
#endif
}

SDIFresult SDIF_Write4(const void *block, size_t n, FILE *f)
{
#ifdef LITTLE_ENDIAN
    SDIFresult r;
    const char *q = block;
    int32_t i;
    size_t m = 4*n;

    if ((n << 2) > BUFSIZE) {
      int32_t num = BUFSIZE >> 2;
      if ((r = SDIF_Write4(block, num, f))!=ESDIF_SUCCESS)
        return r;
      return SDIF_Write4(((char *) block) + num, n-num, f);
    }

    for (i = 0; i < m; i += 4) {
      p[i] = q[i+3];
      p[i+3] = q[i];
      p[i+1] = q[i+2];
      p[i+2] = q[i+1];
    }

    return (fwrite(p,4,n,f) == n) ? ESDIF_SUCCESS : ESDIF_WRITE_FAILED;
#else
    return (fwrite(block,4,n,f) == n) ? ESDIF_SUCCESS : ESDIF_WRITE_FAILED;
#endif
}

SDIFresult SDIF_Write8(const void *block, size_t n, FILE *f)
{
#ifdef LITTLE_ENDIAN
    SDIFresult r;
    const char *q = block;
    int32_t i;
    size_t m = 8*n;

    if ((n << 3) > BUFSIZE) {
      int32_t num = BUFSIZE >> 3;
      if ((r = SDIF_Write8(block, num, f))!=ESDIF_SUCCESS)
        return r;
      return SDIF_Write8(((char *) block) + num, n-num, f);
    }

    for (i = 0; i < m; i += 8) {
      p[i] = q[i+7];
      p[i+7] = q[i];
      p[i+1] = q[i+6];
      p[i+6] = q[i+1];
      p[i+2] = q[i+5];
      p[i+5] = q[i+2];
      p[i+3] = q[i+4];
      p[i+4] = q[i+3];
    }

    return (fwrite(p,8,n,f) == n) ? ESDIF_SUCCESS : ESDIF_WRITE_FAILED;
#else
    return (fwrite(block,8,n,f) == n) ? ESDIF_SUCCESS : ESDIF_WRITE_FAILED;
#endif
}

SDIFresult SDIF_Read1(void *block, size_t n, FILE *f)
{
    return (fread (block,1,n,f) == n) ? ESDIF_SUCCESS : ESDIF_READ_FAILED;
}

SDIFresult SDIF_Read2(void *block, size_t n, FILE *f)
{
#ifdef LITTLE_ENDIAN
    SDIFresult r;
    char *q = block;
    int32_t i;
    size_t m = 2*n;

    if ((n << 1) > BUFSIZE) {
      int32_t num = BUFSIZE >> 1;
      if ((r = SDIF_Read2(block, num, f))!=ESDIF_SUCCESS)
        return r;
      return SDIF_Read2(((char *) block) + num, n-num, f);
    }

    if (fread(p,2,n,f) != n)
      return ESDIF_READ_FAILED;

    for (i = 0; i < m; i += 2) {
      q[i] = p[i+1];
      q[i+1] = p[i];
    }

    return ESDIF_SUCCESS;
#else
    return (fread(block,2,n,f) == n) ? ESDIF_SUCCESS : ESDIF_READ_FAILED;
#endif

}

SDIFresult SDIF_Read4(void *block, size_t n, FILE *f)
{
#ifdef LITTLE_ENDIAN
    SDIFresult r;
    char *q = block;
    int32_t i;
    size_t m = 4*n;

    if ((n << 2) > BUFSIZE) {
      int32_t num = BUFSIZE >> 2;
      if ((r = SDIF_Read4(block, num, f))!=ESDIF_SUCCESS)
        return r;
      return SDIF_Read4(((char *) block) + num, n-num, f);
    }

    if (fread(p,4,n,f) != n) return ESDIF_READ_FAILED;

    for (i = 0; i < m; i += 4) {
      q[i] = p[i+3];
      q[i+3] = p[i];
      q[i+1] = p[i+2];
      q[i+2] = p[i+1];
    }

    return ESDIF_SUCCESS;

#else
    return (fread(block,4,n,f) == n) ? ESDIF_SUCCESS : ESDIF_READ_FAILED;
#endif

}

SDIFresult SDIF_Read8(void *block, size_t n, FILE *f)
{
#ifdef LITTLE_ENDIAN
    SDIFresult r;
    char *q = block;
    int32_t i;
    size_t m = 8*n;

    if ((n << 3) > BUFSIZE) {
      int32_t num = BUFSIZE >> 3;
      if ((r = SDIF_Read8(block, num, f))!=ESDIF_SUCCESS)
        return r;
      return SDIF_Read8(((char *) block) + num, n-num, f);
    }

    if (fread(p,8,n,f) != n)
      return ESDIF_READ_FAILED;

    for (i = 0; i < m; i += 8) {
      q[i] = p[i+7];
      q[i+7] = p[i];
      q[i+1] = p[i+6];
      q[i+6] = p[i+1];
      q[i+2] = p[i+5];
      q[i+5] = p[i+2];
      q[i+3] = p[i+4];
      q[i+4] = p[i+3];
    }

    return ESDIF_SUCCESS;

#else
    return (fread(block,8,n,f) == n) ? ESDIF_SUCCESS : ESDIF_READ_FAILED;
#endif
}

/* static function definitions follow. */

static int32_t SizeofSanityCheck(void)
{
    int32_t OOK = 1;
    static char errorMessage[sizeof("sizeof(sdif_float64) is 999!!!")];

    if (sizeof(sdif_int32) != 4) {
      snprintf(errorMessage, sizeof("sizeof(sdif_float64) is 999!!!"),
              "sizeof(sdif_int32) is %d!", (int32_t)sizeof(sdif_int32));
      OOK = 0;
    }

    if (sizeof(sdif_float32) != 4) {
      snprintf(errorMessage, sizeof("sizeof(sdif_float64) is 999!!!"),
               "sizeof(sdif_float32) is %d!",
              (int32_t)sizeof(sdif_float32));
      OOK = 0;
    }

    if (sizeof(sdif_float64) != 8) {
      snprintf(errorMessage, sizeof("sizeof(sdif_float64) is 999!!!"),
              "sizeof(sdif_float64) is %d!",
              (int32_t)sizeof(sdif_float64));
      OOK = 0;
    }

    if (!OOK) {
      error_string_array[ESDIF_BAD_SIZEOF] = errorMessage;
    }
    return OOK;
}

static SDIFresult SkipBytes(FILE *f, int32_t bytesToSkip)
{
#ifdef STREAMING
    /* Can't fseek in a stream, so waste some time needlessly copying
       some bytes in memory */
    {
#define BLOCK_SIZE 1024
      char buf[BLOCK_SIZE];
      while (bytesToSkip > BLOCK_SIZE) {
        if (fread (buf, BLOCK_SIZE, 1, f) != 1) {
          return ESDIF_READ_FAILED;
        }
        bytesToSkip -= BLOCK_SIZE;
      }

      if (fread (buf, bytesToSkip, 1, f) != 1) {
        return ESDIF_READ_FAILED;
      }
    }
#else
    /* More efficient implementation */
    if (fseek(f, bytesToSkip, SEEK_CUR) != 0) {
      return ESDIF_SKIP_FAILED;
    }
#endif
   return ESDIF_SUCCESS;
}

