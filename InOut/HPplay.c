/*  
    HPplay.c:

    Copyright (C) 1991 Barry Vercoe

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

/*                                                               HPPLAY.C   */

/*  This module included only when -DHP is defined.  Called from RTAUDIO.C to
    provide open-write-close functions to the audio device-driver on the HP 735.
    Read (record) function not yet supported.
 */

#include <stdio.h>
/* #include <stdlib.h> */
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <audio/Alib.h>
#include <audio/CUlib.h>

static  long            status;
static  SSPlayParams    streamParams;
static  AGainEntry      gainEntry[4];
static  ATransID        xid;
static  char            server[80], afile[80];
static  AFileFormat     switchFormat;
static  Audio           *audio;
static  char            *pSpeaker;
static  AudioAttributes Attribs, PlayAttribs;
static  AudioAttrMask   AttribsMask, PlayAttribsMask;
static  SStream         audioStream;
static  int             fd, len;
static  FILE            *pfile;
static  int             streamSocket;
static  char            *buf, *bufBase;
static  Bool            audioPaused;
static  int             useIntSpeaker;
static  long            seekOffset, data_length, pauseCount;
static  AByteOrder      byte_order, play_byte_order;

void AWopen(int nchanls, long srate, int oMaxLag)       /* open for audio output */
{
    server[0] = '\0';
    /*  loopCount = 1; */

    streamParams.priority = APriorityUrgent;          /* urgent priority */
    /*  streamParams.priority = APriorityNormal;   */ /* normal priority */

    /*  filenameFound = False;     */
    /*  switchFormat = AFFUnknown; */

    switchFormat = AFFRawLin16;
    AttribsMask = 0;
    PlayAttribsMask = 0;
    seekOffset = 0;

    /* user specified source sampling rate */
    Attribs.attr.sampled_attr.sampling_rate = srate;      /* or atoi("44100") */
    AttribsMask = (AttribsMask | ASSamplingRateMask);

    /* user specified play sampling rate */
    PlayAttribs.attr.sampled_attr.sampling_rate =  srate; /* or atoi("44100") */
    PlayAttribsMask = (PlayAttribsMask | ASSamplingRateMask);

    /* user specified stereo */
    Attribs.attr.sampled_attr.channels = nchanls;
    AttribsMask = (AttribsMask | ASChannelsMask);

    /* open audio connection  */
    audio = AOpenAudio( server, NULL );

    /* open the audio file (if any) */
    /* no filename specified, read audio stream from stdin */
    /*   fd = fileno( stdin ); */
    pfile = NULL;

    /* Determine attributes of source */
    AChooseSourceAttributes(audio, afile, pfile, switchFormat, AttribsMask,
                            &Attribs,&seekOffset,&data_length,&byte_order,NULL);

    /* Select attributes for playback */
    AChoosePlayAttributes (audio, &Attribs, PlayAttribsMask, &PlayAttribs,
                           &play_byte_order, NULL );

    /* setup the playback parameters */
    pSpeaker = getenv( "SPEAKER" );         /* get user speaker preference */
    if (pSpeaker)
      useIntSpeaker = ( (*pSpeaker == 'i') || (*pSpeaker == 'I') );
    else
      /* SPEAKER environment variable not found - use internal speaker */
      useIntSpeaker = 1;

    switch(PlayAttribs.attr.sampled_attr.channels ) {
    case 1:
      gainEntry[0].u.o.out_ch = AOCTMono;
      gainEntry[0].gain = AUnityGain;
      gainEntry[0].u.o.out_dst = (useIntSpeaker) ? AODTMonoIntSpeaker :
                                                   AODTMonoJack;
      break;
    case 2:
    default:    /* assume no more than 2 channels */
      gainEntry[0].u.o.out_ch = AOCTLeft;
      gainEntry[0].gain = AUnityGain;
      gainEntry[0].u.o.out_dst = (useIntSpeaker) ? AODTLeftIntSpeaker :
                                                   AODTLeftJack;
      gainEntry[1].u.o.out_ch = AOCTRight;
      gainEntry[1].gain = AUnityGain;
      gainEntry[1].u.o.out_dst = (useIntSpeaker) ? AODTRightIntSpeaker :
                                                   AODTRightJack;
      break;
    }
    streamParams.gain_matrix.type = AGMTOutput;       /* gain matrix */
    streamParams.gain_matrix.num_entries = PlayAttribs.attr.sampled_attr.channels;
    streamParams.gain_matrix.gain_entries = gainEntry;
    streamParams.play_volume = AUnityGain;            /* play volume */
    /*  streamParams.priority = APriorityNormal;    */      /* normal priority */
    streamParams.event_mask = 0;                    /* don't solicit any events */
    /* create an audio stream  */
    xid = APlaySStream( audio, ~0, &PlayAttribs,
                        &streamParams, &audioStream, NULL);
    /* create a stream socket */
    streamSocket = socket( AF_INET, SOCK_STREAM, 0 );
    if (streamSocket < 0 ) {
      perror( Str(X_465,"Socket creation failed") );
      exit(1);
    }

    /* connect the stream socket to the audio stream port */
    status = connect( streamSocket, (struct sockaddr *)&audioStream.tcp_sockaddr,
                      sizeof(struct sockaddr_in) );
    if (status < 0) {
      perror( Str(X_226,"Connect failed") );
      exit(1);
    }
    bufBase = (char *) malloc( audio->block_size );

  /* start stream paused so we can transfer enough data (2 second worth) before
     playing starts to prevent stream from running out */
    APauseAudio( audio, xid, NULL, NULL );
    pauseCount = 2 * PlayAttribs.attr.sampled_attr.channels
                   * PlayAttribs.attr.sampled_attr.sampling_rate
                   * (PlayAttribs.attr.sampled_attr.bits_per_sample >> 3);
    audioPaused = True;
    buf = bufBase;
    len = audio->block_size;

    if (oMaxLag < audio->block_size &&
        (O.msglevel & WARNMSG)) {
      printf(Str(X_98,"WARNING: -B %d probably too small, suggest %d\n"),
              oMaxLag, audio->block_size);
    }
}

void AudioWrite(char *outbuf, int nbytes)         /* put samples to DAC  */
{
    if (write(streamSocket, outbuf, nbytes) < nbytes)
      printf(Str(X_293,"HP write to streamSocket: "
                       "could not write all bytes requested\n"));
    if (audioPaused) {
      pauseCount -= nbytes;
      if (pauseCount <= 0) {
        AResumeAudio( audio, xid, NULL, NULL );
        audioPaused = False;
      }
    }
}

void AWclose()                            /* close the I/O device entirely  */
{                                         /* called only when both complete */
    if (audioPaused) {
      AResumeAudio( audio, xid, NULL, NULL );
      audioPaused = False;
    }
    close( streamSocket );
/* set close mode to prevent playback from stopping when we close audio connection */
    ASetCloseDownMode( audio, AKeepTransactions, NULL );
    /* now can close */
    ACloseAudio( audio, NULL );
    free(bufBase);
}


