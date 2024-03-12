/*
Copyright (c) 1996, 1997, 1998, 1999.  The Regents of the University of
California (Regents).  All Rights Reserved.

Permission to use, copy, modify, and distribute this software and its
documentation, without fee and without a signed licensing agreement, is hereby
granted, provided that the above copyright notice, this paragraph and the
following two paragraphs appear in all copies, modifications, and
distributions.  Contact The Office of Technology Licensing, UC Berkeley, 2150
Shattuck Avenue, Suite 510, Berkeley, CA 94720-1620, (510) 643-7201, for
commercial licensing opportunities.

Written by Matt Wright and Sami Khoury, The Center for New Music and Audio
Technologies, University of California, Berkeley.

     IN NO EVENT SHALL REGENTS BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT,
     SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS,
     ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF
     REGENTS HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

     REGENTS SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT
     LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
     FOR A PARTICULAR PURPOSE. THE SOFTWARE AND ACCOMPANYING
     DOCUMENTATION, IF ANY, PROVIDED HEREUNDER IS PROVIDED "AS IS".
     REGENTS HAS NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES,
     ENHANCEMENTS, OR MODIFICATIONS.
*/

/* sdif-mem.h

   Recommended data structures for storing SDIF data in memory.  It stores
   frame and matrix headers with the standard byte encoding, and it stores the
   matrices themselves just as they appear in the file, but everything else is
   done with linked lists rather than counting the sizes of things and knowing
   that the next one must appear right after this one.

Version 1.0, 9/21/99

*/

/* Return value/error message conventions: same as sdif.[ch], except
   for the constructor procedures, which return a pointer to the new object
   or zero if out of memory. */

/* Structs defining our data structures.  Note that they contain the
   frame and matrix header structs from sdif.h */
/*RWD TODO: declare objects, no pounters */
#ifndef CSOUND_SDIF_MEM_H
#define CSOUND_SDIF_MEM_H
#include "sdif.h"

typedef struct SDIFmemMatrixStruct {
    SDIF_MatrixHeader header;
    void *data;
    struct SDIFmemMatrixStruct *next;
} *SDIFmem_Matrix;

typedef struct SDIFmemFrameStruct {
    SDIF_FrameHeader header;
    SDIFmem_Matrix matrices;  /* linked list of matrices. */
    struct SDIFmemFrameStruct *prev;
    struct SDIFmemFrameStruct *next;
} *SDIFmem_Frame;

/* SDIFmem_Init --
   You must call this before any of the other procedures in this library.  You
   pass in the procedures that will be used for malloc() and free(). */
SDIFresult SDIFmem_Init(void *(*MemoryAllocator)(int32_t numBytes),
              void (*MemoryFreer)(void *memory, int32_t numBytes));

/* Constructors */

/* SDIFmem_CreateEmptyFrame --
   Returns 0 if out of memory. */
SDIFmem_Frame SDIFmem_CreateEmptyFrame(void);

/* SDIFmem_CreateEmptyMatrix --
   Note that the data pointer will be a null pointer; you must allocate the
   memory to hold the matrix data itself.  Returns 0 if out of memory.*/
SDIFmem_Matrix SDIFmem_CreateEmptyMatrix(void);

/* SDIFmem_FreeFrame --
   Frees the given frame and all of its matrices.  It's your job to repair
   any next and prev pointers pointing to this frame */
void SDIFmem_FreeFrame(SDIFmem_Frame f);

/* SDIFmem_FreeMatrix --
   Frees the given matrix.  It's your job to repair any next pointers pointing
   to this matrix.  If (m->data != 0) it calls MemoryFreer on m->data too. */
void SDIFmem_FreeMatrix(SDIFmem_Matrix m);

/* SDIFmem_RepairFrameHeader ---
   If you've been playing with the matrices in an SDIFmem_Frame, call this
   to recompute the size of the frame and the number of matrices. */
void SDIFmem_RepairFrameHeader(SDIFmem_Frame f);

/* SDIFmem_ReadFrameContents --
   Assuming that you just read an SDIF_FrameHeader and decided that you want
   to read this frame into memory, call this procedure.  It will allocate a
   new SDIFmem_Frame, copy the contents of your SDIF_FrameHeader into it,
   initialize the prev and next pointers to 0, read all of the frame's
   matrices from the file and store them in newly allocated SDIFmem_Matrix
   structures, and put the result in the supplied pointer argument. */
SDIFresult SDIFmem_ReadFrameContents(SDIF_FrameHeader *head, FILE *f,
                                     SDIFmem_Frame *putithere);

/* SDIFmem_ReadFrame --
   Just like SDIFmem_ReadFrameContents, but it also reads the frame header
   for you.  (For use when you know you want to read the next frame into memory
   even without peeking at the header.)
*/
SDIFresult SDIFmem_ReadFrame(FILE *f, SDIFmem_Frame *putithere);

/* SDIFmem_AddMatrix --
   Add a new matrix to an existing frame.  Checks to make sure this matrix
   doesn't have the same MatrixType as any other matrix already in the frame.
   Updates the size and numMatrices fields in the frame header. */
SDIFresult SDIFmem_AddMatrix(SDIFmem_Frame f, SDIFmem_Matrix m);

/* Write the given SDIFmem_Frame, including the frame header and all the
   matrices, to the given file handle.  */
SDIFresult SDIFmem_WriteFrame(FILE *sdif_handle, SDIFmem_Frame f);

/* Write the given SDIFmem_Matrix, including the matrix header, matrix data,
   and possible padding bytes, to the given file handle.  Usually you want
   to call SDIFmem_WriteFrame() instead. */
SDIFresult SDIFmem_WriteMatrix(FILE *sdif_handle, SDIFmem_Matrix m);

#endif

