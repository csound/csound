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
#include "cs_glue.h"
#include "csound.hpp"
#include "cs_glue.hpp"

extern "C" {

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

  // callback for storing messages in the buffer only
  static void csoundMessageBufferCallback_1_(CSOUND *csound, int attr,
                                             const char *fmt, va_list args);

  // callback for writing messages to the buffer, and also stdout/stderr
  static void csoundMessageBufferCallback_2_(CSOUND *csound, int attr,
                                             const char *fmt, va_list args);

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

  void csoundEnableMessageBuffer(CSOUND *csound, int toStdOut)
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

  const char *csoundGetFirstMessage(CSOUND *csound)
  {
    csMsgBuffer *pp = (csMsgBuffer*) csoundGetHostData(csound);
    if (pp && pp->firstMsg)
      return &(pp->firstMsg->s[0]);
    return (char*) 0;
  }

  /**
   * Returns the attribute parameter (see msg_attr.h) of the first message
   * in the buffer.
   */

  int csoundGetFirstMessageAttr(CSOUND *csound)
  {
    csMsgBuffer *pp = (csMsgBuffer*) csoundGetHostData(csound);
    if (pp && pp->firstMsg)
      return pp->firstMsg->attr;
    return 0;
  }

  /**
   * Removes the first message from the buffer.
   */

  void csoundPopFirstMessage(CSOUND *csound)
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

  int csoundGetMessageCnt(CSOUND *csound)
  {
    csMsgBuffer *pp = (csMsgBuffer*) csoundGetHostData(csound);
    if (pp)
      return pp->msgCnt;
    return 0;
  }

  /**
   * Releases all memory used by the message buffer.
   */

  void csoundDestroyMessageBuffer(CSOUND *csound)
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

  static void csoundMessageBufferCallback_1_(CSOUND *csound, int attr,
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

  static void csoundMessageBufferCallback_2_(CSOUND *csound, int attr,
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

}       // extern "C"

// ----------------------------------------------------------------------------

/**
 * CsoundOpcodeList(CSOUND *)
 * CsoundOpcodeList(Csound *)
 *
 * Creates an alphabetically sorted opcode list for a Csound instance.
 * Should be called after csoundCompile() or Csound::Compile().
 */

/**
 * Returns the number of opcodes, or -1 if there is no list.
 */

int CsoundOpcodeList::Count()
{
    return cnt;
}

/**
 * Returns the name of the opcode at index 'ndx' (counting from zero),
 * or NULL if the index is out of range.
 */

const char * CsoundOpcodeList::Name(int ndx)
{
    if (lst && (unsigned int) ndx < (unsigned int) cnt)
      return lst[ndx].opname;
    return (char*) 0;
}

/**
 * Returns the output types of the opcode at index 'ndx' (counting from
 * zero), or NULL if the index is out of range.
 */

const char * CsoundOpcodeList::OutTypes(int ndx)
{
    if (lst && (unsigned int) ndx < (unsigned int) cnt)
      return lst[ndx].outypes;
    return (char*) 0;
}

/**
 * Returns the input types of the opcode at index 'ndx' (counting from
 * zero), or NULL if the index is out of range.
 */

const char * CsoundOpcodeList::InTypes(int ndx)
{
    if (lst && (unsigned int) ndx < (unsigned int) cnt)
      return lst[ndx].intypes;
    return (char*) 0;
}

/**
 * Releases the memory used by the opcode list. Should be called
 * before the Csound instance is destroyed or reset.
 */

void CsoundOpcodeList::Clear()
{
    // FIXME: this depends on csoundDisposeOpcodeList() ignoring the
    // instance pointer
    if (lst)
      csoundDisposeOpcodeList((CSOUND*) 0, lst);
    lst = (opcodeListEntry*) 0;
    cnt = -1;
}

CsoundOpcodeList::CsoundOpcodeList(CSOUND *csound)
{
    lst = (opcodeListEntry*) 0;
    cnt = csoundNewOpcodeList(csound, &lst);
    if (cnt < 0 || !lst) {
      lst = (opcodeListEntry*) 0;
      cnt = -1;
    }
}

CsoundOpcodeList::CsoundOpcodeList(Csound *csound)
{
    lst = (opcodeListEntry*) 0;
    cnt = csound->NewOpcodeList(lst);
    if (cnt < 0 || !lst) {
      lst = (opcodeListEntry*) 0;
      cnt = -1;
    }
}

CsoundOpcodeList::~CsoundOpcodeList()
{
    this->Clear();
}

// ----------------------------------------------------------------------------

/**
 * CsoundChannelList(CSOUND *)
 * CsoundChannelList(Csound *)
 *
 * Creates an alphabetically sorted list of named channels of a Csound
 * instance. Should be called after csoundCompile() or Csound::Compile().
 */

void CsoundChannelList::ResetVariables()
{
    lst = (CsoundChannelListEntry*) 0;
    cnt = -1;
    csound = (CSOUND*) 0;
}

int CsoundChannelList::GetChannelMetaData(int ndx,
                                          MYFLT &dflt, MYFLT &min, MYFLT &max)
{
    const char  *name;
    if (!lst || (unsigned int) ndx >= (unsigned int) cnt)
      return -1;
    name = lst[ndx].name;
    return csoundGetControlChannelParams(csound, name, &dflt, &min, &max);
}

/**
 * Returns the number of channels (-1 if there is no list).
 */

int CsoundChannelList::Count()
{
    return cnt;
}

/**
 * Returns the name of the channel at index 'ndx' (counting from zero),
 * or NULL if the index is out of range.
 */

const char * CsoundChannelList::Name(int ndx)
{
    if (lst && (unsigned int) ndx < (unsigned int) cnt)
      return lst[ndx].name;
    return (char*) 0;
}

/**
 * Returns the type of the channel at index 'ndx' (counting from zero),
 * or -1 if the index is out of range.
 */

int CsoundChannelList::Type(int ndx)
{
    if (lst && (unsigned int) ndx < (unsigned int) cnt)
      return lst[ndx].type;
    return -1;
}

/**
 * Returns 1 if the channel at index 'ndx' (counting from zero) exists
 * and is a control channel, and 0 otherwise.
 */

int CsoundChannelList::IsControlChannel(int ndx)
{
    if (lst && (unsigned int) ndx < (unsigned int) cnt)
      return ((lst[ndx].type & CSOUND_CHANNEL_TYPE_MASK)
                == CSOUND_CONTROL_CHANNEL ? 1 : 0);
    return 0;
}

/**
 * Returns 1 if the channel at index 'ndx' (counting from zero) exists
 * and is an audio channel, and 0 otherwise.
 */

int CsoundChannelList::IsAudioChannel(int ndx)
{
    if (lst && (unsigned int) ndx < (unsigned int) cnt)
      return ((lst[ndx].type & CSOUND_CHANNEL_TYPE_MASK)
                == CSOUND_AUDIO_CHANNEL ? 1 : 0);
    return 0;
}

/**
 * Returns 1 if the channel at index 'ndx' (counting from zero) exists
 * and is a string channel, and 0 otherwise.
 */

int CsoundChannelList::IsStringChannel(int ndx)
{
    if (lst && (unsigned int) ndx < (unsigned int) cnt)
      return ((lst[ndx].type & CSOUND_CHANNEL_TYPE_MASK)
                == CSOUND_STRING_CHANNEL ? 1 : 0);
    return 0;
}

/**
 * Returns 1 if the channel at index 'ndx' (counting from zero) exists
 * and the input bit is set, and 0 otherwise.
 */

int CsoundChannelList::IsInputChannel(int ndx)
{
    if (lst && (unsigned int) ndx < (unsigned int) cnt)
      return ((lst[ndx].type & CSOUND_INPUT_CHANNEL) ? 1 : 0);
    return 0;
}

/**
 * Returns 1 if the channel at index 'ndx' (counting from zero) exists
 * and the output bit is set, and 0 otherwise.
 */

int CsoundChannelList::IsOutputChannel(int ndx)
{
    if (lst && (unsigned int) ndx < (unsigned int) cnt)
      return ((lst[ndx].type & CSOUND_OUTPUT_CHANNEL) ? 1 : 0);
    return 0;
}

/**
 * Returns the sub-type (0: normal, 1: integer, 2: linear, 3: exponential)
 * of the control channel at index 'ndx' (counting from zero), or -1 if
 * the channel does not exist or is not a control channel.
 */

int CsoundChannelList::SubType(int ndx)
{
    MYFLT dflt, min, max;
    int   tmp;
    tmp = this->GetChannelMetaData(ndx, dflt, min, max);
    return (tmp >= 0 ? tmp : -1);
}

/**
 * Returns the default value set for the control channel at index 'ndx'
 * (counting from zero), or 0.0 if the channel does not exist, is not a
 * control channel, or has no default value.
 */

double CsoundChannelList::DefaultValue(int ndx)
{
    MYFLT dflt, min, max;
    if (this->GetChannelMetaData(ndx, dflt, min, max) > 0)
      return dflt;
    return 0.0;
}

/**
 * Returns the minimum value set for the control channel at index 'ndx'
 * (counting from zero), or 0.0 if the channel does not exist, is not a
 * control channel, or has no minimum value.
 */

double CsoundChannelList::MinValue(int ndx)
{
    MYFLT dflt, min, max;
    if (this->GetChannelMetaData(ndx, dflt, min, max) > 0)
      return min;
    return 0.0;
}

/**
 * Returns the maximum value set for the control channel at index 'ndx'
 * (counting from zero), or 0.0 if the channel does not exist, is not a
 * control channel, or has no maximum value.
 */

double CsoundChannelList::MaxValue(int ndx)
{
    MYFLT dflt, min, max;
    if (this->GetChannelMetaData(ndx, dflt, min, max) > 0)
      return max;
    return 0.0;
}

/**
 * Releases the memory used by the channel list. Should be called
 * before the Csound instance is destroyed or reset.
 */

void CsoundChannelList::Clear()
{
    // FIXME: this depends on csoundDeleteChannelList() ignoring the
    // instance pointer
    if (lst)
      csoundDeleteChannelList((CSOUND*) 0, lst);
    this->ResetVariables();
}

CsoundChannelList::CsoundChannelList(CSOUND *csound)
{
    lst = (CsoundChannelListEntry*) 0;
    cnt = csoundListChannels(csound, &lst);
    this->csound = csound;
    if (cnt < 0 || !lst)
      this->ResetVariables();
}

CsoundChannelList::CsoundChannelList(Csound *csound)
{
    lst = (CsoundChannelListEntry*) 0;
    cnt = csound->ListChannels(lst);
    this->csound = csound->GetCsound();
    if (cnt < 0 || !lst)
      this->ResetVariables();
}

CsoundChannelList::~CsoundChannelList()
{
    this->Clear();
}

// ----------------------------------------------------------------------------

/**
 * CsoundUtilityList(CSOUND *)
 * CsoundUtilityList(Csound *)
 *
 * Creates an alphabetically sorted list of utilities registered
 * for a Csound instance. Should be called after csoundPreCompile()
 * or Csound::PreCompile().
 */

/**
 * Returns the number of utilities, or -1 if there is no list.
 */

int CsoundUtilityList::Count()
{
    return cnt;
}

/**
 * Returns the name of the utility at index 'ndx' (counting from zero),
 * or NULL if the index is out of range.
 */

const char * CsoundUtilityList::Name(int ndx)
{
    if (lst && (unsigned int) ndx < (unsigned int) cnt)
      return lst[ndx];
    return (char*) 0;
}

/**
 * Releases the memory used by the utility list. Should be called
 * before the Csound instance is destroyed or reset.
 */

void CsoundUtilityList::Clear()
{
    // FIXME: this depends on csoundDeleteUtilityList() ignoring the
    // instance pointer
    if (lst)
      csoundDeleteUtilityList((CSOUND*) 0, lst);
    lst = (char**) 0;
    cnt = -1;
}

CsoundUtilityList::CsoundUtilityList(CSOUND *csound)
{
    int n = -1;
    lst = csoundListUtilities(csound);
    if (lst) {
      while (lst[++n])
        ;
    }
    cnt = n;
}

CsoundUtilityList::CsoundUtilityList(Csound *csound)
{
    int n = -1;
    lst = csound->ListUtilities();
    if (lst) {
      while (lst[++n])
        ;
    }
    cnt = n;
}

CsoundUtilityList::~CsoundUtilityList()
{
    this->Clear();
}

// ----------------------------------------------------------------------------

/**
 * CsoundMYFLTArray()
 *
 * Creates a pointer for use with csoundGetChannelPtr(),
 * csoundGetOutputBuffer(), or other functions that return a
 * pointer to an array of floating point values.
 *
 * CsoundMYFLTArray(int cnt)
 *
 * Allocates an array of 'cnt' floating point values, for use
 * with Csound API functions that take a MYFLT* pointer.
 */

/**
 * Stores a string in the array (note: only do this with a pointer
 * returned by csoundGetChannelPtr()), optionally limiting the length to
 * maxLen - 1 characters.
 */

void CsoundMYFLTArray::SetStringValue(const char *s, int maxLen)
{
    if (p) {
      int i = 0;
      if (s && --maxLen > 0) {
        while (s[i]) {
          ((char*) p)[i] = s[i];
          if (++i >= maxLen)
            break;
        }
      }
      ((char*) p)[i] = (char) '\0';
    }
}

/**
 * Returns a string from the array (note: only do this with a pointer
 * returned by csoundGetChannelPtr()), or NULL if there is no array.
 */

const char * CsoundMYFLTArray::GetStringValue()
{
    return (char*) p;
}

/**
 * Clears the array pointer, and releases any memory that was allocated
 * by calling the constructor with a positive number of elements.
 */

void CsoundMYFLTArray::Clear()
{
    if (pp)
      free(pp);
    p = (MYFLT*) 0;
    pp = (void*) 0;
}

CsoundMYFLTArray::CsoundMYFLTArray()
{
    p = (MYFLT*) 0;
    pp = (void*) 0;
}

CsoundMYFLTArray::CsoundMYFLTArray(int n)
{
    p = (MYFLT*) 0;
    pp = (void*) 0;
    if (n > 0)
      pp = (void*) malloc((size_t) n * sizeof(MYFLT));
    if (pp) {
      p = (MYFLT*) pp;
      for (int i = 0; i < n; i++)
        p[i] = (MYFLT) 0;
    }
}

CsoundMYFLTArray::~CsoundMYFLTArray()
{
    this->Clear();
}

// ----------------------------------------------------------------------------

/**
 * A simple class for creating argv[] lists for use with
 * functions like csoundCompile().
 */

void CsoundArgVList::destroy_argv()
{
    for (int i = 0; i < cnt; i++)
      free((void*) ArgV_[i]);
    if (ArgV_)
      free((void*) ArgV_);
    ArgV_ = (char**) 0;
    cnt = -1;
}

/**
 * Returns the count of arguments in the list, zero if there are
 * none, and -1 if the list could not be allocated.
 */

int CsoundArgVList::argc()
{
    return cnt;
}

/**
 * Returns a char** pointer for use with csoundCompile() etc.
 */

char ** CsoundArgVList::argv()
{
    return ArgV_;
}

/**
 * Returns the argument at the specified index (counting from zero),
 * or NULL if the index is out of range.
 */

const char * CsoundArgVList::argv(int ndx)
{
    if (ArgV_ && (unsigned int) ndx < (unsigned int) cnt)
      return ArgV_[ndx];
    return (char*) 0;
}

/**
 * Inserts a new value to the argument list at the specified index
 * (counting from zero). If there is not enough memory, the list is
 * not changed.
 */

void CsoundArgVList::Insert(int ndx, const char *s)
{
    char  **new_argv;
    int   new_cnt, i;

    if (!s)
      return;
    if (ndx > cnt)
      ndx = cnt;
    if (ndx < 0)
      ndx = 0;
    new_cnt = (cnt >= 0 ? cnt + 1 : 1);
    new_argv = (char**) malloc((size_t) (new_cnt + 1) * sizeof(char*));
    if (!new_argv)
      return;
    for (i = 0; i < ndx; i++)
      new_argv[i] = ArgV_[i];
    new_argv[i] = (char*) malloc(strlen(s) + (size_t) 1);
    if (!new_argv[i]) {
      free((void*) new_argv);
      return;
    }
    strcpy(new_argv[i], s);
    while (++i < new_cnt)
      new_argv[i] = ArgV_[i - 1];
    if (ArgV_)
      free((void*) ArgV_);
    ArgV_ = new_argv;
    cnt = new_cnt;
}

/**
 * Appends a new value at the end of the argument list.
 * If there is not enough memory, the list is not changed.
 */

void CsoundArgVList::Append(const char *s)
{
    this->Insert(0x7FFFFFFF, s);
}

/**
 * Removes all elements of the list.
 */

void CsoundArgVList::Clear()
{
    this->destroy_argv();
    ArgV_ = (char**) malloc(sizeof(char*));
    if (ArgV_) {
      ArgV_[0] = (char*) 0;
      cnt = 0;
    }
    else
      cnt = -1;
}

CsoundArgVList::CsoundArgVList()
{
    cnt = -1;
    ArgV_ = (char**) malloc(sizeof(char*));
    if (ArgV_) {
      ArgV_[0] = (char*) 0;
      cnt = 0;
    }
}

CsoundArgVList::~CsoundArgVList()
{
    this->destroy_argv();
}

// ----------------------------------------------------------------------------

/**
 * Experimental class for wrapping callbacks using SWIG directors.
 */

extern "C" {

  static void MessageCallback_wrapper(CSOUND *csound,
                                      int attr, const char *fmt, va_list args)
  {
    CsoundCallbackWrapper *p;
    p = (CsoundCallbackWrapper*) csoundGetHostData(csound);
#if defined(HAVE_C99) && !defined(WIN32)
    {
      char  buf[2048];
      int   n;
      n = vsnprintf(&(buf[0]), (size_t) 2048, fmt, args);
      if (n < 0) {      // for compatibility only
        fprintf(stderr, " *** buffer overflow in message callback\n");
        exit(-1);
      }
      else if (n >= 2048) {
        char  *bufp = (char*) malloc((size_t) n + (size_t) 1);
        if (bufp) {
          vsprintf(bufp, fmt, args);
          p->MessageCallback(attr, bufp);
          free((void*) bufp);
        }
      }
      else
        p->MessageCallback(attr, &(buf[0]));
    }
#else
    {
      char  buf[16384];
      if (vsprintf(&(buf[0]), fmt, args) >= 16384) {
        fprintf(stderr, " *** buffer overflow in message callback\n");
        exit(-1);
      }
      p->MessageCallback(attr, &(buf[0]));
    }
#endif
  }

  static void InputValueCallback_wrapper(CSOUND *csound,
                                         const char *chnName, MYFLT *value)
  {
    CsoundCallbackWrapper *p;
    p = (CsoundCallbackWrapper*) csoundGetHostData(csound);
    *value = (MYFLT) p->InputValueCallback(chnName);
  }

  static void OutputValueCallback_wrapper(CSOUND *csound,
                                          const char *chnName, MYFLT value)
  {
    CsoundCallbackWrapper *p;
    p = (CsoundCallbackWrapper*) csoundGetHostData(csound);
    p->OutputValueCallback(chnName, (double) value);
  }

  static int YieldCallback_wrapper(CSOUND *csound)
  {
    CsoundCallbackWrapper *p;
    p = (CsoundCallbackWrapper*) csoundGetHostData(csound);
    return p->YieldCallback();
  }

}       // extern "C"

void CsoundCallbackWrapper::SetMessageCallback()
{
    csoundSetMessageCallback(csound_, MessageCallback_wrapper);
}

void CsoundCallbackWrapper::SetInputValueCallback()
{
    csoundSetInputValueCallback(csound_, InputValueCallback_wrapper);
}

void CsoundCallbackWrapper::SetOutputValueCallback()
{
    csoundSetOutputValueCallback(csound_, OutputValueCallback_wrapper);
}

void CsoundCallbackWrapper::SetYieldCallback()
{
    csoundSetYieldCallback(csound_, YieldCallback_wrapper);
}

CsoundCallbackWrapper::CsoundCallbackWrapper(Csound *cs)
{
    csound_ = cs->GetCsound();
    cs->SetHostData((void*) this);
}

CsoundCallbackWrapper::CsoundCallbackWrapper(CSOUND *cs)
{
    csound_ = cs;
    csoundSetHostData(cs, (void*) this);
}

