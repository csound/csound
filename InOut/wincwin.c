                         /*                      WINCWIN.C        */
#include "cs.h"
#include <stdlib.h>
#include <stdarg.h>
#include <windows.h>
#include "cwin.h"
#include "cwindow.h"

int Graphable_(void)     /* called during program initialisation */
{
        return 1;
}

void MakeGraph_(WINDAT *wdptr, char *name)
        /* called from window.c to open a graphics window */
{
    IGN(wdptr);
/*     cwin_caption(name); */
/*     cwin_show(); */
}

                                /* Next line must be in line with cwin.cpp */
#define PICTURE_SIZE (32768)
void DrawGraph_(WINDAT *wdptr)    /* called from window.c to graph an array */
{
    MYFLT       *fdata = wdptr->fdata;
    long        npts   = wdptr->npts;
    char        *msg   = wdptr->caption;
    int         lsegs, pol;
    char        string[100];
    int scale = 1;

    pol  = wdptr->polarity;
    cwin_clear();

    sprintf(string,Str(X_48,"%s  %ld points, max %5.3f %s"),
            msg,npts,wdptr->oabsmax, (wdptr->waitflg?Str(X_78,"(wait)"):""));
    cwin_caption(string);
    lsegs = npts;                       /* one lineseg per datum */
    while (lsegs>PICTURE_SIZE) lsegs /=2, scale *= 2; /* Rescale */
    {       /* take scale factors out of for-loop for faster run-time */
        MYFLT x0 = FL(0.0), y0 = FL(0.0), y_off = FL(0.005), x_off = FL(0.005);
        MYFLT x_scale = (FL(2.0)-FL(2.0)*x_off)/ (MYFLT)(lsegs-1);
        MYFLT y_scale = (FL(2.0)-FL(2.0)*y_off)/ wdptr->oabsmax;
        MYFLT  f,*fdptr = fdata;
        int i = 0, j = lsegs;
        if (pol == (short)BIPOL) {
            y_off += FL(0.0);
            y_scale /= FL(2.0);             /* max data scales to h/2 */
        }
        else y_off += FL(1.0);
        cwin_line_dash(0x0000ff, (float)x_off, (float)y_off, (float)(FL(2.0)+x_off), (float)y_off);
        x0 = FL(0.0); y0 = y_off - (*fdptr * y_scale); j--;
        fdptr += scale;
        while (j--) {
          MYFLT x, y;
          f = *fdptr;
          fdptr += scale;
          cwin_line(0xff0000, (float)x0, (float)y0,
                    (float)(x=x_off+ i++ * x_scale), (float)(y=y_off - (f * y_scale)));
          x0 = x; y0 = y;
        }
    }
    cwin_paint();               /* Is too slow comment this line out!! */
    cwin_show();
    if (wdptr->waitflg) {
        cwin_getchar();
        sprintf(string,Str(X_49,"%s  %ld points, max %5.3f"),msg,npts,wdptr->oabsmax);
        cwin_caption(string);
    }
    POLL_EVENTS();
 }

void KillGraph_(WINDAT *wdptr)
{
    IGN(wdptr);
    cwin_clear();
    POLL_EVENTS();
}


int ExitGraph_(void)
{
    cwin_clear();
    POLL_EVENTS();
    return 0;
}

MYFLT mouse_x = FL(0.0), mouse_y = FL(0.0);

void MakeXYin_(XYINDAT *wdptr, MYFLT x, MYFLT y)
{
        /* Use full screen, since we presently don't do more
           than one independent xyin anyway - re Aug 3, 1999 */
    wdptr->m_x = GetSystemMetrics(SM_CXSCREEN);
    wdptr->m_y = GetSystemMetrics(SM_CYSCREEN);
    wdptr->x = (MYFLT)x;
    wdptr->y = (MYFLT)y;
    return;
}

extern void cwin_report_right(char*);
void ReadXYin_(XYINDAT *wdptr)
{
/*  char buff[20]; */
        /* Read cursor from full screen (re Aug 3, 99) */
    POINT mp;
    GetCursorPos(&mp);
    wdptr->x  = (float)mp.x / wdptr->m_x;
    wdptr->y  = (float)mp.y / wdptr->m_y;
/*    wdptr->x  = mouse_x; */
/*    wdptr->y  = mouse_y; */
/*    printf("[%.2f,%.2f]\n", mouse_x, mouse_y); */
/*     sprintf(buff, "[%.2f,%.2f]", mouse_x, mouse_y); */
/*     cwin_report_right(buff); */
    return;
}
