/*
    sort.c:

    Copyright (C) 1991 Barry Vercoe, John ffitch

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

#include "cs.h"                                       /*   SORT.C  */

static void sorter(void)                       /* the main sorting routine */
{
    SRTBLK *newbp, *prvbp;
    SRTBLK *bp;
    char c, pc;
    MYFLT diff;
    int prdiff, indiff;

    bp = frstbp;
    while ((newbp = bp->nxtblk) != NULL
           && (c = newbp->text[0]) != 's' && c != 'e') {
      prvbp = newbp;
      while ((prvbp = prvbp->prvblk) != NULL
             && (pc = prvbp->text[0]) != 'w'
             && (c == 'w' ||
                 (pc != 't' &&
                  (c == 't' ||
                   ((diff = newbp->newp2 - prvbp->newp2) < 0 ||
                    (!diff &&
                     ((prdiff = newbp->preced - prvbp->preced) < 0 ||
                      (!prdiff && c == 'i'&&
                       ((indiff = newbp->insno - prvbp->insno) < 0 ||
                        (!indiff && newbp->newp3 < prvbp->newp3) )
                       ))))))))
        ;

      if (prvbp != bp) {                      /*   if re-ordered,  */
        if ((bp->nxtblk = newbp->nxtblk) != NULL)
          newbp->nxtblk->prvblk = bp;       /* unlink, */
        if ((newbp->prvblk = prvbp) != NULL) {    /*  relink */
          newbp->nxtblk = prvbp->nxtblk;    /*  in mid */
          prvbp->nxtblk = prvbp->nxtblk->prvblk = newbp;
        }
        else {
          newbp->nxtblk = frstbp;         /* or at top */
          frstbp = frstbp->prvblk = newbp;
        }
      }
      else bp = newbp;                        /*  else loop nxtblk */
    }
}

void sort(void)
{
    SRTBLK *bp;

    if ((bp = frstbp) == NULL)
      return;
    do  {
      switch (bp->text[0]) {
      case 'i':
        if (bp->insno < 0)
          bp->preced = 'a';
        else bp->preced = 'd';
        break;
      case 'f':
        bp->preced = 'b';
        break;
      case 'a':
        bp->preced = 'c';
        break;
      case 'q':
      case 'w':
      case 't':
      case 's':
      case 'e':
        break;
      default:
        err_printf(Str(X_1195,"sort: illegal opcode %c(%.2x)\n"),
                   bp->text[0],bp->text[0]);
        break;
      }
    } while ((bp = bp->nxtblk) != NULL);
    sorter();
}
