#include <stdio.h>              /*                              WIN98.C */
#include <starbase.c.h>
#include "cs.h"

#define TOP      40
#define BOTTOM  760
#define WIDTH   720
#define Y_OFF   400
#define X0       20
#define X100   1023

static  int     gfd, holdit;
static  int     drawbuf[(X100-X0+1)*2];
static  int     drawmax = (X100-X0 +1);
static  char    caption[100];

MakeGraph(wdptr)              /* called from window.c to open a graphics window */
     WINDAT *wdptr;           /* pointer to control structure */
{
         int y, yinc, yy, yyinc;

         if (!gfd) {                                 /* open the graphics display */
             gfd = gopen("/dev/graphics",OUTDEV,"hp98710",INIT);
             buffer_mode(gfd,0);                                /* open graphics */
             mapping_mode (gfd,1);                              /* set params    */
             system("linear 128 255 0 0 0");
             fill_color_index(gfd, 128);
             perimeter_color_index(gfd, 128);
             dcrectangle(gfd, X0,0, X100,TOP-1);
             dcrectangle(gfd, X0,BOTTOM+1, X100,768);
             fill_color_index(gfd,0);
             perimeter_color_index(gfd, 0);
             line_color_index(gfd, 2);
             dcmove(gfd, X0-1, TOP);                            /* draw y-axis    */
             dcdraw(gfd, X0-1, BOTTOM);
             yinc = WIDTH/4;
             yyinc = yinc/5;
             for (y = TOP; y <= BOTTOM; ) {                     /*  and hatch marks */
                  dcmove(gfd, X0-12, y);
                  dcdraw(gfd, X0-1, y);
                  yy = y;
                  y += yinc;
                  while ((yy += yyinc) < y && yy < BOTTOM) {
                         dcmove(gfd, X0-6, yy);
                         dcdraw(gfd, X0-1, yy);
                  }
              }
         }
         wdptr->windid = -1;                    /* report this as "window open" */
 }

DrawGraph(wdptr)                       /* called from window.c to graph an array */
     WINDAT *wdptr;                    /* struct containing data pointers, etc.  */
{
         register float *fp;                    /* ptr to src array */
         register int   *dbp;                   /* ptr to drawbuf */
         register int   xord, xinc, xend, finc;
         int      inpts, drawpts;
         float    absmax, fscale;

         if ((inpts=wdptr->npts) <= drawmax) {          /* for this npts in,    */
                  finc = 1;                             /*   set incrs for simple */
                  xinc = drawmax/inpts;                 /*   mapping onto drawmax */
                  xend = X0 + xinc*(inpts-1);
         }
         else {
                  finc = (inpts-1)/drawmax + 1;
                  xinc = 1;
                  xend = X0 + inpts/finc -1;
         }
         absmax = wdptr->absmax;
         fscale = (WIDTH>>1) / absmax;
         for (fp=wdptr->fdata,dbp=drawbuf,xord=X0; xord <= xend; xord += xinc) {
                  *dbp++ = xord;                        /* scale data into drawbuf  */
                  *dbp++ = Y_OFF - (int)(*fp * fscale);
                  fp += finc;
         }
         drawpts = (dbp - drawbuf) >> 1;                /*    & prepare caption */
         sprintf(caption,"%s  %d points, scalemax %8.2f",wdptr->caption,inpts,absmax);
         while(holdit)
             ;                                          /* if we wish to proceed, */
         dcrectangle(gfd, X0,0, X100,BOTTOM);           /*  erase screen for redraw */
         line_color_index(gfd, 2);                      /* set red */
         dcmove(gfd, X0, Y_OFF);                        /* draw x-axis */
         dcdraw(gfd, xend, Y_OFF);
         line_color_index(gfd, 1);                      /* set white */
         dcpolyline(gfd, drawbuf, drawpts, FALSE);      /* draw segment */
         dctext(gfd, X0, TOP-10, caption);              /*  and caption */
}
