/*
    cs_glue.hpp:

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#ifndef CSOUND_CS_GLUE_HPP
#define CSOUND_CS_GLUE_HPP

/**
 * CsoundOpcodeList(CSOUND *)
 * CsoundOpcodeList(Csound *)
 *
 * Creates an alphabetically sorted opcode list for a Csound instance.
 * Should be called after csoundCompile() or Csound::Compile().
 */

class PUBLIC CsoundOpcodeList {
 private:
    opcodeListEntry *lst;
    int             cnt;
 public:
    /**
     * Returns the number of opcodes, or -1 if there is no list.
     */
    int Count();
    /**
     * Returns the name of the opcode at index 'ndx' (counting from zero),
     * or NULL if the index is out of range.
     */
    const char *Name(int ndx);
    /**
     * Returns the output types of the opcode at index 'ndx' (counting from
     * zero), or NULL if the index is out of range.
     */
    const char *OutTypes(int ndx);
    /**
     * Returns the input types of the opcode at index 'ndx' (counting from
     * zero), or NULL if the index is out of range.
     */
    const char *InTypes(int ndx);
    /**
     * Releases the memory used by the opcode list. Should be called
     * before the Csound instance is destroyed or reset.
     */
    void Clear();
    // --------
    CsoundOpcodeList(CSOUND *csound);
    CsoundOpcodeList(Csound *csound);
    ~CsoundOpcodeList();
};

/**
 * CsoundChannelList(CSOUND *)
 * CsoundChannelList(Csound *)
 *
 * Creates an alphabetically sorted list of named channels of a Csound
 * instance. Should be called after csoundCompile() or Csound::Compile().
 */

class PUBLIC CsoundChannelList {
 private:
    controlChannelInfo_t  *lst;
    int                     cnt;
    CSOUND                  *csound;
    void ResetVariables();
    int GetChannelMetaData(int ndx, controlChannelHints_t *hints);
 public:
    /**
     * Returns the number of channels (-1 if there is no list).
     */
    int Count();
    /**
     * Returns the name of the channel at index 'ndx' (counting from zero),
     * or NULL if the index is out of range.
     */
    const char *Name(int ndx);
    /**
     * Returns the type of the channel at index 'ndx' (counting from zero),
     * or -1 if the index is out of range.
     */
    int Type(int ndx);
    /**
     * Returns 1 if the channel at index 'ndx' (counting from zero) exists
     * and is a control channel, and 0 otherwise.
     */
    int IsControlChannel(int ndx);
    /**
     * Returns 1 if the channel at index 'ndx' (counting from zero) exists
     * and is an audio channel, and 0 otherwise.
     */
    int IsAudioChannel(int ndx);
    /**
     * Returns 1 if the channel at index 'ndx' (counting from zero) exists
     * and is a string channel, and 0 otherwise.
     */
    int IsStringChannel(int ndx);
    /**
     * Returns 1 if the channel at index 'ndx' (counting from zero) exists
     * and the input bit is set, and 0 otherwise.
     */
    int IsInputChannel(int ndx);
    /**
     * Returns 1 if the channel at index 'ndx' (counting from zero) exists
     * and the output bit is set, and 0 otherwise.
     */
    int IsOutputChannel(int ndx);
    /**
     * Returns the sub-type (0: normal, 1: integer, 2: linear, 3: exponential)
     * of the control channel at index 'ndx' (counting from zero), or -1 if
     * the channel does not exist or is not a control channel.
     */
    int SubType(int ndx);
    /**
     * Returns the default value set for the control channel at index 'ndx'
     * (counting from zero), or 0.0 if the channel does not exist, is not a
     * control channel, or has no default value.
     */
    double DefaultValue(int ndx);
    /**
     * Returns the minimum value set for the control channel at index 'ndx'
     * (counting from zero), or 0.0 if the channel does not exist, is not a
     * control channel, or has no minimum value.
     */
    double MinValue(int ndx);
    /**
     * Returns the maximum value set for the control channel at index 'ndx'
     * (counting from zero), or 0.0 if the channel does not exist, is not a
     * control channel, or has no maximum value.
     */
    double MaxValue(int ndx);
    /**
     * Releases the memory used by the channel list. Should be called
     * before the Csound instance is destroyed or reset.
     */
    void Clear();
    // --------
    CsoundChannelList(CSOUND *csound);
    CsoundChannelList(Csound *csound);
    ~CsoundChannelList();
};

/**
 * CsoundUtilityList(CSOUND *)
 * CsoundUtilityList(Csound *)
 *
 * Creates an alphabetically sorted list of utilities registered
 * for a Csound instance. Should be called after csoundPreCompile()
 * or Csound::PreCompile().
 */

class PUBLIC CsoundUtilityList {
 private:
    char  **lst;
    int   cnt;
 public:
    /**
     * Returns the number of utilities, or -1 if there is no list.
     */
    int Count();
    /**
     * Returns the name of the utility at index 'ndx' (counting from zero),
     * or NULL if the index is out of range.
     */
    const char *Name(int ndx);
    /**
     * Releases the memory used by the utility list. Should be called
     * before the Csound instance is destroyed or reset.
     */
    void Clear();
    // --------
    CsoundUtilityList(CSOUND *csound);
    CsoundUtilityList(Csound *csound);
    ~CsoundUtilityList();
};

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

class PUBLIC CsoundMYFLTArray {
 private:
    MYFLT *p;
    void  *pp;
 public:
    /**
     * Returns a MYFLT** pointer for use with csoundGetChannelPtr().
     */
    MYFLT **GetPtr()
    {
      return (&p);
    }
    /**
     * Returns the address of the element at index 'ndx' (counting from
     * zero) as a MYFLT* pointer, or NULL if there is no array. Does not
     * check if 'ndx' is valid.
     */
    MYFLT *GetPtr(int ndx)
    {
      if (p)
        return &(p[ndx]);
      return (MYFLT*) 0;
    }
    /**
     * Sets the array pointer to a MYFLT* value returned by a Csound
     * API function (e.g. csoundGetSpin() or csoundGetSpout()).
     */
    void SetPtr(MYFLT *ptr)
    {
      p = ptr;
    }
    /**
     * Stores a floating point value at index 'ndx' (counting from zero).
     * No error checking is done, the array is assumed to exist and the
     * index is assumed to be valid.
     */
    void SetValue(int ndx, double value)
    {
      p[ndx] = (MYFLT) value;
    }
    /**
     * Sets two floating point values starting at index 'ndx' (counting
     * from zero). No error checking is done, the array is assumed to exist
     * and the index is assumed to be valid.
     */
    void SetValues(int ndx, double v0, double v1)
    {
      p[ndx]     = (MYFLT) v0;  p[ndx + 1] = (MYFLT) v1;
    }
    /**
     * Sets three floating point values starting at index 'ndx' (counting
     * from zero). No error checking is done, the array is assumed to exist
     * and the index is assumed to be valid.
     */
    void SetValues(int ndx, double v0, double v1, double v2)
    {
      p[ndx]     = (MYFLT) v0;  p[ndx + 1] = (MYFLT) v1;
      p[ndx + 2] = (MYFLT) v2;
    }
    /**
     * Sets four floating point values starting at index 'ndx' (counting
     * from zero). No error checking is done, the array is assumed to exist
     * and the index is assumed to be valid.
     */
    void SetValues(int ndx, double v0, double v1, double v2, double v3)
    {
      p[ndx]     = (MYFLT) v0;  p[ndx + 1] = (MYFLT) v1;
      p[ndx + 2] = (MYFLT) v2;  p[ndx + 3] = (MYFLT) v3;
    }
    /**
     * Sets five floating point values starting at index 'ndx' (counting
     * from zero). No error checking is done, the array is assumed to exist
     * and the index is assumed to be valid.
     */
    void SetValues(int ndx,
                   double v0, double v1, double v2, double v3, double v4)
    {
      p[ndx]     = (MYFLT) v0;  p[ndx + 1] = (MYFLT) v1;
      p[ndx + 2] = (MYFLT) v2;  p[ndx + 3] = (MYFLT) v3;
      p[ndx + 4] = (MYFLT) v4;
    }
    /**
     * Sets six floating point values starting at index 'ndx' (counting
     * from zero). No error checking is done, the array is assumed to exist
     * and the index is assumed to be valid.
     */
    void SetValues(int ndx,
                   double v0, double v1, double v2, double v3, double v4,
                   double v5)
    {
      p[ndx]     = (MYFLT) v0;  p[ndx + 1] = (MYFLT) v1;
      p[ndx + 2] = (MYFLT) v2;  p[ndx + 3] = (MYFLT) v3;
      p[ndx + 4] = (MYFLT) v4;  p[ndx + 5] = (MYFLT) v5;
    }
    /**
     * Sets seven floating point values starting at index 'ndx' (counting
     * from zero). No error checking is done, the array is assumed to exist
     * and the index is assumed to be valid.
     */
    void SetValues(int ndx,
                   double v0, double v1, double v2, double v3, double v4,
                   double v5, double v6)
    {
      p[ndx]     = (MYFLT) v0;  p[ndx + 1] = (MYFLT) v1;
      p[ndx + 2] = (MYFLT) v2;  p[ndx + 3] = (MYFLT) v3;
      p[ndx + 4] = (MYFLT) v4;  p[ndx + 5] = (MYFLT) v5;
      p[ndx + 6] = (MYFLT) v6;
    }
    /**
     * Sets eight floating point values starting at index 'ndx' (counting
     * from zero). No error checking is done, the array is assumed to exist
     * and the index is assumed to be valid.
     */
    void SetValues(int ndx,
                   double v0, double v1, double v2, double v3, double v4,
                   double v5, double v6, double v7)
    {
      p[ndx]     = (MYFLT) v0;  p[ndx + 1] = (MYFLT) v1;
      p[ndx + 2] = (MYFLT) v2;  p[ndx + 3] = (MYFLT) v3;
      p[ndx + 4] = (MYFLT) v4;  p[ndx + 5] = (MYFLT) v5;
      p[ndx + 6] = (MYFLT) v6;  p[ndx + 7] = (MYFLT) v7;
    }
    /**
     * Sets nine floating point values starting at index 'ndx' (counting
     * from zero). No error checking is done, the array is assumed to exist
     * and the index is assumed to be valid.
     */
    void SetValues(int ndx,
                   double v0, double v1, double v2, double v3, double v4,
                   double v5, double v6, double v7, double v8)
    {
      p[ndx]     = (MYFLT) v0;  p[ndx + 1] = (MYFLT) v1;
      p[ndx + 2] = (MYFLT) v2;  p[ndx + 3] = (MYFLT) v3;
      p[ndx + 4] = (MYFLT) v4;  p[ndx + 5] = (MYFLT) v5;
      p[ndx + 6] = (MYFLT) v6;  p[ndx + 7] = (MYFLT) v7;
      p[ndx + 8] = (MYFLT) v8;
    }
    /**
     * Sets ten floating point values starting at index 'ndx' (counting
     * from zero). No error checking is done, the array is assumed to exist
     * and the index is assumed to be valid.
     */
    void SetValues(int ndx,
                   double v0, double v1, double v2, double v3, double v4,
                   double v5, double v6, double v7, double v8, double v9)
    {
      p[ndx]     = (MYFLT) v0;  p[ndx + 1] = (MYFLT) v1;
      p[ndx + 2] = (MYFLT) v2;  p[ndx + 3] = (MYFLT) v3;
      p[ndx + 4] = (MYFLT) v4;  p[ndx + 5] = (MYFLT) v5;
      p[ndx + 6] = (MYFLT) v6;  p[ndx + 7] = (MYFLT) v7;
      p[ndx + 8] = (MYFLT) v8;  p[ndx + 9] = (MYFLT) v9;
    }
    /**
     * Returns the floating point value at index 'ndx' (counting from zero).
     * No error checking is done, the array is assumed to exist and the
     * index is assumed to be valid.
     */
    double GetValue(int ndx)
    {
      return (double) p[ndx];
    }
    /**
     * Copies 'n' values to the array from a source pointer, starting at
     * index 'ndx' (counting from zero). No error checking is done.
     */
    void SetValues(int ndx, int n, const MYFLT *src)
    {
      for (int i = 0; i < n; i++)
        p[ndx + i] = src[i];
    }
    /**
     * Copies 'n' values from the array to 'dst', starting at index 'ndx'
     * (counting from zero). No error checking is done.
     */
    void GetValues(MYFLT *dst, int ndx, int n)
    {
      for (int i = 0; i < n; i++)
        dst[i] = p[ndx + i];
    }
    /**
     * Stores a string in the array (note: only do this with a pointer
     * returned by csoundGetChannelPtr()), optionally limiting the length to
     * maxLen - 1 characters.
     */
    void SetStringValue(const char *s, int maxLen);
    /**
     * Returns a string from the array (note: only do this with a pointer
     * returned by csoundGetChannelPtr()), or NULL if there is no array.
     */
    const char *GetStringValue();
    /**
     * Clears the array pointer, and releases any memory that was allocated
     * by calling the constructor with a positive number of elements.
     */
    void Clear();
    // --------
    CsoundMYFLTArray();
    CsoundMYFLTArray(int n);
    ~CsoundMYFLTArray();
};

/**
 * A simple class for creating argv[] lists for use with
 * functions like csoundCompile().
 */

class PUBLIC CsoundArgVList {
 private:
    char  **ArgV_;
    int   cnt;
    void destroy_argv();
 public:
    /**
     * Returns the count of arguments in the list, zero if there are
     * none, and -1 if the list could not be allocated.
     */
    int argc();
    /**
     * Returns a char** pointer for use with csoundCompile() etc.
     */
    char **argv();
    /**
     * Returns the argument at the specified index (counting from zero),
     * or NULL if the index is out of range.
     */
    const char *argv(int ndx);
    /**
     * Inserts a new value to the argument list at the specified index
     * (counting from zero). If there is not enough memory, the list is
     * not changed.
     */
    void Insert(int ndx, const char *s);
    /**
     * Appends a new value at the end of the argument list.
     * If there is not enough memory, the list is not changed.
     */
    void Append(const char *s);
    /**
     * Removes all elements of the list.
     */
    void Clear();
    // --------
    CsoundArgVList();
    ~CsoundArgVList();
};

/**
 * Experimental class for wrapping callbacks using SWIG directors.
 */

class CsoundMidiInputBuffer;
class CsoundMidiOutputBuffer;

class PUBLIC CsoundCallbackWrapper {
 private:
    CSOUND  *csound_;
 public:
    virtual void MessageCallback(int attr, char *msg)
    {
      (void) attr;
      (void) msg;
    }
    virtual double InputValueCallback(const char *chnName)
    {
      (void) chnName;
      return 0.0;
    }
    virtual void OutputValueCallback(const char *chnName, double value)
    {
      (void) chnName;
      (void) value;
    }
    virtual int YieldCallback()
    {
      return 1;
    }
    virtual void MidiInputCallback(CsoundMidiInputBuffer *p)
    {
      (void) p;
    }
    virtual void MidiOutputCallback(CsoundMidiOutputBuffer *p)
    {
      (void) p;
    }

    void SetMessageCallback();
    // void SetInputValueCallback();
    // void SetOutputValueCallback();
    void SetYieldCallback();
    void SetMidiInputCallback(CsoundArgVList *argv);
    void SetMidiOutputCallback(CsoundArgVList *argv);
    // void SetChannelIOCallbacks();
    CSOUND *GetCsound()
    {
      return csound_;
    }
    // for converting SWIG char* type to a Python string
    static const char *CharPtrToString(const char *s)
    {
      return s;
    }
    // --------
    CsoundCallbackWrapper(Csound *csound);
    CsoundCallbackWrapper(CSOUND *csound);
    virtual ~CsoundCallbackWrapper()
    {
    }
 private:
    static int midiInOpenCallback(CSOUND *, void **, const char *);
    static int midiInReadCallback(CSOUND *, void *, unsigned char *, int);
    static int midiInCloseCallback(CSOUND *, void *);
    static int midiOutOpenCallback(CSOUND *, void **, const char *);
    static int midiOutWriteCallback(CSOUND *, void *,
                                    const unsigned char *, int);
    static int midiOutCloseCallback(CSOUND *, void *);
};

// ---------------------------- MIDI INPUT ----------------------------

class PUBLIC CsoundMidiInputBuffer {
 private:
    unsigned char   *buf;
    void            *mutex_;
    int             bufReadPos;
    int             bufWritePos;
    int             bufBytes;
    int             bufSize;
 public:
    CsoundMidiInputBuffer(unsigned char *buf, int bufSize);
    ~CsoundMidiInputBuffer();
    /**
     * Sends a MIDI message, 'msg' is calculated as follows:
     *   STATUS + DATA1 * 256 + DATA2 * 65536
     */
    void SendMidiMessage(int msg);
    /**
     * Sends a MIDI message; 'channel' should be in the range 1 to 16,
     * and data1 and data2 should be in the range 0 to 127.
     */
    void SendMidiMessage(int status, int channel, int data1, int data2);
    /**
     * Sends a note-on message on 'channel' (1 to 16) for 'key' (0 to 127)
     * with 'velocity' (0 to 127).
     */
    void SendNoteOn(int channel, int key, int velocity);
    /**
     * Sends a note-off message on 'channel' (1 to 16) for 'key' (0 to 127)
     * with 'velocity' (0 to 127).
     */
    void SendNoteOff(int channel, int key, int velocity);
    /**
     * Sends a note-off message on 'channel' (1 to 16) for 'key',
     * using a 0x90 status with zero velocity.
     */
    void SendNoteOff(int channel, int key);
    /**
     * Sets polyphonic pressure on 'channel' (1 to 16) to 'value' (0 to 127)
     * for 'key' (0 to 127).
     */
    void SendPolyphonicPressure(int channel, int key, int value);
    /**
     * Sets controller 'ctl' (0 to 127) to 'value' (0 to 127)
     * on 'channel' (1 to 16).
     */
    void SendControlChange(int channel, int ctl, int value);
    /**
     * Sends program change to 'pgm' (1 to 128) on 'channel' (1 to 16).
     */
    void SendProgramChange(int channel, int pgm);
    /**
     * Sets channel pressure to 'value' (0 to 127) on 'channel' (1 to 16).
     */
    void SendChannelPressure(int channel, int value);
    /**
     * Sets pitch bend to 'value' (-8192 to 8191) on 'channel' (1 to 16).
     */
    void SendPitchBend(int channel, int value);
    // -----------------------------------------------------------------
    friend class CsoundCallbackWrapper;
 protected:
    /**
     * Copies at most 'nBytes' bytes of MIDI data from the buffer to 'buf'.
     * Returns the number of bytes copied.
     */
    int GetMidiData(unsigned char *buf, int nBytes);
};

/**
 * The following class allows sending MIDI input messages to a Csound
 * instance.
 */

class PUBLIC CsoundMidiInputStream : public CsoundMidiInputBuffer {
 private:
    unsigned char   buf_[4096];
    CSOUND          *csound;
 public:
    CsoundMidiInputStream(CSOUND *csound);
    CsoundMidiInputStream(Csound *csound);
    //~CsoundMidiInputStream()
    //{
    //}
    /**
     * Enables MIDI input for the associated Csound instance.
     * Should be called between csoundPreCompile() and csoundCompile().
     * If 'argv' is not NULL, the command line arguments required for
     * MIDI input are appended.
     */
    void EnableMidiInput(CsoundArgVList *argv);
 private:
    static int midiInOpenCallback(CSOUND *, void **, const char *);
    static int midiInReadCallback(CSOUND *, void *, unsigned char *, int);
    static int midiInCloseCallback(CSOUND *, void *);
};

// ---------------------------- MIDI OUTPUT ---------------------------

class PUBLIC CsoundMidiOutputBuffer {
 private:
    unsigned char   *buf;
    void            *mutex_;
    int             bufReadPos;
    int             bufWritePos;
    int             bufBytes;
    int             bufSize;
 public:
    CsoundMidiOutputBuffer(unsigned char *buf, int bufSize);
    ~CsoundMidiOutputBuffer();
    /**
     * Pops and returns the first message from the buffer, in the following
     * format:
     *   STATUS + DATA1 * 256 + DATA2 * 65536
     * where STATUS also includes the channel number (0 to 15), if any.
     * The return value is zero if there are no messages.
     */
    int PopMessage();
    /**
     * Returns the status byte for the first message in the buffer, not
     * including the channel number in the case of channel messages.
     * The return value is zero if there are no messages.
     */
    int GetStatus();
    /**
     * Returns the channel number (1 to 16) for the first message in the
     * buffer.  The return value is zero if there are no messages, or the
     * first message is not a channel message.
     */
    int GetChannel();
    /**
     * Returns the first data byte (0 to 127) for the first message in the
     * buffer.  The return value is zero if there are no messages, or the
     * first message does not have any data bytes.
     */
    int GetData1();
    /**
     * Returns the second data byte (0 to 127) for the first message in the
     * buffer.  The return value is zero if there are no messages, or the
     * first message has less than two data bytes.
     */
    int GetData2();
    // -----------------------------------------------------------------
    friend class CsoundCallbackWrapper;
 protected:
    /**
     * Copies at most 'nBytes' bytes of MIDI data to the buffer from 'buf'.
     * Returns the number of bytes copied.
     */
    int SendMidiData(const unsigned char *buf, int nBytes);
};

/**
 * The following class allows receiving MIDI output messages
 * from a Csound instance.
 */

class PUBLIC CsoundMidiOutputStream : public CsoundMidiOutputBuffer {
 private:
    unsigned char   buf_[4096];
    CSOUND          *csound;
 public:
    CsoundMidiOutputStream(CSOUND *csound);
    CsoundMidiOutputStream(Csound *csound);
    //~CsoundMidiOutputStream()
    //{
    //}
    /**
     * Enables MIDI output for the associated Csound instance.
     * Should be called between csoundPreCompile() and csoundCompile().
     * If 'argv' is not NULL, the command line arguments required for
     * MIDI output are appended.
     */
    void EnableMidiOutput(CsoundArgVList *argv);
 private:
    static int midiOutOpenCallback(CSOUND *, void **, const char *);
    static int midiOutWriteCallback(CSOUND *, void *,
                                    const unsigned char *, int);
    static int midiOutCloseCallback(CSOUND *, void *);
};

#endif  // CSOUND_CS_GLUE_HPP

