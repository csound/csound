/*
    sftype.h:

    Copyright (C) 2000 Gabriel Maldonado

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

#include "csoundCore.h"

#if !defined(_SFTYPE_H)
#ifdef          __GNUC__
#  ifndef       PACKED
#    define     PACKED  __attribute__((packed))
#  endif        /* PACKED */
#elif defined MSVC
#  pragma       pack(push, before, 1)
#  define     PACKED
#elif defined(mac_classic)
#  pragma               pack(1)
#  define     PACKED
#elif defined(__WATCOMC__)
#  include <pshpack1.h>
#  define     PACKED
#else
# error No pack defined
#endif

typedef unsigned long       DWORD;
/*  typedef int                 BOOL; */
typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef short SHORT;
typedef char CHAR;

typedef unsigned short SFTransform;

typedef struct
{
#ifdef WORDS_BIGENDIAN
  /* bigendian m/c like Mac and SGI */
        BYTE byHi  PACKED;
        BYTE byLo  PACKED;
#else
        BYTE byLo  PACKED;
        BYTE byHi  PACKED;
#endif
} rangesType;

typedef union
{
        rangesType ranges       PACKED;
        SHORT shAmount          PACKED;
        WORD wAmount            PACKED;
} genAmountType;

/* The SFSampleLink is an enumeration type which
   describes both the type of sample (mono, stereo left, etc.)
   and the whether the sample is located in RAM or ROM memory: */

typedef enum
{
        monoSample = 1,
        rightSample = 2,
        leftSample = 4,
        linkedSample = 8,
        ROMSample =     0x8000,  /*32768 */
        RomMonoSample = 0x8001,  /*32769 */
        RomRightSample = 0x8002,  /*32770 */
        RomLeftSample = 0x8004,   /*32772 */
        RomLinkedSample = 0x8008   /*32776 */
} SFSampleLink;

 /* The INFO-list chunk in a SoundFont 2 compatible file contains
  three mandatory and a variety of optional subchunks as defined below.
  The INFO-list chunk gives basic information about the SoundFont
  compatible bank contained in the file.
 */

/* The ifil subchunk is a mandatory subchunk identifying the SoundFont
  specification version level to which the file complies.
  It is always four bytes in length, and contains data
  according to the structure:
  */

/* <iver-rec>   ->       */
typedef struct
{
        WORD wMajor     PACKED;
        WORD wMinor     PACKED;
} sfVersionTag;

/* <phdr-rec>   ->       */
typedef struct
{
        CHAR achPresetName[20]  PACKED;
        WORD wPreset            PACKED;
        WORD wBank              PACKED;
        WORD wPresetBagNdx      PACKED;
        DWORD dwLibrary         PACKED;
        DWORD dwGenre           PACKED;
        DWORD dwMorphology      PACKED;
} sfPresetHeader;

/* <pbag-rec>   ->       */
typedef struct
        {
        WORD wGenNdx    PACKED;
        WORD wModNdx    PACKED;
} sfPresetBag;

/* <pmod-rec> ->         */
typedef struct
{
        /*
        SFModulator sfModSrcOper        PACKED;
        SFGenerator sfModDestOper       PACKED;
        SHORT modAmount                 PACKED;
        SFModulator sfModAmtSrcOper     PACKED;
        SFTransform sfModTransOper      PACKED;
        */

  /* SFModulator sfModSrcOper; */
        WORD sfModSrcOper       PACKED;
  /* SFGenerator sfModDestOper; */
        WORD sfModDestOper      PACKED;
        SHORT modAmount         PACKED;
  /* SFModulator sfModAmtSrcOper; */
        WORD sfModAmtSrcOper    PACKED;
        SFTransform sfModTransOper      PACKED;

} sfModList;

/* <pgen-rec>   ->       */
typedef struct
        {
          /*SFGenerator sfGenOper; */
        WORD sfGenOper          PACKED;
        genAmountType genAmount PACKED;
} sfGenList;

/* <inst-rec>   ->       */
typedef struct
{
        CHAR achInstName[20]    PACKED;
        WORD wInstBagNdx        PACKED;
} sfInst;

/* <ibag-rec>   ->       */
typedef struct
{
        WORD wInstGenNdx        PACKED;
        WORD wInstModNdx        PACKED;
} sfInstBag;

/* <imod-rec> ->         */
typedef struct
{

        /*
        SFModulator sfModSrcOper;
        SFGenerator sfModDestOper;
        SHORT modAmount;
        SFModulator sfModAmtSrcOper;
        SFTransform sfModTransOper;
        */

  /* SFModulator sfModSrcOper; */
        WORD sfModSrcOper       PACKED;
  /* SFGenerator sfModDestOper; */
        WORD sfModDestOper      PACKED;
        SHORT modAmount         PACKED;
  /* SFModulator sfModAmtSrcOper; */
        WORD sfModAmtSrcOper    PACKED;
        SFTransform sfModTransOper      PACKED;
} sfInstModList;

/* <igen-rec>   ->       */
typedef struct
{
  /* SFGenerator sfGenOper; */
        WORD sfGenOper          PACKED;
        genAmountType genAmount PACKED;
} sfInstGenList;

/* <shdr-rec>   ->       */
typedef struct
{
        CHAR achSampleName[20]  PACKED;
        DWORD dwStart           PACKED;
        DWORD dwEnd             PACKED;
        DWORD dwStartloop       PACKED;
        DWORD dwEndloop         PACKED;
        DWORD dwSampleRate      PACKED;
        BYTE byOriginalKey      PACKED;
        CHAR chCorrection       PACKED;
        WORD wSampleLink        PACKED;
  /*SFSampleLink sfSampleType; */
        WORD sfSampleType       PACKED;
} sfSample;

#define _SFTYPE_H
#ifdef          MSVC
#  pragma       pack(pop, before)
#endif
#if defined(mac_classic)
#  pragma               pack(0)
#elif defined(__WATCOMC__)
#include <poppack.h>
#endif

#endif
