/*
    text.h:

    Copyright (C) 1999 John ffitch
    Jan 27 2005: Replaced with new implementation by Istvan Varga

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

#ifndef CSOUND_TEXT_H
#define CSOUND_TEXT_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Deal with localisation of mesages */

#define Str(x)  csoundLocalizeString(x)

/* list of languages */

typedef enum {
    CSLANGUAGE_DEFAULT = 0,
    CSLANGUAGE_AFRIKAANS,
    CSLANGUAGE_ALBANIAN,
    CSLANGUAGE_ARABIC,
    CSLANGUAGE_ARMENIAN,
    CSLANGUAGE_ASSAMESE,
    CSLANGUAGE_AZERI,
    CSLANGUAGE_BASQUE,
    CSLANGUAGE_BELARUSIAN,
    CSLANGUAGE_BENGALI,
    CSLANGUAGE_BULGARIAN,
    CSLANGUAGE_CATALAN,
    CSLANGUAGE_CHINESE,
    CSLANGUAGE_CROATIAN,
    CSLANGUAGE_CZECH,
    CSLANGUAGE_DANISH,
    CSLANGUAGE_DUTCH,
    CSLANGUAGE_ENGLISH_UK,
    CSLANGUAGE_ENGLISH_US,
    CSLANGUAGE_ESTONIAN,
    CSLANGUAGE_FAEROESE,
    CSLANGUAGE_FARSI,
    CSLANGUAGE_FINNISH,
    CSLANGUAGE_FRENCH,
    CSLANGUAGE_GEORGIAN,
    CSLANGUAGE_GERMAN,
    CSLANGUAGE_GREEK,
    CSLANGUAGE_GUJARATI,
    CSLANGUAGE_HEBREW,
    CSLANGUAGE_HINDI,
    CSLANGUAGE_HUNGARIAN,
    CSLANGUAGE_ICELANDIC,
    CSLANGUAGE_INDONESIAN,
    CSLANGUAGE_ITALIAN,
    CSLANGUAGE_JAPANESE,
    CSLANGUAGE_KANNADA,
    CSLANGUAGE_KASHMIRI,
    CSLANGUAGE_KAZAK,
    CSLANGUAGE_KONKANI,
    CSLANGUAGE_KOREAN,
    CSLANGUAGE_LATVIAN,
    CSLANGUAGE_LITHUANIAN,
    CSLANGUAGE_MACEDONIAN,
    CSLANGUAGE_MALAY,
    CSLANGUAGE_MALAYALAM,
    CSLANGUAGE_MANIPURI,
    CSLANGUAGE_MARATHI,
    CSLANGUAGE_NEPALI,
    CSLANGUAGE_NORWEGIAN,
    CSLANGUAGE_ORIYA,
    CSLANGUAGE_POLISH,
    CSLANGUAGE_PORTUGUESE,
    CSLANGUAGE_PUNJABI,
    CSLANGUAGE_ROMANIAN,
    CSLANGUAGE_RUSSIAN,
    CSLANGUAGE_SANSKRIT,
    CSLANGUAGE_SERBIAN,
    CSLANGUAGE_SINDHI,
    CSLANGUAGE_SLOVAK,
    CSLANGUAGE_SLOVENIAN,
    CSLANGUAGE_SPANISH,
    CSLANGUAGE_SWAHILI,
    CSLANGUAGE_SWEDISH,
    CSLANGUAGE_TAMIL,
    CSLANGUAGE_TATAR,
    CSLANGUAGE_TELUGU,
    CSLANGUAGE_THAI,
    CSLANGUAGE_TURKISH,
    CSLANGUAGE_UKRAINIAN,
    CSLANGUAGE_URDU,
    CSLANGUAGE_UZBEK,
    CSLANGUAGE_VIETNAMESE
} cslanguage_t;

/* NOTE: function prototypes are in csound.h */

#include "csound.h"

/*
 * Set language to 'lang_code' (lang_code can be for example
 * CSLANGUAGE_ENGLISH_UK or CSLANGUAGE_FRENCH or many others,
 * see n_getstr.h for the list of languages). This affects all
 * Csound instances running in the address space of the current
 * process. The special language code CSLANGUAGE_DEFAULT can be
 * used to disable translation of messages and free all memory
 * allocated by a previous call to csoundSetLanguage().
 * csoundSetLanguage() loads all files for the selected language
 * from the directory specified by the CSSTRNGS environment
 * variable.
 */
/* void csoundSetLanguage(cslanguage_t lang_code); */

/*
 * Translate string 's' to the current language, and return
 * pointer to the translated message. This may be the same as
 * 's' if language was set to CSLANGUAGE_DEFAULT.
 */
/* char *csoundLocalizeString(const char *s); */

void init_getstring(int argc, char **argv);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif          /* CSOUND_TEXT_H */

