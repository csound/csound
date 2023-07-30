/*
    cs_glue.cpp:

    Copyright (C) 2005, 2006 Istvan Varga

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

#include "csound.h"
//#include "cs_glue.h"
#include "csound.hpp"
#include "csdl.h"
#include "cs_glue.hpp"

extern "C" {


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
    lst = (controlChannelInfo_t*) 0;
    cnt = -1;
    csound = (CSOUND*) 0;
}

int CsoundChannelList::GetChannelMetaData(int ndx,
                                          controlChannelHints_t *hints)
{
    const char  *name;
    if (!lst || (unsigned int) ndx >= (unsigned int) cnt)
      return -1;
    name = lst[ndx].name;
    int ret = csoundGetControlChannelHints(csound, name, hints);
    return ret;
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
    int   tmp;
    controlChannelHints_t hints;
    tmp = this->GetChannelMetaData(ndx, &hints);
    if (tmp >= 0) {
        tmp = hints.behav;
    }
    return tmp;
}

/**
 * Returns the default value set for the control channel at index 'ndx'
 * (counting from zero), or 0.0 if the channel does not exist, is not a
 * control channel, or has no default value.
 */

double CsoundChannelList::DefaultValue(int ndx)
{
    controlChannelHints_t hints;
    if (this->GetChannelMetaData(ndx, &hints) > 0)
      return hints.dflt;
    return 0.0;
}

/**
 * Returns the minimum value set for the control channel at index 'ndx'
 * (counting from zero), or 0.0 if the channel does not exist, is not a
 * control channel, or has no minimum value.
 */

double CsoundChannelList::MinValue(int ndx)
{
    controlChannelHints_t hints;
    if (this->GetChannelMetaData(ndx, &hints) > 0)
      return hints.min;
    return 0.0;
}

/**
 * Returns the maximum value set for the control channel at index 'ndx'
 * (counting from zero), or 0.0 if the channel does not exist, is not a
 * control channel, or has no maximum value.
 */

double CsoundChannelList::MaxValue(int ndx)
{
    controlChannelHints_t hints;
    if (this->GetChannelMetaData(ndx, &hints) > 0)
      return hints.max;
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
    lst = (controlChannelInfo_t*) 0;
    cnt = csoundListChannels(csound, &lst);
    this->csound = csound;
    if (cnt < 0 || !lst)
      this->ResetVariables();
}

CsoundChannelList::CsoundChannelList(Csound *csound)
{
    lst = (controlChannelInfo_t*) 0;
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
// FIXME - this is broken and does not work with STRINGDAT
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
    return ((STRINGDAT*) p)->data;
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
    new_argv[i] = (char*) 0;
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

  static CS_NOINLINE void MessageCallback_wrapper(CSOUND *csound,
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
          vsnprintf(bufp, 2048, fmt, args);
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

  /**
  static CS_NOINLINE void InputValueCallback_wrapper(CSOUND *csound,
                                         const char *chnName, MYFLT *value)
  {
    CsoundCallbackWrapper *p;
    p = (CsoundCallbackWrapper*) csoundGetHostData(csound);
    *value = (MYFLT) p->InputValueCallback(chnName);
  }
  */
  /**
  static CS_NOINLINE void OutputValueCallback_wrapper(CSOUND *csound,
                                          const char *chnName, MYFLT value)
  {
    CsoundCallbackWrapper *p;
    p = (CsoundCallbackWrapper*) csoundGetHostData(csound);
    p->OutputValueCallback(chnName, (double) value);
  }
  */
  static CS_NOINLINE int YieldCallback_wrapper(CSOUND *csound)
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

  /*void CsoundCallbackWrapper::SetInputValueCallback()
{
    csoundSetInputValueCallback(csound_, InputValueCallback_wrapper);
}

void CsoundCallbackWrapper::SetOutputValueCallback()
{
    csoundSetOutputValueCallback(csound_, OutputValueCallback_wrapper);
}
  */
void CsoundCallbackWrapper::SetYieldCallback()
{
    csoundSetYieldCallback(csound_, YieldCallback_wrapper);
}

void CsoundCallbackWrapper::SetMidiInputCallback(CsoundArgVList *argv)
{
    csoundSetExternalMidiInOpenCallback(csound_, midiInOpenCallback);
    csoundSetExternalMidiReadCallback(csound_, midiInReadCallback);
    csoundSetExternalMidiInCloseCallback(csound_, midiInCloseCallback);
    if (argv != (CsoundArgVList*) 0) {
      argv->Append("-+rtmidi=null");
      argv->Append("-M0");
    }
    csoundMessage(csound_,
                  "rtmidi: CsoundCallbackWrapper::MidiInputCallback() "
                  "enabled\n");
}

void CsoundCallbackWrapper::SetMidiOutputCallback(CsoundArgVList *argv)
{
    csoundSetExternalMidiOutOpenCallback(csound_, midiOutOpenCallback);
    csoundSetExternalMidiWriteCallback(csound_, midiOutWriteCallback);
    csoundSetExternalMidiOutCloseCallback(csound_, midiOutCloseCallback);
    if (argv != (CsoundArgVList*) 0) {
      argv->Append("-+rtmidi=null");
      argv->Append("-Q0");
    }
    csoundMessage(csound_,
                  "rtmidi: CsoundCallbackWrapper::MidiOutputCallback() "
                  "enabled\n");
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

int CsoundCallbackWrapper::midiInOpenCallback(CSOUND *csound, void **userData,
                                              const char *devName)
{
    (void) devName;
    (*userData) = csoundGetHostData(csound);
    return 0;
}

int CsoundCallbackWrapper::midiInReadCallback(CSOUND *csound, void *userData,
                                              unsigned char *buf, int nBytes)
{
    CsoundMidiInputBuffer   buf_(buf, nBytes);
    int                     bytesRead;

    (void) csound;
    ((CsoundCallbackWrapper*) userData)->MidiInputCallback(&buf_);
    bytesRead = buf_.bufBytes;
    return bytesRead;
}

int CsoundCallbackWrapper::midiInCloseCallback(CSOUND *csound, void *userData)
{
    (void) csound;
    (void) userData;
    return 0;
}

int CsoundCallbackWrapper::midiOutOpenCallback(CSOUND *csound, void **userData,
                                               const char *devName)
{
    (void) devName;
    (*userData) = csoundGetHostData(csound);
    return 0;
}

int CsoundCallbackWrapper::midiOutWriteCallback(CSOUND *csound, void *userData,
                                                const unsigned char *buf,
                                                int nBytes)
{
    CsoundMidiOutputBuffer  buf_((unsigned char*) buf, nBytes);
    int                     bytesWritten;

    (void) csound;
    buf_.bufBytes = nBytes;
    ((CsoundCallbackWrapper*) userData)->MidiOutputCallback(&buf_);
    bytesWritten = nBytes - buf_.bufBytes;
    return bytesWritten;
}

int CsoundCallbackWrapper::midiOutCloseCallback(CSOUND *csound, void *userData)
{
    (void) csound;
    (void) userData;
    return 0;
}
  /*
void CsoundCallbackWrapper::SetChannelIOCallbacks()
{
    csoundSetChannelIOCallback(csound_,
                               (CsoundChannelIOCallback_t)
                                   ChannelIOCallback_wrapper);
}
  */
// ----------------------------------------------------------------------------

static const unsigned char midiMessageByteCnt[32] = {
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
    3, 3, 3, 3,  3, 3, 3, 3,  2, 2, 2, 2,  3, 3, 0, 1
};

CsoundMidiInputBuffer::CsoundMidiInputBuffer(unsigned char *buf, int bufSize)
{
    this->buf = buf;
    mutex_ = csoundCreateMutex(0);
    bufReadPos = 0;
    bufWritePos = 0;
    bufBytes = 0;
    this->bufSize = bufSize;
}

CsoundMidiInputBuffer::~CsoundMidiInputBuffer()
{
    buf = (unsigned char*) 0;
    csoundDestroyMutex(mutex_);
    mutex_ = (void*) 0;
}

/**
 * Sends a MIDI message, 'msg' is calculated as follows:
 *   STATUS + DATA1 * 256 + DATA2 * 65536
 */

void CsoundMidiInputBuffer::SendMidiMessage(int msg)
{
    int   nBytes = (int) midiMessageByteCnt[(msg & (int) 0xF8) >> 3];

    if (!nBytes)
      return;
    csoundLockMutex(mutex_);
    if ((bufBytes + nBytes) <= bufSize) {
      buf[bufWritePos] = (unsigned char) msg & (unsigned char) 0xFF;
      bufWritePos = (bufWritePos < (bufSize - 1) ? bufWritePos + 1 : 0);
      bufBytes++;
      if (nBytes > 1) {
        buf[bufWritePos] = (unsigned char) (msg >> 8) & (unsigned char) 0x7F;
        bufWritePos = (bufWritePos < (bufSize - 1) ? bufWritePos + 1 : 0);
        bufBytes++;
        if (nBytes > 2) {
          buf[bufWritePos] = (unsigned char) (msg >> 16)
                             & (unsigned char) 0x7F;
          bufWritePos = (bufWritePos < (bufSize - 1) ? bufWritePos + 1 : 0);
          bufBytes++;
        }
      }
    }
    csoundUnlockMutex(mutex_);
}

/**
 * Sends a MIDI message; 'channel' should be in the range 1 to 16,
 * and data1 and data2 should be in the range 0 to 127.
 */

void CsoundMidiInputBuffer::SendMidiMessage(int status, int channel,
                                        int data1, int data2)
{
    int   nBytes = (int) midiMessageByteCnt[(status & (int) 0xF8) >> 3];

    if (!nBytes)
      return;
    csoundLockMutex(mutex_);
    if ((bufBytes + nBytes) <= bufSize) {
      unsigned char st = (unsigned char) status & (unsigned char) 0xFF;
      if (nBytes > 1) {
        st = (st & (unsigned char) 0xF0)
             + ((unsigned char) (status + channel - 1) & (unsigned char) 0x0F);
      }
      buf[bufWritePos] = st;
      bufWritePos = (bufWritePos < (bufSize - 1) ? bufWritePos + 1 : 0);
      bufBytes++;
      if (nBytes > 1) {
        buf[bufWritePos] = (unsigned char) data1 & (unsigned char) 0x7F;
        bufWritePos = (bufWritePos < (bufSize - 1) ? bufWritePos + 1 : 0);
        bufBytes++;
        if (nBytes > 2) {
          buf[bufWritePos] = (unsigned char) data2 & (unsigned char) 0x7F;
          bufWritePos = (bufWritePos < (bufSize - 1) ? bufWritePos + 1 : 0);
          bufBytes++;
        }
      }
    }
    csoundUnlockMutex(mutex_);
}

/**
 * Sends a note-on message on 'channel' (1 to 16) for 'key' (0 to 127)
 * with 'velocity' (0 to 127).
 */

void CsoundMidiInputBuffer::SendNoteOn(int channel, int key, int velocity)
{
    SendMidiMessage((int) 0x90, channel, key, velocity);
}

/**
 * Sends a note-off message on 'channel' (1 to 16) for 'key' (0 to 127)
 * with 'velocity' (0 to 127).
 */

void CsoundMidiInputBuffer::SendNoteOff(int channel, int key, int velocity)
{
    SendMidiMessage((int) 0x80, channel, key, velocity);
}

/**
 * Sends a note-off message on 'channel' (1 to 16) for 'key',
 * using a 0x90 status with zero velocity.
 */

void CsoundMidiInputBuffer::SendNoteOff(int channel, int key)
{
    SendMidiMessage((int) 0x90, channel, key, 0);
}

/**
 * Sets polyphonic pressure on 'channel' (1 to 16) to 'value' (0 to 127)
 * for 'key' (0 to 127).
 */

void CsoundMidiInputBuffer::SendPolyphonicPressure(int channel, int key,
                                                   int value)
{
    SendMidiMessage((int) 0xA0, channel, key, value);
}

/**
 * Sets controller 'ctl' (0 to 127) to 'value' (0 to 127)
 * on 'channel' (1 to 16).
 */

void CsoundMidiInputBuffer::SendControlChange(int channel, int ctl, int value)
{
    SendMidiMessage((int) 0xB0, channel, ctl, value);
}

/**
 * Sends program change to 'pgm' (1 to 128) on 'channel' (1 to 16).
 */

void CsoundMidiInputBuffer::SendProgramChange(int channel, int pgm)
{
    SendMidiMessage((int) 0xC0, channel, pgm - 1, 0);
}

/**
 * Sets channel pressure to 'value' (0 to 127) on 'channel' (1 to 16).
 */

void CsoundMidiInputBuffer::SendChannelPressure(int channel, int value)
{
    SendMidiMessage((int) 0xD0, channel, value, 0);
}

/**
 * Sets pitch bend to 'value' (-8192 to 8191) on 'channel' (1 to 16).
 */

void CsoundMidiInputBuffer::SendPitchBend(int channel, int value)
{
    SendMidiMessage((int) 0xE0, channel,
                (value + 8192) & (int) 0x7F,
                ((value + 8192) >> 7) & (int) 0x7F);
}

/**
 * Copies at most 'nBytes' bytes of MIDI data from the buffer to 'buf'.
 * Returns the number of bytes copied.
 */

int CsoundMidiInputBuffer::GetMidiData(unsigned char *buf, int nBytes)
{
    int   i;

    if (!bufBytes)
      return 0;
    csoundLockMutex(mutex_);
    for (i = 0; i < nBytes && bufBytes > 0; i++) {
      buf[i] = this->buf[bufReadPos];
      bufReadPos = (bufReadPos < (bufSize - 1) ? bufReadPos + 1 : 0);
      bufBytes--;
    }
    csoundUnlockMutex(mutex_);
    return i;
}

// ----------------------------------------------------------------------------

/**
 * The following class allows sending MIDI input messages to a Csound
 * instance.
 */

CsoundMidiInputStream::CsoundMidiInputStream(CSOUND *csound)
    : CsoundMidiInputBuffer(&(buf_[0]), 4096)
{
    this->csound = csound;
}

CsoundMidiInputStream::CsoundMidiInputStream(Csound *csound)
    : CsoundMidiInputBuffer(&(buf_[0]), 4096)
{
    this->csound = csound->GetCsound();
}

int CsoundMidiInputStream::midiInOpenCallback(CSOUND *csound, void **userData,
                                              const char *devName)
{
    (void) devName;
    (*userData) = *((void**) csoundQueryGlobalVariable(csound,
                                                       "__csnd_MidiInObject"));
    return 0;
}

int CsoundMidiInputStream::midiInReadCallback(CSOUND *csound, void *userData,
                                              unsigned char *buf, int nBytes)
{
    (void) csound;
    return ((CsoundMidiInputStream*) userData)->GetMidiData(buf, nBytes);
}

int CsoundMidiInputStream::midiInCloseCallback(CSOUND *csound, void *userData)
{
    (void) csound;
    (void) userData;
    return 0;
}

/**
 * Enables MIDI input for the associated Csound instance.
 * Should be called between csoundPreCompile() and csoundCompile().
 * If 'argv' is not NULL, the command line arguments required for
 * MIDI input are appended.
 */

void CsoundMidiInputStream::EnableMidiInput(CsoundArgVList *argv)
{
    csoundCreateGlobalVariable(csound, "__csnd_MidiInObject",
                                             sizeof(void*));
    *((void**) csoundQueryGlobalVariable(csound, "__csnd_MidiInObject")) =
        (void*) this;
    csoundSetExternalMidiInOpenCallback(csound, midiInOpenCallback);
    csoundSetExternalMidiReadCallback(csound, midiInReadCallback);
    csoundSetExternalMidiInCloseCallback(csound, midiInCloseCallback);
    if (argv != (CsoundArgVList*) 0) {
      argv->Append("-+rtmidi=null");
      argv->Append("-M0");
    }
    csoundMessage(csound, "rtmidi: CsoundMidiInputStream enabled\n");
}

// ----------------------------------------------------------------------------

CsoundMidiOutputBuffer::CsoundMidiOutputBuffer(unsigned char *buf, int bufSize)
{
    this->buf = buf;
    mutex_ = csoundCreateMutex(0);
    bufReadPos = 0;
    bufWritePos = 0;
    bufBytes = 0;
    this->bufSize = bufSize;
}

CsoundMidiOutputBuffer::~CsoundMidiOutputBuffer()
{
    buf = (unsigned char*) 0;
    csoundDestroyMutex(mutex_);
    mutex_ = (void*) 0;
}

/**
 * Pops and returns the first message from the buffer, in the following
 * format:
 *   STATUS + DATA1 * 256 + DATA2 * 65536
 * where STATUS also includes the channel number (0 to 15), if any.
 * The return value is zero if there are no messages.
 */

int CsoundMidiOutputBuffer::PopMessage()
{
    int     msg = 0;

    if (bufBytes) {
      csoundLockMutex(mutex_);
      if (bufBytes > 0) {
        int             nBytes;
        unsigned char   st;

        st = buf[bufReadPos] & (unsigned char) 0xFF;
        nBytes = (int) midiMessageByteCnt[(st & (unsigned char) 0xF8) >> 3];
        if (nBytes > 0 && bufBytes >= nBytes) {
          bufReadPos = (bufReadPos < (bufSize - 1) ? bufReadPos + 1 : 0);
          bufBytes--;
          msg = (int) st;
          if (nBytes > 1) {
            msg += ((int) (buf[bufReadPos] & (unsigned char) 0x7F) << 8);
            bufReadPos = (bufReadPos < (bufSize - 1) ? bufReadPos + 1 : 0);
            bufBytes--;
            if (nBytes > 2) {
              msg += ((int) (buf[bufReadPos] & (unsigned char) 0x7F) << 16);
              bufReadPos = (bufReadPos < (bufSize - 1) ? bufReadPos + 1 : 0);
              bufBytes--;
            }
          }
        }
        else {
          // invalid MIDI data, discard the rest of the buffer
          bufReadPos = bufWritePos;
          bufBytes = 0;
        }
      }
      csoundUnlockMutex(mutex_);
    }
    return msg;
}

/**
 * Returns the status byte for the first message in the buffer, not
 * including the channel number in the case of channel messages.
 * The return value is zero if there are no messages.
 */

int CsoundMidiOutputBuffer::GetStatus()
{
    unsigned char   st = (unsigned char) 0;

    if (bufBytes) {
      csoundLockMutex(mutex_);
      if (bufBytes > 0) {
        int   nBytes;
        st = buf[bufReadPos] & (unsigned char) 0xFF;
        nBytes = (int) midiMessageByteCnt[(st & (unsigned char) 0xF8) >> 3];
        if (nBytes <= 0 || bufBytes < nBytes)
          st = (unsigned char) 0;       // invalid MIDI data
        if (nBytes > 1)
          st &= (unsigned char) 0xF0;   // channel msg: remove channel number
      }
      csoundUnlockMutex(mutex_);
    }
    return (int) st;
}

/**
 * Returns the channel number (1 to 16) for the first message in the
 * buffer.  The return value is zero if there are no messages, or the
 * first message is not a channel message.
 */

int CsoundMidiOutputBuffer::GetChannel()
{
    unsigned char   st = (unsigned char) 0;

    if (bufBytes) {
      csoundLockMutex(mutex_);
      if (bufBytes > 0) {
        int   nBytes;
        st = buf[bufReadPos] & (unsigned char) 0xFF;
        nBytes = (int) midiMessageByteCnt[(st & (unsigned char) 0xF8) >> 3];
        if (nBytes < 2 || bufBytes < nBytes)
          st = (unsigned char) 0;       // invalid MIDI data, or system message
        else
          st = (st & (unsigned char) 0x0F) + (unsigned char) 1;
      }
      csoundUnlockMutex(mutex_);
    }
    return (int) st;
}

/**
 * Returns the first data byte (0 to 127) for the first message in the
 * buffer.  The return value is zero if there are no messages, or the
 * first message does not have any data bytes.
 */

int CsoundMidiOutputBuffer::GetData1()
{
    unsigned char   st, d1 = (unsigned char) 0;

    if (bufBytes) {
      csoundLockMutex(mutex_);
      if (bufBytes > 0) {
        int   nBytes;
        st = buf[bufReadPos] & (unsigned char) 0xFF;
        nBytes = (int) midiMessageByteCnt[(st & (unsigned char) 0xF8) >> 3];
        if (nBytes >= 2 && bufBytes >= nBytes) {
          int   pos = bufReadPos;
          pos = (pos < (bufSize - 1) ? pos + 1 : 0);
          d1 = buf[pos] & (unsigned char) 0x7F;
        }
      }
      csoundUnlockMutex(mutex_);
    }
    return (int) d1;
}

/**
 * Returns the second data byte (0 to 127) for the first message in the
 * buffer.  The return value is zero if there are no messages, or the
 * first message has less than two data bytes.
 */

int CsoundMidiOutputBuffer::GetData2()
{
    unsigned char   st, d2 = (unsigned char) 0;

    if (bufBytes) {
      csoundLockMutex(mutex_);
      if (bufBytes > 0) {
        int   nBytes;
        st = buf[bufReadPos] & (unsigned char) 0xFF;
        nBytes = (int) midiMessageByteCnt[(st & (unsigned char) 0xF8) >> 3];
        if (nBytes >= 3 && bufBytes >= nBytes) {
          int   pos = bufReadPos;
          pos = (pos < (bufSize - 1) ? pos + 1 : 0);
          pos = (pos < (bufSize - 1) ? pos + 1 : 0);
          d2 = buf[pos] & (unsigned char) 0x7F;
        }
      }
      csoundUnlockMutex(mutex_);
    }
    return (int) d2;
}

/**
 * Copies at most 'nBytes' bytes of MIDI data to the buffer from 'buf'.
 * Returns the number of bytes copied.
 */

int CsoundMidiOutputBuffer::SendMidiData(const unsigned char *buf, int nBytes)
{
    int   i;

    csoundLockMutex(mutex_);
    for (i = 0; i < nBytes && bufBytes < bufSize; i++) {
      this->buf[bufWritePos] = buf[i];
      bufWritePos = (bufWritePos < (bufSize - 1) ? bufWritePos + 1 : 0);
      bufBytes++;
    }
    csoundUnlockMutex(mutex_);
    return i;
}

// ----------------------------------------------------------------------------

/**
 * The following class allows receiving MIDI output messages
 * from a Csound instance.
 */

CsoundMidiOutputStream::CsoundMidiOutputStream(CSOUND *csound)
    : CsoundMidiOutputBuffer(&(buf_[0]), 4096)
{
    this->csound = csound;
}

CsoundMidiOutputStream::CsoundMidiOutputStream(Csound *csound)
    : CsoundMidiOutputBuffer(&(buf_[0]), 4096)
{
    this->csound = csound->GetCsound();
}

int CsoundMidiOutputStream::midiOutOpenCallback(CSOUND *csound,
                                                void **userData,
                                                const char *devName)
{
    (void) devName;
    (*userData) = *((void**) csoundQueryGlobalVariable(csound,
                                                       "__csnd_MidiOutObject"));
    return 0;
}

int CsoundMidiOutputStream::midiOutWriteCallback(CSOUND *csound,
                                                 void *userData,
                                                 const unsigned char *buf,
                                                 int nBytes)
{
    (void) csound;
    return ((CsoundMidiOutputStream*) userData)->SendMidiData(buf, nBytes);
}

int CsoundMidiOutputStream::midiOutCloseCallback(CSOUND *csound,
                                                 void *userData)
{
    (void) csound;
    (void) userData;
    return 0;
}

/**
 * Enables MIDI output for the associated Csound instance.
 * Should be called between csoundPreCompile() and csoundCompile().
 * If 'argv' is not NULL, the command line arguments required for
 * MIDI output are appended.
 */

void CsoundMidiOutputStream::EnableMidiOutput(CsoundArgVList *argv)
{
    csoundCreateGlobalVariable(csound, "__csnd_MidiOutObject",
                                             sizeof(void*));
    *((void**) csoundQueryGlobalVariable(csound, "__csnd_MidiOutObject")) =
        (void*) this;
    csoundSetExternalMidiOutOpenCallback(csound, midiOutOpenCallback);
    csoundSetExternalMidiWriteCallback(csound, midiOutWriteCallback);
    csoundSetExternalMidiOutCloseCallback(csound, midiOutCloseCallback);
    if (argv != (CsoundArgVList*) 0) {
      argv->Append("-+rtmidi=null");
      argv->Append("-Q0");
    }
    csoundMessage(csound, "rtmidi: CsoundMidiOutputStream enabled\n");
}

};

