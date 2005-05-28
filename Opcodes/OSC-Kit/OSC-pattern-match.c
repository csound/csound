/*
Copyright (c) 1998.  The Regents of the University of California (Regents).
All Rights Reserved.

Permission to use, copy, modify, and distribute this software and its
documentation for educational, research, and not-for-profit purposes, without
fee and without a signed licensing agreement, is hereby granted, provided that
the above copyright notice, this paragraph and the following two paragraphs
appear in all copies, modifications, and distributions.  Contact The Office of
Technology Licensing, UC Berkeley, 2150 Shattuck Avenue, Suite 510, Berkeley,
CA 94720-1620, (510) 643-7201, for commercial licensing opportunities.

Written by Matt Wright, The Center for New Music and Audio Technologies,
University of California, Berkeley.

     IN NO EVENT SHALL REGENTS BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT,
     SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS,
     ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF
     REGENTS HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

     REGENTS SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT
     LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
     FOR A PARTICULAR PURPOSE. THE SOFTWARE AND ACCOMPANYING
     DOCUMENTATION, IF ANY, PROVIDED HEREUNDER IS PROVIDED "AS IS".
     REGENTS HAS NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES,
     ENHANCEMENTS, OR MODIFICATIONS.

The OpenSound Control WWW page is
    http://www.cnmat.berkeley.edu/OpenSoundControl
*/

/*
    OSC-pattern-match.c
    Matt Wright, 3/16/98
    Adapted from oscpattern.c, by Matt Wright and Amar Chaudhury
 */

#include "OSC-common.h"
#include "OSC-pattern-match.h"

static const char *theWholePattern;     /* Just for warning messages */

static OSCBoolean MatchBrackets (const char *pattern, const char *test);
static OSCBoolean MatchList (const char *pattern, const char *test);

OSCBoolean PatternMatch (const char *  pattern, const char * test) {
  theWholePattern = pattern;

  if (pattern == 0 || pattern[0] == 0) {
    return test[0] == 0;
  }

  if (test[0] == 0) {
    if (pattern[0] == '*')
      return PatternMatch (pattern+1,test);
    else
      return FALSE;
  }

  switch (pattern[0]) {
    case 0      : return test[0] == 0;
    case '?'    : return PatternMatch (pattern + 1, test + 1);
    case '*'    :
      if (PatternMatch (pattern+1, test)) {
        return TRUE;
      } else {
        return PatternMatch (pattern, test+1);
      }
    case ']'    :
    case '}'    :
      OSCWarning("Spurious %c in pattern \".../%s/...\"",pattern[0], theWholePattern);
      return FALSE;
    case '['    :
      return MatchBrackets (pattern,test);
    case '{'    :
      return MatchList (pattern,test);
    case '\\'   :
      if (pattern[1] == 0) {
        return test[0] == 0;
      } else if (pattern[1] == test[0]) {
        return PatternMatch (pattern+2,test+1);
      } else {
        return FALSE;
      }
    default     :
      if (pattern[0] == test[0]) {
        return PatternMatch (pattern+1,test+1);
      } else {
        return FALSE;
      }
  }
}

/* we know that pattern[0] == '[' and test[0] != 0 */

static OSCBoolean MatchBrackets (const char *pattern, const char *test) {
  OSCBoolean result;
  OSCBoolean negated = FALSE;
  const char *p = pattern;

  if (pattern[1] == 0) {
    OSCWarning("Unterminated [ in pattern \".../%s/...\"", theWholePattern);
    return FALSE;
  }

  if (pattern[1] == '!') {
    negated = TRUE;
    p++;
  }

  while (*p != ']') {
    if (*p == 0) {
      OSCWarning("Unterminated [ in pattern \".../%s/...\"", theWholePattern);
      return FALSE;
    }
    if (p[1] == '-' && p[2] != 0) {
      if (test[0] >= p[0] && test[0] <= p[2]) {
        result = !negated;
        goto advance;
      }
    }
    if (p[0] == test[0]) {
      result = !negated;
      goto advance;
    }
    p++;
  }

  result = negated;

advance:

  if (!result)
    return FALSE;

  while (*p != ']') {
    if (*p == 0) {
      OSCWarning("Unterminated [ in pattern \".../%s/...\"", theWholePattern);
      return FALSE;
    }
    p++;
  }

  return PatternMatch (p+1,test+1);
}

static OSCBoolean MatchList (const char *pattern, const char *test) {

 const char *restOfPattern, *tp = test;

 for(restOfPattern = pattern; *restOfPattern != '}'; restOfPattern++) {
   if (*restOfPattern == 0) {
     OSCWarning("Unterminated { in pattern \".../%s/...\"", theWholePattern);
     return FALSE;
   }
 }

 restOfPattern++; /* skip close curly brace */

 pattern++; /* skip open curly brace */

 while (1) {

   if (*pattern == ',') {
     if (PatternMatch (restOfPattern, tp)) {
       return TRUE;
     } else {
       tp = test;
       ++pattern;
     }
   } else if (*pattern == '}') {
     return PatternMatch (restOfPattern, tp);
   } else if (*pattern == *tp) {
     ++pattern;
     ++tp;
   } else {
     tp = test;
     while (*pattern != ',' && *pattern != '}') {
       pattern++;
     }
     if (*pattern == ',') {
       pattern++;
     }
   }
 }

}

