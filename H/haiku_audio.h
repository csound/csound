/*
  haiku_audio.h:

  Haiku Audio Interface
  -- Caution -- things may change...

  Copyright (C) 2012 Peter J. Goodeve

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


#ifndef _HAIKU_AUDIO
#define _HAIKU_AUDIO


class Generator {
 public:
        Generator(float sampleRate, int nchans, size_t bufferSize,
                  int32 sampleSize);
;       ~Generator();
        int RunAudio();

        size_t mBufSize;      // in bytes (= samples * chans * floatsize)
        float mFrameRate;
        int mChans;
        int32 mSampleSize;
        double *mDataBuf;     // filled by rtplay_, cleared afer copy
        size_t mXferSize;     // actual source size in bytes (may be less than full)
        sem_id cs_sem;        // to be waited on by Csound

private:
        struct Generator_Private * state;
};

#endif
