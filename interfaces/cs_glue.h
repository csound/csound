/*
    cs_glue.h:

    Copyright (C) 2005 Istvan Varga

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#ifndef CSOUND_CS_GLUE_H
#define CSOUND_CS_GLUE_H

#ifdef __cplusplus
extern "C" {
#endif

  /**
   * Creates a buffer for storing messages printed by Csound.
   * Should be called after creating a Csound instance; note that
   * the message buffer uses the host data pointer, and the buffer
   * should be freed by calling csoundDestroyMessageBuffer() before
   * deleting the Csound instance.
   * If 'toStdOut' is non-zero, the messages are also printed to
   * stdout and stderr (depending on the type of the message),
   * in addition to being stored in the buffer.
   */
  void csoundEnableMessageBuffer(CSOUND *csound, int toStdOut);

  /**
   * Returns the first message from the buffer.
   */
  const char *csoundGetFirstMessage(CSOUND *csound);

  /**
   * Returns the attribute parameter (see msg_attr.h) of the first message
   * in the buffer.
   */
  int csoundGetFirstMessageAttr(CSOUND *csound);

  /**
   * Removes the first message from the buffer.
   */
  void csoundPopFirstMessage(CSOUND *csound);

  /**
   * Returns the number of pending messages in the buffer.
   */
  int csoundGetMessageCnt(CSOUND *csound);

  /**
   * Releases all memory used by the message buffer.
   */
  void csoundDestroyMessageBuffer(CSOUND *csound);

#ifdef __cplusplus
}       /* extern "C" */
#endif

#endif  /* CSOUND_CS_GLUE_H */

