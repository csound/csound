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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#if !defined(_SFTYPE_H)
#define _SFTYPE_H
#ifdef          __GNUC__
#  ifndef       PACKED
#    define     PACKED  __attribute__((packed))
#  endif        /* PACKED */
#elif defined MSVC
#  pragma       pack(push, before, 1)
#  define     PACKED
#else
# error "No pack defined."
#endif

#ifndef WORDS_BIGENDIAN
#  if defined(__POWERPC__) || defined(__PPC__) || defined(__ppc__)
#    define WORDS_BIGENDIAN 1
#  endif
#endif

#if defined(WORDS_BIGENDIAN) && defined(__i386__)
#  undef WORDS_BIGENDIAN
#endif

#if !defined(WIN32) || defined(__CYGWIN__)
typedef uint32_t    DWORD;
#endif

/*  typedef int32_t     BOOL; */
typedef uint8_t     BYTE;
typedef uint16_t    WORD;
typedef short       SHORT;
typedef char        CHAR;
typedef int8_t      SBYTE;

typedef uint16_t SFTransform;

typedef struct
{
#ifdef WORDS_BIGENDIAN
  /* bigendian m/c like Mac and SGI */
        BYTE byHi;
        BYTE byLo;
#else
        BYTE byLo;
        BYTE byHi;
#endif
} PACKED rangesType;

typedef union
{
  rangesType ranges;
  SHORT shAmount;
  WORD wAmount;
} PACKED genAmountType;

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
        WORD wMajor;
        WORD wMinor;
} PACKED sfVersionTag;

/* <phdr-rec>   ->       */
typedef struct
{
        CHAR achPresetName[20];
        WORD wPreset;
        WORD wBank;
        WORD wPresetBagNdx;
        DWORD dwLibrary;
        DWORD dwGenre;
        DWORD dwMorphology;
} PACKED sfPresetHeader;

/* <pbag-rec>   ->       */
typedef struct
        {
        WORD wGenNdx;
        WORD wModNdx;
} PACKED sfPresetBag;

/* <pmod-rec> ->         */
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
        WORD sfModSrcOper;
  /* SFGenerator sfModDestOper; */
        WORD sfModDestOper;
        SHORT modAmount;
  /* SFModulator sfModAmtSrcOper; */
        WORD sfModAmtSrcOper;
        SFTransform sfModTransOper;

} PACKED sfModList;

/* <pgen-rec>   ->       */
typedef struct
        {
          /*SFGenerator sfGenOper; */
        WORD sfGenOper;
        genAmountType genAmount;
} PACKED sfGenList;

/* <inst-rec>   ->       */
typedef struct
{
        CHAR achInstName[20];
        WORD wInstBagNdx;
} PACKED sfInst;

/* <ibag-rec>   ->       */
typedef struct
{
        WORD wInstGenNdx;
        WORD wInstModNdx;
} PACKED sfInstBag;

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
        WORD sfModSrcOper;
  /* SFGenerator sfModDestOper; */
        WORD sfModDestOper;
        SHORT modAmount;
  /* SFModulator sfModAmtSrcOper; */
        WORD sfModAmtSrcOper;
        SFTransform sfModTransOper;
} PACKED sfInstModList;

/* <igen-rec>   ->       */
typedef struct
{
  /* SFGenerator sfGenOper; */
        WORD sfGenOper;
        genAmountType genAmount;
} PACKED sfInstGenList;

/* <shdr-rec>   ->       */
typedef struct
{
        CHAR achSampleName[20];
        DWORD dwStart;
        DWORD dwEnd;
        DWORD dwStartloop;
        DWORD dwEndloop;
        DWORD dwSampleRate;
        BYTE byOriginalKey;
        CHAR chCorrection;
        WORD wSampleLink;
  /*SFSampleLink sfSampleType; */
        WORD sfSampleType;
} PACKED sfSample;

#ifdef          MSVC
#  pragma       pack(pop, before)
#endif

#endif
