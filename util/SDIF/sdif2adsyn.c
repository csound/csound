/* sdif2adsyn.c: convert SDIF 1TRC additive synthesis data to Csound adsyn format
 * v1.0 Richard W Dobson August 4 2000

    Copyright (c) 2000 Richard Dobson

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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "sdif.h"
#include "sdif-mem.h"

#ifndef max
#define max(x,y) ((x)>(y) ? (x) : (y))
#endif
#ifndef min
#define min(x,y) ((x)>(y) ? (y) : (x))
#endif

/* SDIF indices start from 1 */
#define MAXPARTIALS (1025)
/* some old but worthy CNMAT functions */

typedef struct {
    sdif_float32 index, freq, amp, phase;
} SDIF_RowOf1TRC;

int32_t
SDIF_Read1TRCVals(FILE *f,
                  sdif_float32 *indexp, sdif_float32 *freqp,
                  sdif_float32 *ampp, sdif_float32 *phasep) {
    SDIF_RowOf1TRC data;

#ifdef LITTLE_ENDIAN
    if (SDIF_Read4(&data, 4, f) != ESDIF_SUCCESS) return -1;
#else
    if (fread(&data, sizeof(data), 1, f) != 1) return -1;
#endif

    *indexp = data.index;
    *freqp = data.freq;
    *ampp = data.amp;
    *phasep = data.phase;

    return 0;

}

void usage(void)
{
    printf("SDIF2ADS v1.0: convert sdif 1TRC data to Csound hetro data file.\n"
           "usage: [-sN][-pN] sdif2adsyn infile.sdif outfile.ads\n"
           "        -s   : apply amplitude scale factor N\n"
           "        -p   : keep only the first N partials.\n"
           "               (limit: 1024 partials)\n"
           "  NB: the source partial track indices are used directly\n"
           "      to select internal storage.\n"
           "      As these can be arbitrary values, the maximum\n"
           "      of 1024 partials may not be realized in all cases.\n");
}

typedef struct partial_point{
        float amp;
        float freq;
        float pos;
        struct partial_point *next;
} P_POINT;

typedef struct partial_props {
        int64_t numpoints;
        float maxamp;
        float maxfreq;
        float minfreq;
        P_POINT *head;
} PPROPS;

int32_t write_partial(FILE *fp,const P_POINT *partial,float sfac,int32_t do_scale)
;

P_POINT *new_ppoint(float amp,float freq,float pos)
{
    P_POINT *point;

    point = (P_POINT *) malloc(sizeof(P_POINT));
    if (point) {
      point->amp = amp;
      point->freq = freq;
      point->pos = pos;
      point->next = NULL;
    }
    return point;
}

/* if anyone wants it, there is enough information to select a
   frequency range to output.*/

PPROPS *new_pprops(void)
{
    PPROPS *prop;
    prop = (PPROPS *) malloc(sizeof(PPROPS));
    if (prop) {
      prop->numpoints = 0;
      prop->maxamp    = 0.0f;
      prop->maxfreq   = 0.0f;
      prop->minfreq   = 0.0f;
      prop->head      = NULL;   /* will point to head of partial[n]     */
    }
    return prop;
}

int main(int argc, char **argv)
{
    int32_t i,framecount, partials_written = 0;
    int32_t result, iindex;
    int32_t max_partials = MAXPARTIALS;
    int32_t n_partials = 0, maxpoints = 0;
    sdif_int32 frame_id = -1;
    SDIF_FrameHeader fh;
    SDIF_MatrixHeader mh;

    FILE *infile,*outfile;
    P_POINT **partials;
    PPROPS **pprops;
    sdif_float32 index,amp,freq,phase;

    float thistime = 0.0f;
    float maxfreq = 0.0f, minfreq = 100000.0f, maxamp = 0.0f;
    float scalefac = 1.0;
    int32_t doscale = 0;
    short stemp;                 /* to write partial count to adsyn file */

    if (argc < 2) {
      usage();
      exit(1);
    }
    printf("SDIF2ADS v1.1\n");

    if (SDIF_Init() != ESDIF_SUCCESS) {
      fprintf(stderr,"OOPS: SDIF does not work on this machine!\n");
      exit(1);
    }

    while (argv[1][0]=='-') {
      switch (argv[1][1]) {
      case('s'):
        if (argv[1][2] == '\0') {
          fprintf(stderr,"-s flag requires parameter.\n");
          usage();
          exit(1);
        }
        scalefac = (float) atof(&(argv[1][2]));
        if (scalefac==0.0f) {
          fprintf(stderr,"silly value for scalefac!\n");
          exit(1);
        }
        doscale = 1;
        break;
      case('p'):
        if (argv[1][2] == '\0') {
          fprintf(stderr,"-p flag requires parameter.\n");
          usage();
          exit(1);
        }
        max_partials = atoi(&(argv[1][2]));
        if (max_partials < 1) {
          fprintf(stderr, "-p value too low: need at least one partial!\n");
          usage();
          exit(1);
        }
        if (max_partials > (MAXPARTIALS-1)) {
          printf("Warning: -p value too high: setting to %d\n",
                 MAXPARTIALS-1);
        }

        break;
      default:
        fprintf(stderr,"\nunrecognized flag option %s",argv[1]);
        usage();
        exit(1);
      }
      argc--;
      argv++;
    }
    if (argc < 2) {
      fprintf(stderr,"\nSDIF2ADSYN: insufficient arguments");
      usage();
      exit(1);
    }

    result = SDIF_OpenRead(argv[1],&infile);
    if (result != ESDIF_SUCCESS) {
      fprintf(stderr,"Could not open %s: %s\n",
              argv[1], SDIF_GetErrorString(result));
      exit(1);
    }

    if ((outfile = fopen(argv[2],"wb"))==NULL) {
      fprintf(stderr,"\nunable to open outfile %s",argv[3]);
      fclose(infile);
      exit(1);
    }

    /* initialize partials array */
    /* NB index value 0 NOT USED */
    /* allocate for 1024 partials; use user limit on writing outfile */
    partials  = (P_POINT **) malloc(MAXPARTIALS * sizeof(P_POINT *));
    if (partials==NULL) {
      puts("No memory for partial data!\n");
      exit(1);
    }
    pprops = (PPROPS **) malloc (MAXPARTIALS * sizeof(PPROPS *));
    if (pprops==NULL) {
      puts("No memory for partial data!\n");
      free(partials);
      exit(1);
    }

    for (i=0;i < MAXPARTIALS; i++) {
      partials[i] = NULL;
      pprops[i] = NULL;
    }

    /* STRATEGY: a very straight conversion from a frame structure to
     * a breakpoint structure.  for each new partial in the SDIF file,
     * we start a new partial in the array, at the given index.  when
     * a partial dies, the amplitude is (presumed to be) zero, but the
     * list is not terminated.  if an SDIF frame with that index
     * starts a new partial, the partial with that index continues
     * accordingly.
     *
     * Though in most cases, track indices will small and sequential,
     * this cannot be presumed. If most or all indices are above
     * MAXPARTIALS, we are b******d.
     * Of course, the SDIF file will not tell us what the index range is.
     * That would be too easy.
     */

    /* main loop to read sdif frames */
    framecount = 0;
    /* assume the frame is kosher for now...*/
    while ((result = SDIF_ReadFrameHeader(&fh, infile))==ESDIF_SUCCESS) {

      if (strncmp(fh.frameType,"1TRC",4))
        continue;

      /*we can only copy one stream - use the first one! */
      if (frame_id==-1)
        frame_id = fh.streamID;
      else if (frame_id != fh.streamID) {
        fprintf(stderr,"\nWARNING: multiple stream IDs found - skipping");
        continue;
      }
#ifdef _DEBUG
      /*printf("\nReading SDIF Frame %d, time = %.4f",framecount,fh.time); */
#endif
      thistime = (float) fh.time;
      /* Csound adsyn counts time in msecs, in shorts, so can only have
         32.767 seconds! */
      if (thistime > 32.76f) {
        printf("Sorry! Reached ADSYN limit of 32.76 seconds.\n");
        break;
      }
      framecount++;
      for (i = 0; i < fh.matrixCount; ++i) {
        int32_t j;

        if (SDIF_ReadMatrixHeader(&mh,infile) != ESDIF_SUCCESS) {
          fprintf(stderr,"\nerror reading infile");
          exit(1);
        }
        /* for now, only accept floats */
        if (mh.matrixDataType != SDIF_FLOAT32) {
          fprintf(stderr,"\nsorry - data not floats");
          exit(1);
        }

        n_partials = mh.rowCount;
#ifdef _DEBUG
        /*printf("\nReading Matrix %d: %d rows",i+1,n_partials); */
#endif

        for (j=0;j < n_partials; j++) {

          if (SDIF_Read1TRCVals(infile,&index,&freq,&amp, &phase)) {
            fprintf(stderr,"\nerror reading tracks");
            exit(1);
          }
          if (index < 1.0f) {
            fprintf(stderr,"Illegal partial index %.2f found in frame %d.\n",
                    index,framecount);
            exit(1);
          }
          iindex = (int32_t) index;
          /* a small (unlikely) sacrifice for the sake of a simple algorithm! */
          if (iindex >= MAXPARTIALS) {
            printf("Warning: high partial index %d in frame %d; skipping.\n",
                   iindex,framecount);
            continue;
          }
          /* make a new list if we need it */
          if (partials[iindex]==NULL) {
            partials[iindex] = new_ppoint(amp,freq,thistime);
            if (partials[iindex]==NULL) {
              puts("No memory for partial data!\n");
              exit(1);
            }
            pprops[iindex] = new_pprops();
            if (pprops[iindex]==NULL) {
              puts("No memory for partial data!\n");
              exit(1);
            }
            pprops[iindex]->head = partials[iindex];
            pprops[iindex]->maxamp = amp;
            pprops[iindex]->maxfreq = pprops[iindex]->minfreq = freq;
            pprops[iindex]->numpoints = 1;
          }

          else {
            /*attach new point to existing list, and update props */
            partials[iindex]->next = new_ppoint(amp,freq,thistime);
            if (partials[iindex]->next==NULL) {
              puts("No memory for partial data!\n");
              exit(1);
            }
            partials[iindex] = partials[iindex]->next;

            pprops[iindex]->maxamp = max(pprops[iindex]->maxamp,amp);
            pprops[iindex]->maxfreq = max(pprops[iindex]->maxfreq,freq);
            pprops[iindex]->minfreq = min(pprops[iindex]->minfreq,freq);
            pprops[iindex]->numpoints++;
          }
        }
      }
    }
    /* maybe a single frame is legit? */
    if (framecount < 2) {
      if (framecount==1)
        fprintf(stderr,"Only one frame found: at least two required.\n");
      else
        fprintf(stderr,"No 1TRC frames found in this sdif file.\n");
      SDIF_CloseRead(infile);
      fclose(outfile);
      free(partials);
      free(pprops);
      exit(0);
    }

    /* now we have to write it all out for Csound */
    /* how many partials do we have, and what might a good srate be?*/
    n_partials = 0;
    for (i=0;i < MAXPARTIALS;i++) {
      if (partials[i] != NULL) {
        maxfreq = max(maxfreq,pprops[i]->maxfreq);
        minfreq = min(minfreq,pprops[i]->minfreq);
        maxamp = max(maxamp,pprops[i]->maxamp);
        maxpoints = max(maxpoints,(int32_t)pprops[i]->numpoints);
        n_partials++;
      }
    }
    printf("total partials read   = %d\n",n_partials);
    printf("maximum breakpoints   = %d\n",maxpoints);
    printf("max frequency found   = %.4f\n",maxfreq);
    printf("min frequency found   = %.4f\n",minfreq);
    printf("max partial amp found = %.4f\n",maxamp);
    printf("last frame time       = %.4f\n",thistime);

    /* OK, write this all to  adsyn file as shorts (for now....) */
    stemp = (short) n_partials;
    stemp = min(stemp,max_partials);   /* set user limit, if any */
    /* write number of partial tracks */
    if (fwrite((short *) &stemp,sizeof(short),1,outfile) < 1) {
      fprintf(stderr,"Error writing to outfile.\n");
      exit(1);
    }
    /* now write each set of amp,freq sets */

    for (i=0;i < MAXPARTIALS;i++) {
      if (partials[i]==NULL)
        continue;
      if (write_partial(outfile,pprops[i]->head,scalefac,doscale) < 0) {
        fprintf(stderr,"Error writing partial %d.\n",i);
        exit(1);
      }
      partials_written++;
      if (partials_written == (int32_t) stemp)
        break;
    }

    /* release linked lists */
    for (i=0;i < MAXPARTIALS;i++) {
      P_POINT *tthis,*next;
      if (partials[i]==NULL)
        continue;
      tthis = pprops[i]->head;
      free(pprops[i]);
      next = tthis->next;
      while(next) {
        free(tthis);
        tthis = next;
        next = next->next;
      }
      free(tthis);
    }
    free(partials);
    free(pprops);
    fclose(outfile);
    printf("File conversion completed.\n");
    printf("%d partials written to %s\n",(int32_t)stemp,argv[2]);
    SDIF_CloseRead(infile);
    return 0;
}

/* pass props->head */
/* NB does not check for over-range times */
/* it also trusts that there is a freq element matching each amp element */
int32_t write_partial(FILE *fp,const P_POINT *partial,float sfac,int32_t do_scale)
{
    int32_t length = 0;
    const P_POINT *head;
    float scalefac = 32767.0f;
    short amp,freq,time;
    short amptag = -1, freqtag = -2;
    short trackend = 32767;

    if (fp==NULL)
      return -1;
    if (partial==NULL)
      return -1;

    if (do_scale)
      scalefac *= sfac;

    head = partial;
    if ((fwrite((char *)&amptag,sizeof(short),1,fp)) < 1)
      return -1;

    /* nothing clever, just go through each list twice */

    /*write amps, scaled to 32767 (oh for a f/p adsyn format...) */
    while (head) {
      time = (short)(1000.0f * head->pos);
      if ((fwrite((char *) &time,sizeof(short),1,fp)) < 1)
        return -1;
      amp = (short) (scalefac * head->amp);
      if ((fwrite((char *)&amp,sizeof(short),1,fp)) < 1)
        return -1;
      length++;
      head = head->next;
    }
    if ((fwrite(&trackend,sizeof(short),1,fp)) < 1)
      return -1;

    head = partial;
    if ((fwrite(&freqtag,sizeof(short),1,fp)) < 1)
      return -1;
    while(head) {
      time = (short)(1000.0f * head->pos);
      if ((fwrite((char *) &time,sizeof(short),1,fp)) < 1)
        return -1;
      freq = (short)(head->freq);
      if ((fwrite(&freq,sizeof(short),1,fp)) < 1)
        return -1;
      head = head->next;
    }
    if ((fwrite(&trackend,sizeof(short),1,fp)) < 1)
      return -1;
    return length;
}
