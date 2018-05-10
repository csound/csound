/*
    sort.c:

    Copyright (C) 2010 John ffitch with some code from Barry Vercoe

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

/* the smoothsort is from the Web
http://en.wikibooks.org/wiki/Algorithm_Implementation/Sorting/Smoothsort
recovered march 2010
Adapted from Delphi implementation of Dijkstra's algorithm.
*/

#include "csoundCore.h"                         /*   SORT.C  */

/* inline int ordering(SRTBLK *a, SRTBLK *b) */
/* { */
/*     char cb = b->text[0], ca = a->text[0]; */
/*     MYFLT diff; */
/*     int prdiff, indiff; */
/*     int ans; */
/*     ans = !(ca != 'w' */
/*            && (cb == 'w' || */
/*                (ca != 't' && */
/*                 (cb == 't' || */
/*                  ((diff = b->newp2 - a->newp2) < 0 || */
/*                   (!diff && */
/*                    ((prdiff = b->preced - a->preced) < 0 || */
/*                     (!prdiff && cb == 'i' && */
/*                      ((indiff = b->insno - a->insno) < 0 || */
/*                       (!indiff && b->newp3 < a->newp3) ) */
/*                      )))))) || */
/*                (b->lineno < a->lineno) )); */
/*     /\* fprintf(stderr, "(%p,%p)[%c,%c] -> %d\n", a, b, ca, cb, ans); *\/ */
/*     return ans; */
/* } */
#if !defined(TRUE)
#define TRUE (1)
#endif
#if !defined(FALSE)
#define FALSE (0)
#endif

static inline int ordering(SRTBLK *a, SRTBLK *b)
{
    char cb = b->text[0], ca = a->text[0];
    MYFLT tmp;
    int itmp;
    /* printf("SORT: ca=%c, cb=%c\n", ca, cb); */
    if (ca=='w') return TRUE;
    if (cb=='w') return FALSE;
    if (cb=='t') return FALSE;
    tmp = b->newp2 - a->newp2;
    if (tmp < 0) return FALSE;
    if (tmp > 0) return TRUE;
    itmp = b->preced - a->preced;
    if (itmp < 0) return FALSE;
    if (itmp > 0) return TRUE;
    if ((cb == 'i') && (ca=='i')) {
      /* printf("SORT: ain=%f, bin=%f\n", a->insno, b->insno); */
      tmp = b->insno - a->insno;
      if (tmp < 0) return FALSE;
      if (tmp > 0) return TRUE;
      /* printf("SORT: ap3=%f, bp3=%f\n", a->newp3, b->newp3); */
      /*      tmp = fabs(b->newp3) - fabs(a->newp3); */
      tmp = b->newp3 - a->newp3;
      if (tmp < 0) return FALSE;
      if (tmp > 0) return TRUE;
    }
    /* printf("(%p,%p)[%c,%c]{%d,%d} -> %d\n", */
    /*         a, b, ca, cb, a->lineno, b->lineno,  b->lineno > a->lineno); */
    return (b->lineno > a->lineno);
}

#define UP(IA,IB)   {temp=IA; IA+=(IB)+1;     IB=temp;}
#define DOWN(IA,IB) {temp=IB; IB=(IA)-(IB)-1; IA=temp;}

/* These need to be encapsulated */
#define q  (data[0])
#define r  (data[1])
#define p  (data[2])
#define b  (data[3])
#define c  (data[4])
#define r1 (data[5])
#define b1 (data[6])
#define c1 (data[7])

inline static void sift(SRTBLK *A[], int data[])
{
    int r0, r2, temp;
    SRTBLK * T;
    r0 = r1;
    T = A[r0];
    while (b1 >= 3) {
      r2 = r1-b1+c1;
      if (! ordering(A[r1-1],A[r2])) {
        r2 = r1-1;
        DOWN(b1,c1)
      }
      if (ordering(A[r2],T)) b1 = 1;
      else {
        A[r1] = A[r2];
        r1 = r2;
        DOWN(b1,c1)
      }
    }
    if (UNLIKELY(r1 != r0)) A[r1] = T;
}

inline static void trinkle(SRTBLK *A[], int data[])
{
    int p1,r2,r3, r0, temp;
    SRTBLK * T;
    p1 = p; b1 = b; c1 = c;
    r0 = r1; T = A[r0];
    while (p1 > 0) {
      while ((p1 & 1)==0) {
        p1 >>= 1;
        UP(b1,c1)
      }
      r3 = r1-b1;
      if ((p1==1) || ordering(A[r3], T)) p1 = 0;
      else {
        p1--;
        if (b1==1) {
          A[r1] = A[r3];
          r1 = r3;
        }
        else
          if (b1 >= 3) {
            r2 = r1-b1+c1;
            if (! ordering(A[r1-1],A[r2])) {
              r2 = r1-1;
              DOWN(b1,c1)
              p1 <<= 1;
            }
            if (ordering(A[r2],A[r3])) {
              A[r1] = A[r3]; r1 = r3;
            }
            else {
              A[r1] = A[r2];
              r1 = r2;
              DOWN(b1,c1)
              p1 = 0;
            }
          }
      }
    }
    if (r0-r1) A[r1] = T;
    sift(A, data);
}

inline static void semitrinkle(SRTBLK *A[], int data[])
{
    SRTBLK * T;
    r1 = r-c;
    if (! ordering(A[r1],A[r])) {
      T = A[r]; A[r] = A[r1]; A[r1] = T;
      trinkle(A, data);
    }
}

static void smoothsort(SRTBLK *A[], const int N)
{
    int temp;
    int data[] = {/*q*/ 1, /*r*/ 0, /*p*/ 1, /*b*/ 1, /*c*/ 1, 0,0,0};

    /* building tree */
    while (q < N) {
      r1 = r;
      if ((p & 7)==3) {
        b1 = b; c1 = c; sift(A, data);
        p = (p+1) >> 2;
        UP(b,c) UP(b,c)
      }
      else if ((p & 3)==1) {
        if (q + c < N) {
          b1 = b; c1 = c; sift(A, data);
        }
        else trinkle(A, data);
        DOWN(b,c);
        p <<= 1;
        while (b > 1) {
          DOWN(b,c)
            p <<= 1;
        }
        p++;
      }
      q++; r++;
    }
    r1 = r; trinkle(A, data);

    /* building sorted array */
    while (q > 1) {
      q--;
      if (b==1) {
        r--; p--;
        while ((p & 1)==0) {
          p >>= 1;
          UP(b,c)
        }
      }
      else
      if (b >= 3) {
        p--; r = r-b+c;
        if (p > 0) semitrinkle(A, data);
        DOWN(b,c)
        p = (p << 1) + 1;
        r = r+c;  semitrinkle(A, data);
        DOWN(b,c)
        p = (p << 1) + 1;
      }
      /* element q processed */
    }
    /* element 0 processed */
}


void sort(CSOUND *csound)
{
    SRTBLK *bp;
    SRTBLK **A;
    int i, n = 0;
    if (UNLIKELY((bp = csound->frstbp) == NULL))
      return;
    do {
      n++;                      /* Need to count to alloc the array */
      switch ((int) bp->text[0]) {
      case 'd':
      case 'i':
        if (bp->insno < 0)
          bp->preced = 'b';
        else bp->preced = 'd';
        break;
      case 'f':
        bp->preced = 'c';
        break;
      case 'a':
        bp->preced = 'e';
        break;
      case 'e':
        //        bp->newp2 ;
      case 'q':
      case 'w':
      case 't':
      case 's':
        bp->preced = 'a';
        break;
      case 'x':
        n--;
        break;
      case -1:
      case 'y':
        break;
      default:
        csound->Message(csound, Str("sort: illegal opcode %c(%.2x)\n"),
                                bp->text[0], bp->text[0]);
        break;
      }
    } while ((bp = bp->nxtblk) != NULL);

    if (n>1) {
      /* Get a temporary array and populate it */
      A = ((SRTBLK**) csound->Malloc(csound, n*sizeof(SRTBLK*)));
      bp = csound->frstbp;
      for (i=0; i<n; i++,bp = bp->nxtblk) {
        A[i] = bp;
        if (bp->text[0]=='x') i--; /* try to ignore x opcode */
      }
      if (LIKELY(A[n-1]->text[0]=='e' || A[n-1]->text[0]=='s'))
        smoothsort(A, n-1);
      else
        smoothsort(A, n);
      /* Relink list in order; first and last different */
      csound->frstbp = bp = A[0]; bp->prvblk = NULL; bp->nxtblk = A[1];
      for (i=1; i<n-1; i++ ) {
        bp = A[i]; bp->prvblk = A[i-1]; bp->nxtblk = A[i+1];
      }
      if (n>1) { bp = A[n-1]; } bp->nxtblk = NULL; bp->prvblk = A[n-2];
      /* and return temporary space */
      csound->Free(csound, A);

    }
}
