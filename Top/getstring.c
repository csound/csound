/*
    getstring.c:

    Copyright (C) 1999 John ffitch
    Jan 27 2005: replaced with new implementation by Istvan Varga
    Dec 25 2007: added GNU gettext implementation as alternative -- John ffitch

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

#ifdef HAVE_DIRENT_H
#  include <sys/types.h>
#  include <dirent.h>
#  if 0 && defined(__MACH__)
typedef void* DIR;
DIR opendir(const char *);
struct dirent *readdir(DIR*);
int closedir(DIR*);
#  endif
#endif

#include "namedins.h"

#define CSSTRNGS_VERSION 0x2000
#include <locale.h>
#ifndef GNU_GETTEXT
void init_getstring(void)
{
}
PUBLIC char *csoundLocalizeString(const char *s)
{
    return (char*)s;
}
/* This stub is needed for backwards compatibility */
PUBLIC void csoundSetLanguage(cslanguage_t lang_code)
{
    return;
}
#else
void init_getstring(void)
{
/*     s = csoundGetEnv(NULL, "CS_LANG"); */
/*     if (s == NULL)              /\* Default locale *\/ */
/*       setlocale (LC_MESSAGES, ""); */
/*     else  */
/*       setlocale (LC_MESSAGES, s);    /\* Set to particular value *\/ */
/*    textdomain("csound5"); */  /* This is not needed when using dgettext */
    /* bind_textdomain_codeset("csound5", "UTF-8"); */
#ifdef never
    /* This is experimental; where should these be?? */
    bindtextdomain("csound5", "/home/jpff/Sourceforge/csound/csound5/po");
#endif
}

PUBLIC char *csoundLocalizeString(const char *s)
{
    return dgettext("csound5", s);
}

static const char *language_names[] = {"", /* Default */
                            "af", /* CSLANGUAGE_AFRIKAANS */
                            "sq", /* CSLANGUAGE_ALBANIAN */
                            "ar", /* CSLANGUAGE_ARABIC */
                            "hy", /* CSLANGUAGE_ARMENIAN */
                            "as", /* CSLANGUAGE_ASSAMESE */
                            "az", /* CSLANGUAGE_AZERI */
                            "eu", /* CSLANGUAGE_BASQUE */
                            "be", /* CSLANGUAGE_BELARUSIAN */
                            "bn", /* CSLANGUAGE_BENGALI */
                            "bg", /* CSLANGUAGE_BULGARIAN */
                            "ca", /* CSLANGUAGE_CATALAN */
                            "zh", /* CSLANGUAGE_CHINESE */
                            "hr", /* CSLANGUAGE_CROATIAN */
                            "cs", /* CSLANGUAGE_CZECH */
                            "da", /* CSLANGUAGE_DANISH */
                            "nl", /* CSLANGUAGE_DUTCH */
                            "en_GB",
                            "en_US",
                            "et", /* CSLANGUAGE_ESTONIAN */
                            "fo", /* CSLANGUAGE_FAEROESE */
                            "fa", /* CSLANGUAGE_FARSI */
                            "fi", /* CSLANGUAGE_FINNISH */
                            "fr",
                            "ka", /* CSLANGUAGE_GEORGIAN */
                            "de",
                            "el", /* CSLANGUAGE_GREEK */
                            "gu", /* CSLANGUAGE_GUJARATI */
                            "he", /* CSLANGUAGE_HEBREW */
                            "hi", /* CSLANGUAGE_HINDI */
                            "hu", /* CSLANGUAGE_HUNGARIAN */
                            "is", /* CSLANGUAGE_ICELANDIC */
                            "id", /* CSLANGUAGE_INDONESIAN */
                            "it", /* CSLANGUAGE_ITALIAN */
                            "ja", /* CSLANGUAGE_JAPANESE */
                            "kn", /* CSLANGUAGE_KANNADA */
                            "ks", /* CSLANGUAGE_KASHMIRI */
                            "kk", /* CSLANGUAGE_KAZAK */
                            "kok", /* CSLANGUAGE_KONKANI */
                            "ko", /* CSLANGUAGE_KOREAN */
                            "lv", /* CSLANGUAGE_LATVIAN */
                            "lt", /* CSLANGUAGE_LITHUANIAN */
                            "mk", /* CSLANGUAGE_MACEDONIAN */
                            "ms", /* CSLANGUAGE_MALAY */
                            "ml", /* CSLANGUAGE_MALAYALAM */
                            "mni", /* CSLANGUAGE_MANIPURI */
                            "mr", /* CSLANGUAGE_MARATHI */
                            "ne", /* CSLANGUAGE_NEPALI */
                            "no", /* CSLANGUAGE_NORWEGIAN */
                            "or", /* CSLANGUAGE_ORIYA */
                            "pl", /* CSLANGUAGE_POLISH */
                            "pt", /* CSLANGUAGE_PORTUGUESE */
                            "pa", /* CSLANGUAGE_PUNJABI */
                            "ro",
                            "ru", /* CSLANGUAGE_RUSSIAN */
                            "sa", /* CSLANGUAGE_SANSKRIT */
                            "sr", /* CSLANGUAGE_SERBIAN */
                            "sd", /* CSLANGUAGE_SINDHI */
                            "sk", /* CSLANGUAGE_SLOVAK */
                            "sl", /* CSLANGUAGE_SLOVENIAN */
                            "es",
                            "sw", /* CSLANGUAGE_SWAHILI */
                            "sv", /* CSLANGUAGE_SWEDISH */
                            "ta", /* CSLANGUAGE_TAMIL */
                            "tt", /* CSLANGUAGE_TATAR */
                            "te", /* CSLANGUAGE_TELUGU */
                            "th", /* CSLANGUAGE_THAI */
                            "tr", /* CSLANGUAGE_TURKISH */
                            "uk", /* CSLANGUAGE_UKRAINIAN */
                            "ur", /* CSLANGUAGE_URDU */
                            "uz", /* CSLANGUAGE_UZBEK */
                            "vi", /* CSLANGUAGE_VIETNAMES */
                            "es_CO", /* COLUMBIAN */
  };

PUBLIC void csoundSetLanguage(cslanguage_t lang_code)
{
    const char *name;
    if (lang_code == CSLANGUAGE_DEFAULT)
      fprintf(stderr, "Localisation of messages is disabled, using "
                      "default language.\n");
    else {
      fprintf(stderr, "Setting language of messages to %s ...\n",
                      name=language_names[(int) lang_code]);
      setlocale(LC_MESSAGES, name);
    }
    return;
}
#endif

