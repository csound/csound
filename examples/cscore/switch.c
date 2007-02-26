/*  
    switch.c: for cscore

    Copyright (C) 1991 Barry Vercoe

    This file is part of Csound.

    Csound is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Csound; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "cscore.h"                /*   CSCORE_SWITCH.C  */

/* cscore control test: switch between two 2-sect scores */
/* the warped/un-warped quality should be preserved      */

cscore(CSOUND* cs)             /* callable from either Csound or standalone cscore */ 
{ 
  EVLIST *a, *b; 
  FILE   *fp1, *fp2;                      /* declare two scorefile stream pointers */ 

  fp1 = cscoreFileGetCurrent(cs);         /* this is the command-line score */ 
  fp2 = cscoreFileOpen(cs, "score2.srt"); /* this is an additional score file */ 

  a = cscoreListGetSection(cs);           /* read section from score 1 */ 
  cscoreListPut(cs, a);                   /* write it out as is */ 
  cscorePutString(cs, "s"); 
  cscoreFileSetCurrent(cs, fp2); 
  b = cscoreListGetSection(cs);           /* read section from score 2 */ 
  cscoreListPut(cs, b);                   /* write it out as is */ 
  cscorePutString(cs, "s"); 

  cscoreListFreeEvents(cs, a);            /* optional to reclaim space */ 
  cscoreListFreeEvents(cs, b); 

  cscoreFileSetCurrent(cs, fp1); 
  a = cscoreListGetSection(cs);           /* read next section from score 1 */ 
  cscoreListPut(cs, a);                   /* write it out */ 
  cscorePutString(cs, "s"); 
  cscoreFileSetCurrent(cs, fp2); 
  b = cscoreListGetSection(cs);           /* read next sect from score 2 */ 
  cscoreListPut(cs, b);                   /* write it out */ 
  cscorePutString(cs, "e");
}
