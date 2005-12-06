/*
    cs_glue.hpp:

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

#ifndef CSOUND_CS_GLUE_HPP
#define CSOUND_CS_GLUE_HPP

#include "csound.h"

extern "C" {

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

  /**
   * Creates a pointer to an array of MYFLT values, optionally allocating
   * space for 'cnt' elements if 'cnt' is greater than zero. For use with
   * csoundGetChannelPtr(), 'cnt' should be zero.
   */
  MYFLT **CreateMYFLTArrayPtr(int cnt);

  /**
   * Returns non-zero if 'p' is a valid MYFLT array.
   */
  int MYFLTArrayIsValid(MYFLT **p);

  /**
   * Returns a pointer to the element of MYFLT array 'p' at index 'ndx'
   * (counting from zero). 'p' and 'ndx' are assumed to be valid.
   */
  MYFLT *MYFLTArrayGetPtr(MYFLT **p, int ndx);

  /**
   * Sets a value in MYFLT array 'p' at index 'ndx' (counting from zero).
   * No error checking is done, 'p' and 'ndx' are assumed to be valid.
   */
  void MYFLTArraySet(MYFLT **p, MYFLT value, int ndx);

  /**
   * Sets two values in MYFLT array 'p' starting at index 'ndx' (counting
   * from zero). No error checking is done, 'p' and 'ndx' are assumed to
   * be valid.
   */
  void MYFLTArraySet2(MYFLT **p, MYFLT v0, MYFLT v1, int ndx);

  /**
   * Sets three values in MYFLT array 'p' starting at index 'ndx' (counting
   * from zero). No error checking is done, 'p' and 'ndx' are assumed to
   * be valid.
   */
  void MYFLTArraySet3(MYFLT **p, MYFLT v0, MYFLT v1, MYFLT v2, int ndx);

  /**
   * Sets four values in MYFLT array 'p' starting at index 'ndx' (counting
   * from zero). No error checking is done, 'p' and 'ndx' are assumed to
   * be valid.
   */
  void MYFLTArraySet4(MYFLT **p, MYFLT v0, MYFLT v1, MYFLT v2, MYFLT v3,
                      int ndx);

  /**
   * Sets five values in MYFLT array 'p' starting at index 'ndx' (counting
   * from zero). No error checking is done, 'p' and 'ndx' are assumed to
   * be valid.
   */
  void MYFLTArraySet5(MYFLT **p, MYFLT v0, MYFLT v1, MYFLT v2, MYFLT v3,
                      MYFLT v4, int ndx);

  /**
   * Sets six values in MYFLT array 'p' starting at index 'ndx' (counting
   * from zero). No error checking is done, 'p' and 'ndx' are assumed to
   * be valid.
   */
  void MYFLTArraySet6(MYFLT **p, MYFLT v0, MYFLT v1, MYFLT v2, MYFLT v3,
                      MYFLT v4, MYFLT v5, int ndx);

  /**
   * Sets seven values in MYFLT array 'p' starting at index 'ndx' (counting
   * from zero). No error checking is done, 'p' and 'ndx' are assumed to
   * be valid.
   */
  void MYFLTArraySet7(MYFLT **p, MYFLT v0, MYFLT v1, MYFLT v2, MYFLT v3,
                      MYFLT v4, MYFLT v5, MYFLT v6, int ndx);

  /**
   * Sets eight values in MYFLT array 'p' starting at index 'ndx' (counting
   * from zero). No error checking is done, 'p' and 'ndx' are assumed to
   * be valid.
   */
  void MYFLTArraySet8(MYFLT **p, MYFLT v0, MYFLT v1, MYFLT v2, MYFLT v3,
                      MYFLT v4, MYFLT v5, MYFLT v6, MYFLT v7, int ndx);

  /**
   * Sets nine values in MYFLT array 'p' starting at index 'ndx' (counting
   * from zero). No error checking is done, 'p' and 'ndx' are assumed to
   * be valid.
   */
  void MYFLTArraySet9(MYFLT **p, MYFLT v0, MYFLT v1, MYFLT v2, MYFLT v3,
                      MYFLT v4, MYFLT v5, MYFLT v6, MYFLT v7, MYFLT v8,
                      int ndx);

  /**
   * Sets ten values in MYFLT array 'p' starting at index 'ndx' (counting
   * from zero). No error checking is done, 'p' and 'ndx' are assumed to
   * be valid.
   */
  void MYFLTArraySet10(MYFLT **p, MYFLT v0, MYFLT v1, MYFLT v2, MYFLT v3,
                       MYFLT v4, MYFLT v5, MYFLT v6, MYFLT v7, MYFLT v8,
                       MYFLT v9, int ndx);

  /**
   * Retruns the value from MYFLT array 'p' at index 'ndx' (counting
   * from zero). No error checking is done, 'p' and 'ndx' are assumed
   * to be valid.
   */
  MYFLT MYFLTArrayGet(MYFLT **p, int ndx);

  /**
   * Stores a string in a MYFLT array (note: only do this with a pointer
   * returned by csoundGetChannelPtr()), optionally limiting the length to
   * maxLen - 1 characters. 'p' and 'value' are assumed to be valid.
   */
  void MYFLTArraySetString(MYFLT **p, const char *value, int maxLen);

  /**
   * Returns a string from a MYFLT array (note: only do this with a pointer
   * returned by csoundGetChannelPtr()). No error checking is done, 'p' is
   * assumed to be valid.
   */
  const char *MYFLTArrayGetString(MYFLT **p);

  /**
   * Releases a MYFLT pointer and array previously allocated by
   * CreateMYFLTArrayPtr().
   */
  void DestroyMYFLTArrayPtr(MYFLT **p);

  /**
   * Creates an opcode list pointer to be used by csoundNewOpcodeList().
   */
  opcodeListEntry **CreateOpcodeListPtr();

  /**
   * Returns opcode name from opcode list at index 'ndx' (counting from zero).
   * No error checking is done, 'p' and 'ndx' are assumed to be valid.
   */
  const char *OpcodeListGetName(opcodeListEntry **p, int ndx);

  /**
   * Returns output types from opcode list at index 'ndx' (counting from zero).
   * No error checking is done, 'p' and 'ndx' are assumed to be valid.
   */
  const char *OpcodeListGetOutputTypes(opcodeListEntry **p, int ndx);

  /**
   * Returns input types from opcode list at index 'ndx' (counting from zero).
   * No error checking is done, 'p' and 'ndx' are assumed to be valid.
   */
  const char *OpcodeListGetInputTypes(opcodeListEntry **p, int ndx);

  /**
   * Releases an opcode list pointer previously allocated by
   * CreateOpcodeListPtr().
   */
  void DestroyOpcodeListPtr(opcodeListEntry **p);

  /**
   * Creates a channel list pointer to be used by csoundListChannels().
   */
  CsoundChannelListEntry **CreateChannelListPtr();

  /**
   * Returns channel name from channel list at index 'ndx' (counting from
   * zero). No error checking is done, 'p' and 'ndx' are assumed to be valid.
   */
  const char *ChannelListGetName(CsoundChannelListEntry **p, int ndx);

  /**
   * Returns channel type from channel list at index 'ndx' (counting from
   * zero). No error checking is done, 'p' and 'ndx' are assumed to be valid.
   */
  int ChannelListGetType(CsoundChannelListEntry **p, int ndx);

  /**
   * Releases a channel list pointer previously allocated by
   * CreateChannelListPtr().
   */
  void DestroyChannelListPtr(CsoundChannelListEntry **p);

  /**
   * Creates an empty argument list for use with functions like
   * csoundCompile(). If there is not enough memory, NULL may be
   * returned.
   */
  char **CreateArgVList();

  /**
   * Inserts a new value to an argument list at the specified index,
   * and returns the new list. If there is not enough memory, NULL may
   * be returned.
   */
  char **ArgVListInsertValue(char **argv, const char *s, int ndx);

  /**
   * Appends a new value at the end of an argument list, and returns
   * the new list. If there is not enough memory, NULL may be returned.
   */
  char **ArgVListAppendValue(char **argv, const char *s);

  /**
   * Returns the count of arguments in the specified list.
   */
  int ArgVListGetCnt(char **argv);

  /**
   * Releases an argument list previously allocated by CreateArgVList().
   */
  void DestroyArgVList(char **argv);

  // internal functions

  void csoundMessageBufferCallback_1_(CSOUND *csound, int attr,
                                      const char *fmt, va_list args);

  void csoundMessageBufferCallback_2_(CSOUND *csound, int attr,
                                      const char *fmt, va_list args);

}       // extern "C"

class CsoundArgVList {
 private:
    char  **ArgV_;
 public:
    int argc()
    {
      return ArgVListGetCnt(ArgV_);
    }
    char **argv()
    {
      return ArgV_;
    }
    void Insert(const char *s, int ndx)
    {
      ArgV_ = ArgVListInsertValue(ArgV_, s, ndx);
    }
    void Append(const char *s)
    {
      ArgV_ = ArgVListAppendValue(ArgV_, s);
    }
    void Clear()
    {
      if (ArgV_) {
        char  **tmp = ArgV_;
        ArgV_ = (char**) 0;
        for (int i = 0; tmp[i]; i++)
          free((void*) tmp[i]);
        free((void*) tmp);
      }
      ArgV_ = (char**) malloc(sizeof(char*));
      if (ArgV_)
        ArgV_[0] = (char*) 0;
    }
    CsoundArgVList()
    {
      ArgV_ = (char**) malloc(sizeof(char*));
      if (ArgV_)
        ArgV_[0] = (char*) 0;
    }
    ~CsoundArgVList()
    {
      DestroyArgVList(ArgV_);
    }
};

#endif  // CSOUND_CS_GLUE_HPP

