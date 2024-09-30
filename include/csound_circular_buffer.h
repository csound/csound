/*
  csound_circular_buffer.h: utility circular buffer functions

  Copyright (C) 2024 V Lazzarini

  This file is part of Csound.

  The Csound Library is free software; you can redistribute it
  and/or modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  Csound is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with Csound; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
  02110-1301 USA
*/

#ifndef CS_CIRCULAR_BUFFER_H
#define CS_CIRCULAR_BUFFER_H

#ifdef __cplusplus
extern "C" {
#endif
 
  /** @}*/
  /** @defgroup CIRCULARBUFFER Circular buffer functions
   *
   *  @{ */
   /**
   * Create circular buffer with numelem number of elements. The
   * element's size is set from elemsize. It should be used like:
   *@code
   * void *rb = csoundCreateCircularBuffer(csound, 1024, sizeof(MYFLT));
   *@endcode
   */
  PUBLIC void *csoundCreateCircularBuffer(CSOUND *csound,
                                          int32_t numelem, int32_t elemsize);

  /**
   * Read from circular buffer
   * @param csound This value is currently ignored.
   * @param circular_buffer pointer to an existing circular buffer
   * @param out preallocated buffer with at least items number of elements, where
   *              buffer contents will be read into
   * @param items number of samples to be read
   * @returns the actual number of items read (0 <= n <= items)
   */
  PUBLIC int32_t csoundReadCircularBuffer(CSOUND *csound, void *circular_buffer,
                                      void *out, int32_t items);

  /**
   * Read from circular buffer without removing them from the buffer.
   * @param circular_buffer pointer to an existing circular buffer
   * @param out preallocated buffer with at least items number of elements, where
   *              buffer contents will be read into
   * @param items number of samples to be read
   * @returns the actual number of items read (0 <= n <= items)
   */
  PUBLIC int32_t csoundPeekCircularBuffer(CSOUND *csound, void *circular_buffer,
                                      void *out, int32_t items);

  /**
   * Write to circular buffer
   * @param csound This value is currently ignored.
   * @param p pointer to an existing circular buffer
   * @param inp buffer with at least items number of elements to be written into
   *              circular buffer
   * @param items number of samples to write
   * @returns the actual number of items written (0 <= n <= items)
   */
  PUBLIC int32_t csoundWriteCircularBuffer(CSOUND *csound, void *p,
                                       const void *inp, int32_t items);
  /**
   * Empty circular buffer of any remaining data. This function should only be
   * used if there is no reader actively getting data from the buffer.
   * @param csound This value is currently ignored.
   * @param p pointer to an existing circular buffer
   */
  PUBLIC void csoundFlushCircularBuffer(CSOUND *csound, void *p);

  /**
   * Free circular buffer
   */
  PUBLIC void csoundDestroyCircularBuffer(CSOUND *csound, void *circularbuffer);
  /** @}*/
#ifdef __cplusplus
}
#endif
 
#endif
