/*  
    sfheader.h:

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

                                /*                              SFHEADER.H      */

#ifdef SFIRCAM

# define SIZEOF_HEADER 1024
# define SF_MAGIC 107364L
# define SF_CHAR  sizeof(char)    /* new sfclass, not SFIRCAM standard */
# define SF_ALAW  sizeof(char)    /* new sfclass, not SFIRCAM standard */
# define SF_ULAW  sizeof(char)    /* new sfclass, not SFIRCAM standard */
# define SF_SHORT sizeof(short)
# define SF_LONG  sizeof(long)
# define SF_FLOAT sizeof(float)
# define SF_DOUBLE sizeof(double)
# define SF_24INT 3
# define SF_BUFSIZE     (16*1024) /* used only in play */

typedef union sfheader {
        struct {
                long      sf_magic;
                float     sf_srate;
                long      sf_chans;
                long      sf_packmode;
                char      sf_codes;
        } sfinfo;
        char    filler[SIZEOF_HEADER];
} SFHEADER;

# define sfmagic(x) (x)->sfinfo.sf_magic
# define sfsrate(x) (x)->sfinfo.sf_srate
# define sfchans(x) (x)->sfinfo.sf_chans
# define sfclass(x) (x)->sfinfo.sf_packmode
# define sfcodes(x) (x)->sfinfo.sf_codes
# define sfbsize(x) ((x)->st_size - sizeof(SFHEADER))

# define ismagic(x) ((x)->sfinfo.sf_magic == SF_MAGIC)

# define sfmaxamp(mptr,chan) (mptr)->value[chan]
# define sfmaxamploc(mptr,chan) (mptr)->samploc[chan]
# define sfmaxamptime(x) (x)->timetag
# define ismaxampgood(x,s) (sfmaxamptime(x) >= (s)->st_mtime)


# define sflseek(x,y,z) lseek(x,z != 0 ? (off_t)y : (off_t)((y) + sizeof(SFHEADER)),z)

# define wheader(x,y) write(x,y,sizeof(SFHEADER)) != sizeof(SFHEADER)
# define rheader(x,y) read(x,y,sizeof(SFHEADER)) != sizeof(SFHEADER)

/* #define readopensf(name,fd,sfh,sfst,prog,result) \ */
/* if ((fd = open(name, 0))  < 0) {  \ */
/*      fprintf(stderr,"%s: cannot access file %s\n",prog,name); \ */
/*      result = -1;  \ */
/* } \ */
/* else if (rheader(fd,&sfh)){ \ */
/*      fprintf(stderr,"%s: cannot read header from %s\n",prog,name); \ */
/*      result = -1;  \ */
/* } \ */
/* else if (!ismagic(&sfh)){ \ */
/*      fprintf(stderr,"%s: %s not a bsd soundfile\n",prog,name); \ */
/*      result = -1;  \ */
/* } \ */
/* else if (stat(name,&sfst)){ \ */
/*      fprintf(stderr,"%s: cannot get status on %s\n",prog,name); \ */
/*      result = -1;  \ */
/* } \ */
/* else result = 0; */

#endif

