/*
    cs_glue.cpp:

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

#include "csound.h"
#include "cs_glue.hpp"

struct csMsgStruct {
    struct csMsgStruct  *nxt;
    int         attr;
    char        s[1];
};

struct csMsgBuffer {
    csMsgStruct *firstMsg;
    csMsgStruct *lastMsg;
    int         msgCnt;
    char        *buf;
};

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

extern "C" void csoundEnableMessageBuffer(CSOUND *csound, int toStdOut)
{
    csMsgBuffer *pp;
    size_t      nBytes;

    csoundDestroyMessageBuffer(csound);
    nBytes = sizeof(csMsgBuffer);
    if (!toStdOut)
      nBytes += (size_t) 16384;
    pp = (csMsgBuffer*) malloc(nBytes);
    pp->firstMsg = (csMsgStruct*) 0;
    pp->lastMsg = (csMsgStruct*) 0;
    pp->msgCnt = 0;
    if (!toStdOut) {
      pp->buf = (char*) pp + (int) sizeof(csMsgBuffer);
      pp->buf[0] = (char) '\0';
    }
    else
      pp->buf = (char*) 0;
    csoundSetHostData(csound, (void*) pp);
    if (!toStdOut)
      csoundSetMessageCallback(csound, csoundMessageBufferCallback_1_);
    else
      csoundSetMessageCallback(csound, csoundMessageBufferCallback_2_);
}

/**
 * Returns the first message from the buffer.
 */

extern "C" const char *csoundGetFirstMessage(CSOUND *csound)
{
    csMsgBuffer *pp = (csMsgBuffer*) csoundGetHostData(csound);
    if (pp && pp->firstMsg)
      return &(pp->firstMsg->s[0]);
    else
      return (char*) 0;
}

/**
 * Returns the attribute parameter (see msg_attr.h) of the first message
 * in the buffer.
 */

extern "C" int csoundGetFirstMessageAttr(CSOUND *csound)
{
    csMsgBuffer *pp = (csMsgBuffer*) csoundGetHostData(csound);
    if (pp && pp->firstMsg)
      return pp->firstMsg->attr;
    else
      return 0;
}

/**
 * Removes the first message from the buffer.
 */

extern "C" void csoundPopFirstMessage(CSOUND *csound)
{
    csMsgBuffer *pp = (csMsgBuffer*) csoundGetHostData(csound);
    if (pp && pp->firstMsg) {
      csMsgStruct *tmp = pp->firstMsg;
      pp->firstMsg = tmp->nxt;
      pp->msgCnt--;
      if (!pp->firstMsg)
        pp->lastMsg = (csMsgStruct*) 0;
      free((void*) tmp);
    }
}

/**
 * Returns the number of pending messages in the buffer.
 */

extern "C" int csoundGetMessageCnt(CSOUND *csound)
{
    csMsgBuffer *pp = (csMsgBuffer*) csoundGetHostData(csound);
    if (!pp)
      return 0;
    return pp->msgCnt;
}

/**
 * Releases all memory used by the message buffer.
 */

extern "C" void csoundDestroyMessageBuffer(CSOUND *csound)
{
    csMsgBuffer *pp = (csMsgBuffer*) csoundGetHostData(csound);
    if (!pp)
      return;
    while (pp->firstMsg) {
      csMsgStruct *tmp = pp->firstMsg;
      pp->firstMsg = tmp->nxt;
      pp->msgCnt--;
      if (!pp->firstMsg)
        pp->lastMsg = (csMsgStruct*) 0;
      free((void*) tmp);
    }
    csoundSetHostData(csound, (void*) 0);
    free((void*) pp);
}

extern "C" void csoundMessageBufferCallback_1_(CSOUND *csound, int attr,
                                               const char *fmt, va_list args)
{
    csMsgBuffer *pp = (csMsgBuffer*) csoundGetHostData(csound);
    csMsgStruct *p;
    int         len;

    len = vsprintf(pp->buf, fmt, args);         // FIXME: this can overflow
    if ((unsigned int) len >= (unsigned int) 16384) {
      fprintf(stderr, "csound: internal error: message buffer overflow\n");
      exit(-1);
    }
    p = (csMsgStruct*) malloc(sizeof(csMsgStruct) + (size_t) len);
    p->nxt = (csMsgStruct*) 0;
    p->attr = attr;
    strcpy(&(p->s[0]), pp->buf);
    if (pp->firstMsg == (csMsgStruct*) 0)
      pp->firstMsg = p;
    else
      pp->lastMsg->nxt = p;
    pp->lastMsg = p;
    pp->msgCnt++;
}

extern "C" void csoundMessageBufferCallback_2_(CSOUND *csound, int attr,
                                               const char *fmt, va_list args)
{
    csMsgBuffer *pp = (csMsgBuffer*) csoundGetHostData(csound);
    csMsgStruct *p;
    int         len = 0;

    switch (attr & CSOUNDMSG_TYPE_MASK) {
      case CSOUNDMSG_ERROR:
      case CSOUNDMSG_REALTIME:
      case CSOUNDMSG_WARNING:
        len = vfprintf(stderr, fmt, args);
        break;
      default:
        len = vfprintf(stdout, fmt, args);
    }
    p = (csMsgStruct*) malloc(sizeof(csMsgStruct) + (size_t) len);
    p->nxt = (csMsgStruct*) 0;
    p->attr = attr;
    vsprintf(&(p->s[0]), fmt, args);
    if (pp->firstMsg == (csMsgStruct*) 0)
      pp->firstMsg = p;
    else
      pp->lastMsg->nxt = p;
    pp->lastMsg = p;
    pp->msgCnt++;
}

/**
 * Creates a pointer to an array of MYFLT values, optionally allocating
 * space for 'cnt' elements if 'cnt' is greater than zero. For use with
 * csoundGetChannelPtr(), 'cnt' should be zero.
 */

extern "C" MYFLT **CreateMYFLTArray(int cnt)
{
    void    *pp;
    MYFLT   **p;
    size_t  offs, nBytes = 0;

    offs = sizeof(MYFLT*);
    if (cnt > 0) {
      offs = offs + (sizeof(MYFLT) - (size_t) 1);
      offs = (offs / sizeof(MYFLT)) * sizeof(MYFLT);
      nBytes = (size_t) cnt * sizeof(MYFLT);
    }
    nBytes += offs;
    pp = (void*) malloc(nBytes);
    p = (MYFLT**) pp;
    if (p) {
      if (cnt > 0) {
        (*p) = (MYFLT*) ((char*) pp + (int) offs);
        for (int i = 0; i < cnt; i++)
          (*p)[i] = (MYFLT) 0;
      }
      else
        (*p) = (MYFLT*) 0;
    }
    return p;
}

/**
 * Returns non-zero if 'p' is a valid MYFLT array.
 */

extern "C" int MYFLTArrayIsValid(MYFLT **p)
{
    if (!p || !(*p))
      return 0;
    return 1;
}

/**
 * Returns a pointer to the element of MYFLT array 'p' at index 'ndx'
 * (counting from zero). 'p' and 'ndx' are assumed to be valid.
 */

extern "C" MYFLT *MYFLTArrayGetPtr(MYFLT **p, int ndx)
{
    return &((*p)[ndx]);
}

/**
 * Sets a value in MYFLT array 'p' at index 'ndx' (counting from zero).
 * No error checking is done, 'p' and 'ndx' are assumed to be valid.
 */

extern "C" void MYFLTArraySet(MYFLT **p, MYFLT value, int ndx)
{
    (*p)[ndx] = value;
}

/**
 * Sets two values in MYFLT array 'p' starting at index 'ndx' (counting
 * from zero). No error checking is done, 'p' and 'ndx' are assumed to
 * be valid.
 */

extern "C" void MYFLTArraySet2(MYFLT **p, MYFLT v0, MYFLT v1, int ndx)
{
    (*p)[ndx] = v0;
    (*p)[ndx + 1] = v1;
}

/**
 * Sets three values in MYFLT array 'p' starting at index 'ndx' (counting
 * from zero). No error checking is done, 'p' and 'ndx' are assumed to
 * be valid.
 */

extern "C" void MYFLTArraySet3(MYFLT **p, MYFLT v0, MYFLT v1, MYFLT v2,
                               int ndx)
{
    (*p)[ndx] = v0;
    (*p)[ndx + 1] = v1;
    (*p)[ndx + 2] = v2;
}

/**
 * Sets four values in MYFLT array 'p' starting at index 'ndx' (counting
 * from zero). No error checking is done, 'p' and 'ndx' are assumed to
 * be valid.
 */

extern "C" void MYFLTArraySet4(MYFLT **p, MYFLT v0, MYFLT v1, MYFLT v2,
                               MYFLT v3, int ndx)
{
    (*p)[ndx] = v0;
    (*p)[ndx + 1] = v1;
    (*p)[ndx + 2] = v2;
    (*p)[ndx + 3] = v3;
}

/**
 * Sets five values in MYFLT array 'p' starting at index 'ndx' (counting
 * from zero). No error checking is done, 'p' and 'ndx' are assumed to
 * be valid.
 */

extern "C" void MYFLTArraySet5(MYFLT **p, MYFLT v0, MYFLT v1, MYFLT v2,
                               MYFLT v3, MYFLT v4, int ndx)
{
    (*p)[ndx] = v0;
    (*p)[ndx + 1] = v1;
    (*p)[ndx + 2] = v2;
    (*p)[ndx + 3] = v3;
    (*p)[ndx + 4] = v4;
}

/**
 * Sets six values in MYFLT array 'p' starting at index 'ndx' (counting
 * from zero). No error checking is done, 'p' and 'ndx' are assumed to
 * be valid.
 */

extern "C" void MYFLTArraySet6(MYFLT **p, MYFLT v0, MYFLT v1, MYFLT v2,
                               MYFLT v3, MYFLT v4, MYFLT v5, int ndx)
{
    (*p)[ndx] = v0;
    (*p)[ndx + 1] = v1;
    (*p)[ndx + 2] = v2;
    (*p)[ndx + 3] = v3;
    (*p)[ndx + 4] = v4;
    (*p)[ndx + 5] = v5;
}

/**
 * Sets seven values in MYFLT array 'p' starting at index 'ndx' (counting
 * from zero). No error checking is done, 'p' and 'ndx' are assumed to
 * be valid.
 */

extern "C" void MYFLTArraySet7(MYFLT **p, MYFLT v0, MYFLT v1, MYFLT v2,
                               MYFLT v3, MYFLT v4, MYFLT v5, MYFLT v6,
                               int ndx)
{
    (*p)[ndx] = v0;
    (*p)[ndx + 1] = v1;
    (*p)[ndx + 2] = v2;
    (*p)[ndx + 3] = v3;
    (*p)[ndx + 4] = v4;
    (*p)[ndx + 5] = v5;
    (*p)[ndx + 6] = v6;
}

/**
 * Sets eight values in MYFLT array 'p' starting at index 'ndx' (counting
 * from zero). No error checking is done, 'p' and 'ndx' are assumed to
 * be valid.
 */

extern "C" void MYFLTArraySet8(MYFLT **p, MYFLT v0, MYFLT v1, MYFLT v2,
                               MYFLT v3, MYFLT v4, MYFLT v5, MYFLT v6,
                               MYFLT v7, int ndx)
{
    (*p)[ndx] = v0;
    (*p)[ndx + 1] = v1;
    (*p)[ndx + 2] = v2;
    (*p)[ndx + 3] = v3;
    (*p)[ndx + 4] = v4;
    (*p)[ndx + 5] = v5;
    (*p)[ndx + 6] = v6;
    (*p)[ndx + 7] = v7;
}

/**
 * Sets nine values in MYFLT array 'p' starting at index 'ndx' (counting
 * from zero). No error checking is done, 'p' and 'ndx' are assumed to
 * be valid.
 */

extern "C" void MYFLTArraySet9(MYFLT **p, MYFLT v0, MYFLT v1, MYFLT v2,
                               MYFLT v3, MYFLT v4, MYFLT v5, MYFLT v6,
                               MYFLT v7, MYFLT v8, int ndx)
{
    (*p)[ndx] = v0;
    (*p)[ndx + 1] = v1;
    (*p)[ndx + 2] = v2;
    (*p)[ndx + 3] = v3;
    (*p)[ndx + 4] = v4;
    (*p)[ndx + 5] = v5;
    (*p)[ndx + 6] = v6;
    (*p)[ndx + 7] = v7;
    (*p)[ndx + 8] = v8;
}

/**
 * Sets ten values in MYFLT array 'p' starting at index 'ndx' (counting
 * from zero). No error checking is done, 'p' and 'ndx' are assumed to
 * be valid.
 */

extern "C" void MYFLTArraySet10(MYFLT **p, MYFLT v0, MYFLT v1, MYFLT v2,
                                MYFLT v3, MYFLT v4, MYFLT v5, MYFLT v6,
                                MYFLT v7, MYFLT v8, MYFLT v9, int ndx)
{
    (*p)[ndx] = v0;
    (*p)[ndx + 1] = v1;
    (*p)[ndx + 2] = v2;
    (*p)[ndx + 3] = v3;
    (*p)[ndx + 4] = v4;
    (*p)[ndx + 5] = v5;
    (*p)[ndx + 6] = v6;
    (*p)[ndx + 7] = v7;
    (*p)[ndx + 8] = v8;
    (*p)[ndx + 9] = v9;
}

/**
 * Retruns the value from MYFLT array 'p' at index 'ndx' (counting
 * from zero). No error checking is done, 'p' and 'ndx' are assumed
 * to be valid.
 */

extern "C" MYFLT MYFLTArrayGet(MYFLT **p, int ndx)
{
    return (*p)[ndx];
}

/**
 * Stores a string in a MYFLT array (note: only do this with a pointer
 * returned by csoundGetChannelPtr()), optionally limiting the length to
 * maxLen - 1 characters. 'p' and 'value' are assumed to be valid.
 */

extern "C" void MYFLTArraySetString(MYFLT **p, const char *value, int maxLen)
{
    if (maxLen <= 0 || strlen(value) < (size_t) maxLen)
      strcpy((char*) (*p), value);
    else {
      strncpy((char*) (*p), value, (size_t) (maxLen - 1));
      ((char*) (*p))[maxLen - 1] = (char) '\0';
    }
}

/**
 * Returns a string from a MYFLT array (note: only do this with a pointer
 * returned by csoundGetChannelPtr()). No error checking is done, 'p' is
 * assumed to be valid.
 */

extern "C" const char *MYFLTArrayGetString(MYFLT **p)
{
    return (char*) (*p);
}

/**
 * Releases a MYFLT pointer and array previously allocated by
 * CreateMYFLTArray().
 */

extern "C" void DestroyMYFLTArray(MYFLT **p)
{
    if (p)
      free((void*) p);
}

/**
 * Creates an opcode list pointer to be used by csoundNewOpcodeList().
 */

extern "C" opcodeListEntry **CreateOpcodeListPtr()
{
    opcodeListEntry **p;
    p = (opcodeListEntry**) malloc(sizeof(opcodeListEntry*));
    if (p)
      (*p) = (opcodeListEntry*) 0;
    return p;
}

/**
 * Returns opcode name from opcode list at index 'ndx' (counting from zero).
 * No error checking is done, 'p' and 'ndx' are assumed to be valid.
 */

extern "C" const char *OpcodeListGetName(opcodeListEntry **p, int ndx)
{
    return (*p)[ndx].opname;
}

/**
 * Returns output types from opcode list at index 'ndx' (counting from zero).
 * No error checking is done, 'p' and 'ndx' are assumed to be valid.
 */

extern "C" const char *OpcodeListGetOutputTypes(opcodeListEntry **p, int ndx)
{
    return (*p)[ndx].outypes;
}

/**
 * Returns input types from opcode list at index 'ndx' (counting from zero).
 * No error checking is done, 'p' and 'ndx' are assumed to be valid.
 */

extern "C" const char *OpcodeListGetInputTypes(opcodeListEntry **p, int ndx)
{
    return (*p)[ndx].intypes;
}

/**
 * Releases an opcode list pointer previously allocated by
 * CreateOpcodeListPtr().
 */

extern "C" void DestroyOpcodeListPtr(opcodeListEntry **p)
{
    if (p)
      free((void*) p);
}

/**
 * Creates a channel list pointer to be used by csoundListChannels().
 */

extern "C" CsoundChannelListEntry **CreateChannelListPtr()
{
    CsoundChannelListEntry  **p;
    p = (CsoundChannelListEntry**) malloc(sizeof(CsoundChannelListEntry*));
    if (p)
      (*p) = (CsoundChannelListEntry*) 0;
    return p;
}

/**
 * Returns channel name from channel list at index 'ndx' (counting from
 * zero). No error checking is done, 'p' and 'ndx' are assumed to be valid.
 */

extern "C" const char *ChannelListGetName(CsoundChannelListEntry **p, int ndx)
{
    return (*p)[ndx].name;
}

/**
 * Returns channel type from channel list at index 'ndx' (counting from
 * zero). No error checking is done, 'p' and 'ndx' are assumed to be valid.
 */

extern "C" int ChannelListGetType(CsoundChannelListEntry **p, int ndx)
{
    return (*p)[ndx].type;
}

/**
 * Releases a channel list pointer previously allocated by
 * CreateChannelListPtr().
 */

extern "C" void DestroyChannelListPtr(CsoundChannelListEntry **p)
{
    if (p)
      free((void*) p);
}

/**
 * Creates an empty argument list for use with functions like
 * csoundCompile(). If there is not enough memory, NULL may be
 * returned.
 */

extern "C" char **CreateArgVList()
{
    char  **argv;
    argv = (char**) malloc(sizeof(char*));
    if (argv)
      (*argv) = (char*) 0;
    return argv;
}

/**
 * Inserts a new value to an argument list at the specified index,
 * and returns the new list. If there is not enough memory, NULL may
 * be returned.
 */

extern "C" char **ArgVListInsertValue(char **argv, const char *s, int ndx)
{
    char  **new_argv;
    int   argc = 0;
    int   i;

    if (!s)
      return argv;
    if (argv) {
      for ( ; argv[argc] != (char*) 0; argc++)
        ;
    }
    if (ndx > argc)
      ndx = argc;
    if (ndx < 0)
      ndx = 0;
    new_argv = (char**) malloc(sizeof(char*) * (size_t) (argc + 2));
    if (!new_argv) {
      free((void*) argv);
      return (char**) 0;
    }
    for (i = 0; i < ndx; i++)
      new_argv[i] = argv[i];
    new_argv[i] = (char*) malloc(strlen(s) + (size_t) 1);
    if (new_argv[i] == (char*) 0) {
      free((void*) new_argv);
      free((void*) argv);
      return (char**) 0;
    }
    strcpy(new_argv[i], s);
    while (++i <= argc) {
      new_argv[i] = argv[i - 1];
    }
    new_argv[i] = (char*) 0;
    free((void*) argv);
    return new_argv;
}

/**
 * Appends a new value at the end of an argument list, and returns
 * the new list. If there is not enough memory, NULL may be returned.
 */

extern "C" char **ArgVListAppendValue(char **argv, const char *s)
{
    return ArgVListInsertValue(argv, s, 0x7FFFFFFF);
}

/**
 * Returns the count of arguments in the specified list.
 */

extern "C" int ArgVListGetCnt(char **argv)
{
    int   argc = 0;

    if (argv) {
      for ( ; argv[argc] != (char*) 0; argc++)
        ;
    }
    return argc;
}

/**
 * Releases an argument list previously allocated by CreateArgVList().
 */

extern "C" void DestroyArgVList(char **argv)
{
    if (!argv)
      return;
    for (int i = 0; argv[i] != (char*) 0; i++)
      free((void*) argv[i]);
    free((void*) argv);
}

