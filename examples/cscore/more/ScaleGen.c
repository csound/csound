/*      ScaleGen.c

        Cscore control program to generate a score with
        several major scales in parallel thirds.  Even though it
        requires no input score, it is necessary to specify one.
        You can use the supplied dummy file "e.sco".

        ScaleGen asks you for the number of parallel scales and
        how many octaves to create for each scale.

        Anthony Kozar
        June 6, 2004
        Feb. 12, 2007:  Updated for Csound 5.
 */

#include        <stdio.h>
#include        "cscore.h"

#define SIZE    7
float           MajorScaleDegrees[SIZE] = { .00, .02, .04, .05, .07, .09, .11 };
float           BaseOctave = 6.0;               /* two octaves below middle C */

void    cscore(CSOUND* cs)
{
        EVLIST *scale, *copy, *score;
        EVENT  *e;
        int     numscales, numoctaves, numnotes;
        float   amp;
        int     i, j;

        /* get parameters from user */
        fprintf(stderr, "How many parallel scales? ");
        scanf("%d", &numscales);
        fprintf(stderr, "How many octaves per scale? ");
        scanf("%d", &numoctaves);

        /* make the prototype scale */
        numnotes = numoctaves * 7 + 1;
        amp = 30000.0 / numscales;
        scale = cscoreListCreate(cs, numnotes); /* allocate a list with just
                                                   enough space */
        for     (i = 1; i <= numnotes; i++)     {
                int octave, degree;
                octave = (i-1) / SIZE;
                degree = (i-1) % SIZE;

                e = cscoreCreateEvent(cs, 5); /* allocate a note with 5 pfields */
                e->op = 'i';
                e->p[1] = 1;                  /* instrument 1 */
                e->p[2] = 0.25 * (float)(i-1);/* start on consecutive sixteenth beats */
                e->p[3] = 0.20;               /* slightly detached */
                e->p[4] = amp;                /* amplitude, scaled for numscales */
                /* pitch in octave.step notation */
                e->p[5] = BaseOctave + (float)octave +
                          MajorScaleDegrees[degree];

            /* append e, scale will expand if necessary (though it shouldn't) */
                scale = cscoreListAppendEvent(cs, scale, e);
        }

        /* make and shift copies of scale, */
        /* building the score as we go */
        score = cscoreListCreate(cs, 1);
        e = cscoreDefineEvent(cs, "t 0 100"); /* set tempo to 100 bpm */
        score = cscoreListAppendEvent(cs, score, e);
        /* append scale at time 0 */
        score = cscoreListConcatenate(cs, score, scale);

        for     (j = 1; j < numscales; j++) { /* copy numscales-1 times */
          copy = cscoreListCopyEvents(cs, scale);
          for     (i = 1; i <= copy->nevents; i++) /* iterate over the events */
            /* shifting an eighth beat further for each scale */
            copy->e[i]->p[2] += 0.5 * j;
          /* append this copy to score */
          score = cscoreListConcatenate(cs, score, copy);
          cscoreListFree(cs, copy); /* reclaim list memory (but not events) */
        }

        /* append end-of-score statement */
        score = cscoreListAppendStringEvent(cs, score, "e");
        cscoreListPut(cs, score); /* write out unsorted score */
        cscoreListFreeEvents(cs, score); /* reclaim list and event memory */
        cscoreListFree(cs, scale);

        return;
}
