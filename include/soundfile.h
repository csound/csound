/*
    soundfile.h:  Soundile IO interface

    Copyright (C) 2021 V Lazzarini

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

#ifndef _SOUNDFILE_H_
#define _SOUNDFILE_H_
#include "sysdep.h"

#ifdef USE_LIBSNDFILE
#include <sndfile.h>

#define SFLIB_FALSE SF_FALSE
#define SFLIB_TRUE SF_TRUE
#define SFLIB_INSTRUMENT SF_INSTRUMENT

#ifndef SNDFILE_MP3
// set missing tokens
#define SF_FORMAT_MPEG 0x230000		/* MPEG-1/2 audio stream */
#define	SF_FORMAT_MPEG_LAYER_I	0x0080		/* MPEG-1 Audio Layer I */
#define SF_FORMAT_MPEG_LAYER_II	0x0081,		/* MPEG-1 Audio Layer II */
#define SF_FORMAT_MPEG_LAYER_III 0x0082		/* MPEG-2 Audio Layer III */
#endif


/* standard audio encoding types */
#define AE_CHAR         SF_FORMAT_PCM_S8
#define AE_SHORT        SF_FORMAT_PCM_16
#define AE_24INT        SF_FORMAT_PCM_24
#define AE_LONG         SF_FORMAT_PCM_32
#define AE_UNCH         SF_FORMAT_PCM_U8
#define AE_FLOAT        SF_FORMAT_FLOAT
#define AE_DOUBLE       SF_FORMAT_DOUBLE
#define AE_ULAW         SF_FORMAT_ULAW
#define AE_ALAW         SF_FORMAT_ALAW
#define AE_IMA_ADPCM    SF_FORMAT_IMA_ADPCM
#define AE_MS_ADPCM     SF_FORMAT_MS_ADPCM
#define AE_GSM610       SF_FORMAT_GSM610
#define AE_VOX          SF_FORMAT_VOX_ADPCM
#define AE_G721_32      SF_FORMAT_G721_32
#define AE_G723_24      SF_FORMAT_G723_24
#define AE_G723_40      SF_FORMAT_G723_40
#define AE_DWVW_12      SF_FORMAT_DWVW_12
#define AE_DWVW_16      SF_FORMAT_DWVW_16
#define AE_DWVW_24      SF_FORMAT_DWVW_24
#define AE_DWVW_N       SF_FORMAT_DWVW_N
#define AE_DPCM_8       SF_FORMAT_DPCM_8
#define AE_DPCM_16      SF_FORMAT_DPCM_16
#define AE_VORBIS       SF_FORMAT_VORBIS
#define AE_MPEG         SF_FORMAT_MPEG | SF_FORMAT_MPEG_LAYER_III

#define AE_LAST   SF_FORMAT_DPCM_16     /* current last audio encoding value */

/* file types */
#define TYP_WAV   (SF_FORMAT_WAV >> 16)
#define TYP_AIFF  (SF_FORMAT_AIFF >> 16)
#define TYP_AU    (SF_FORMAT_AU >> 16)
#define TYP_RAW   (SF_FORMAT_RAW >> 16)
#define TYP_PAF   (SF_FORMAT_PAF >> 16)
#define TYP_SVX   (SF_FORMAT_SVX >> 16)
#define TYP_NIST  (SF_FORMAT_NIST >> 16)
#define TYP_VOC   (SF_FORMAT_VOC >> 16)
#define TYP_IRCAM (SF_FORMAT_IRCAM >> 16)
#define TYP_W64   (SF_FORMAT_W64 >> 16)
#define TYP_MAT4  (SF_FORMAT_MAT4 >> 16)
#define TYP_MAT5  (SF_FORMAT_MAT5 >> 16)
#define TYP_PVF   (SF_FORMAT_PVF >> 16)
#define TYP_XI    (SF_FORMAT_XI >> 16)
#define TYP_HTK   (SF_FORMAT_HTK >> 16)
#define TYP_SDS   (SF_FORMAT_SDS >> 16)
#define TYP_AVR   (SF_FORMAT_AVR >> 16)
#define TYP_WAVEX (SF_FORMAT_WAVEX >> 16)
#define TYP_SD2   (SF_FORMAT_SD2 >> 16)
#define TYP_FLAC  (SF_FORMAT_FLAC >> 16)
#define TYP_CAF   (SF_FORMAT_CAF >> 16)
#define TYP_WVE   (SF_FORMAT_WVE >> 16)
#define TYP_OGG   (SF_FORMAT_OGG >> 16)
#define TYP_MPEG  (SF_FORMAT_MPEG >> 16)
#define TYP_MPC2K (SF_FORMAT_MPC2K >> 16)
#define TYP_RF64  (SF_FORMAT_RF64 >> 16)

#define FORMAT2SF(x) ((int32_t) (x))
#define SF2FORMAT(x) ((int32_t) (x) & 0xFFFF)
#define TYPE2SF(x)   ((int32_t) (x) << 16)
#define TYP2SF(x)    ((int32_t) (x) << 16)
#define SF2TYPE(x)   ((int32_t) (x & SF_FORMAT_TYPEMASK) >> 16)
#define TYPE2ENC(x)   ((int32_t) (x & SF_FORMAT_SUBMASK))
#define ENDIANESSBITS (SF_FORMAT_TYPEMASK | SF_FORMAT_SUBMASK)
  
#else
#include <stddef.h>

#define AE_CHAR         10
#define AE_SHORT        20
#define AE_24INT        30
#define AE_LONG         40
#define AE_UNCH         50
#define AE_FLOAT        60
#define AE_DOUBLE       70
#define AE_ULAW         80
#define AE_ALAW         90
#define AE_IMA_ADPCM    100
#define AE_MS_ADPCM     110
#define AE_GSM610       120
#define AE_VOX          130
#define AE_G721_32      140
#define AE_G723_24      150
#define AE_G723_40      160
#define AE_DWVW_12      170
#define AE_DWVW_16      180
#define AE_DWVW_24      190
#define AE_DWVW_N       200
#define AE_DPCM_8       210
#define AE_DPCM_16      220
#define AE_VORBIS       230
#define AE_MPEG         240

#define AE_LAST   SF_FORMAT_DPCM_16     /* current last audio encoding value */

/* file types */
#define TYP_WAV   (0)
#define TYP_AIFF  (1)
#define TYP_AU    (2)
#define TYP_RAW   (3)
#define TYP_PAF   (4)
#define TYP_SVX   (5)
#define TYP_NIST  (6)
#define TYP_VOC   (7)
#define TYP_IRCAM (8)
#define TYP_W64   (9)
#define TYP_MAT4  (10)
#define TYP_MAT5  (11)
#define TYP_PVF   (12)
#define TYP_XI    (13)
#define TYP_HTK   (14)
#define TYP_SDS   (15)
#define TYP_AVR   (16)
#define TYP_WAVEX (17)
#define TYP_SD2   (18)
#define TYP_FLAC  (19)
#define TYP_CAF   (20)
#define TYP_WVE   (21)
#define TYP_OGG   (22)
#define TYP_MPC2K (23)
#define TYP_RF64  (24)
#define TYP_MPEG  (25)

#define FORMAT2SF(x) ((int32_t) (x))
#define SF2FORMAT(x) ((int32_t) (x))
#define TYPE2SF(x)   ((int32_t) (x))
#define TYP2SF(x)    ((int32_t) (x))
#define SF2TYPE(x)   ((int32_t) (x))
#define TYPE2ENC(x)   ((int32_t) (x))
#define ENDIANESSBITS 0

#define SNDFILE void
#define SFLIB_FALSE 0
#define SFLIB_TRUE 1
#define SFC_SET_CLIPPING 0
#define SFC_SET_VBR_ENCODING_QUALITY 0
#define SFC_SET_ADD_PEAK_CHUNK 0
#define SFC_SET_DITHER_ON_WRITE 0
#define SFC_SET_NORM_DOUBLE  0
#define SFC_SET_NORM_FLOAT  0
#define SFC_GET_SIGNAL_MAX 0
#define SFC_CALC_NORM_SIGNAL_MAX 0
#define SFC_GET_MAX_ALL_CHANNELS 0
#define SFC_CALC_NORM_MAX_ALL_CHANNELS 0
#define SFC_SET_UPDATE_HEADER_AUTO 0
#define SFC_SET_ADD_PEAK_CHUNK 0
#define SFC_UPDATE_HEADER_NOW 0
#define SFC_GET_INSTRUMENT 0

enum
  {   SF_STR_TITLE,
      SF_STR_COPYRIGHT,
      SF_STR_SOFTWARE,
      SF_STR_ARTIST,
      SF_STR_COMMENT,
      SF_STR_DATE,
      SF_STR_ALBUM,
      SF_STR_LICENSE,
      SF_STR_TRACKNUMBER,
      SF_STR_GENRE
  } ;


enum { SFM_READ = 0, SFM_WRITE};

#define SFCLIB_GET_INSTRUMENT 0
enum
  {    
    SF_LOOP_NONE = 800,
    SF_LOOP_FORWARD,
    SF_LOOP_BACKWARD,
    SF_LOOP_ALTERNATING
  } ;

typedef struct
{
  int32_t gain ;
  char basenote, detune ;
  char velocity_lo, velocity_hi ;
  char key_lo, key_hi ;
  int32_t loop_count ;

  struct
  {
    int32_t mode ;
    uint32_t start ;
    uint32_t end ;
    uint32_t count ;
  } loops [16] ; 
} SFLIB_INSTRUMENT ;

typedef long sf_count_t;
#endif // USE_LIBSNDFILE  
#endif /* _SOUNDFILE_H_ */
