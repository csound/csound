#include "cscore.h"			/*			CSCORE.C	*/
#ifdef __STDC__
# include <stdlib.h>
#endif

static EVENT buf;

EVLIST * lcreat(nslots)                /* allocate array of event pointer slots */
 int nslots;
{
register EVLIST *a;

	a = (EVLIST *) calloc((unsigned)(nslots+2), sizeof(EVENT *));
	a->nslots = nslots;
	return(a);
}

EVENT * createv(pcnt)                   /* create new event space */
 register int pcnt;
{
register EVENT *e;

	e = (EVENT *) calloc((unsigned)(pcnt+2), sizeof(float));
	e->pcnt = pcnt;
	return(e);
}

EVENT * copyev(e)                     /* make a new copy of an event */
 EVENT *e;
{
	EVENT *f;
register int  n;
register float *p, *q;

	n = e->pcnt;
	f = createv(n);
	f->op = e->op;
	p = &e->p[1];
	q = &f->p[1];
	while (n--)
		*q++ = *p++;
	return(f);
}

EVENT * defev(s)                    /* define an event from string arg */
 register char *s;
{
	EVENT *e;
register float *p, *q;

	while (*s == ' ')
		s++;
	buf.op = *s++;				/* read opcode */
	while (*s == ' ')
		s++;
        p = &buf.p[1];
	q = &buf.p[PMAX];
	while (sscanf(s,"%f",p++) > 0) {		/* read pfields */
		while (*s >= '0' && *s <= '9' || *s == '.' || *s == '-')
			s++;
		while (*s == ' ')
			s++;
		if (p > q && *s != '\0')  {		/* too many ? */
			p++;
                        printf("PMAX exceeded, string event truncated.\n");
			break;
		}
	}
        buf.pcnt = p - &buf.p[1] - 1;   /* count of params recvd */
	e = copyev(&buf);               /* copy event to a new space */
	return(e);
}

EVENT * getev()                      /* read an event from stdin score file */
{
	EVENT *e;
register float *p, *q;
	float junk;
	char c;
                     
	if (scanf("%1s",&buf.op) != EOF)  {	 /* if opcode found */
		while (buf.op == ';') {
			while ((c = getchar()) != '\n' && c != EOF)
				;
			if (scanf("%1s",&buf.op) == EOF)
				return(NULL);
		}
                p = &buf.p[1];
		q = &buf.p[PMAX];
		while (scanf("%f",p++) > 0) {	    /* read pfields into buf */
		    if (p > q && scanf("%f",&junk) > 0) {      /* too many ? */
			printf("PMAX exceeded, input event truncated.\n");
			while (scanf("%f",&junk) > 0)         /* flush extra */
				;
		    }
		}
		buf.pcnt = p - &buf.p[1] - 1;	/* count of params recvd */
		e = copyev(&buf);		/* copy event to a new space */
		return(e);
	}
	else
		return(NULL);
}

void putev(e)                                /* put event to stdout */
 register EVENT *e;
{
register int  pcnt;
register float *q;

	printf("%c",e->op);
	q = &e->p[1];
	pcnt = e->pcnt;
	while (pcnt--)
		printf(" %g",*q++);
	printf("\n");
}

void putstr(s)				/* put string to stdout */
 char *s;
{
	printf("%s\n",s);
}

int lcount(a)			/* count entries in event list */
 EVLIST *a;
{
register EVENT **p;
register int  n, nrem;

	n = 0;	nrem = a->nslots;
	p = &a->e[1];
	while ((nrem--) && *p++ != NULL)
		n++;
	return(n);
}

EVLIST * lappev(a,e)                     /* append an event to a list */
 EVLIST *a;
 EVENT *e;
{
register EVENT **p, **q;
	EVLIST *b;
register int  n, slots;

	n = lcount(a);
	slots = a->nslots;
	if (slots > n) {
		p = &a->e[n+1];
		*p++ = e;
		if (slots > n+1)
			*p = NULL;
	}
        else {
		b = lcreat(n + 5);
		p = &a->e[1];
                q = &b->e[1];
		while (n--)
			*q++ = *p++;
		*q++ = e;
		*q = NULL;
		cfree(a);
		a = b;
	}
	return(a);
}

EVLIST * lget()              /* get section events from stdin scorefile */
{
register EVENT *e;
register EVLIST *a;

	a = lcreat(5);
	while ((e = getev()) != NULL) {
		if (e->op == 's' || e->op == 'e')
			break;
		a = lappev(a,e);
        }
	return(a);
}

void lput(a) 			/* put listed events to stdout */
 EVLIST *a;
{
register EVENT **p;
register int  n;

	n = a->nslots;
	p = &a->e[1];
	while ((n--) && *p != NULL)
		putev(*p++);
}

EVLIST * lcopy(a)
 EVLIST *a;
{
	EVLIST *b;
register EVENT **p, **q;
register int  n;

	n = lcount(a);
	b = lcreat(n);
	p = &a->e[1];
	q = &b->e[1];
	while (n--)
		*q++ = *p++;
	return(b);
}

EVLIST * lcopyev(a)
 EVLIST *a;
{
	EVLIST *b;
register EVENT **p, **q;
register int  n;

        n = lcount(a);
	b = lcreat(n);
	p = &a->e[1];
	q = &b->e[1];
	while (n--)
		*q++ = copyev(*p++);
	return(b);
}

EVLIST * lcat(a,b)
 EVLIST *a, *b;
{
register EVENT **p, **q;
register int n, i, j;

	i = lcount(a);
	j = lcount(b);
	if (i + j > a->nslots) {
		EVLIST *c;
		c = lcreat(i+j);
		p = &a->e[1];
		q = &c->e[1];
		n = i;
		while (n--)
			*q++ = *p++;
		cfree(a);
		a = c;
	}
	else if (i + j < a->nslots)
		a->e[i+j+1] = NULL;
	p = &a->e[i+1];
	q = &b->e[1];
	while (j--)
		*p++ = *q++;
	return(a);
}

void lsort(a)		/* put evlist pointers into chronological order */
 EVLIST *a;
{
	EVENT **p, **q;
register EVENT *e, *f;
	int  n, gap, i, j;

	n = lcount(a);
	e = a->e[n];
	if (e->op == 's' || e->op == 'e')
		--n;
	for (gap = n/2;  gap > 0;  gap /=2)
	    for (i = gap;  i < n;  i++)
		for (j = i-gap;  j >= 0;  j -= gap) {
		    p = &a->e[j+1];     e = *p;
		    q = &a->e[j+1+gap]; f = *q;
		    if (e->p[2] < f->p[2])
			break;
		    if (e->p[2] == f->p[2]) {
			if (e->op == f->op) {
			    if (e->op == 'f')
				break;
			    if (e->p[1] < f->p[1])
				break;
			    if (e->p[1] == f->p[1])
				if (e->p[3] <= f->p[3])
				    break;
			}
			else if (e->op < f->op)
			    break;
		    }
                    *p = f;  *q = e;         
		}
}

EVLIST * lxins(a,s)                      /* list extract by instr numbers */
 EVLIST *a;
 char *s;
{
	int  x[5], xcnt;
register int xn, *xp, insno, n;
	EVENT **p, **q, *e;
	EVLIST *b, *c;

	xcnt = sscanf(s,"%d%d%d%d%d",&x[0],&x[1],&x[2],&x[3],&x[4]);
	n = lcount(a);
	b = lcreat(n);
	p = &a->e[1];
	q = &b->e[1];
	while ((n--) && (e = *p++) != NULL) {
		if (e->op != 'i')
			*q++ = e;
		else {
			insno = e->p[1];
			xn = xcnt;  xp = x;
			while (xn--)
				if (*xp++ == insno) {
					*q++ = e;
					break;
				}
                }    
	}
	c = lcopy(b);
	cfree(b);
        return(c);
}

EVLIST * lxtimev(a,from,to)                      /* list extract by time */
 EVLIST *a;
 float from, to;
{
register EVENT **p, **q, *e;
	EVLIST *b, *c;
	float maxp3;
	int  n;

	n = lcount(a);
	b = lcreat(n);
	p = &a->e[1];
	q = &b->e[1];
	maxp3 = to - from;
	while ((n--) && (e = *p++) != NULL)
		switch (e->op) {
		case 'f':
			if (e->p[2] < to) {
				*q++ = e = copyev(e);
				if (e->p[2] <= from)
					e->p[2] = 0;
				else
					e->p[2] -= from;
			}
			break;
		case 'i':
			if (e->p[2] < from) {
				if (e->p[2] + e->p[3] > from) {
					*q++ = e = copyev(e);
					e->p[3] -= from - e->p[2];
					e->p[2] = 0;
					if (e->p[3] > maxp3)
						e->p[3] = maxp3;
				}
			}
                        else if (e->p[2] < to) {
				*q++ = e = copyev(e);
				if (e->p[2] + e->p[3] > to)
					e->p[3] = to - e->p[2];
				e->p[2] -= from;
			}
			break;
		default:
			*q++ = copyev(e);
			break;
		}
	c = lcopy(b);
	cfree(b);
	return(c);
}

EVLIST * lsepf(a)                        /* separate f events from evlist */
 EVLIST *a;
{
register EVENT **p, **q, **r, **s;
	EVLIST *b, *c;
	int  n;

	n = lcount(a);
	b = lcreat(n);
	p = q = &a->e[1];
	r = s = &b->e[1];
	while (n--) {
		if ((*p)->op == 'f')
			*r++ = *p++;
		else
			*q++ = *p++;
	}
	if (r > s)
		*q = NULL;
	c = lcopy(b);
	cfree(b);
	return(c);
}

void relev(e)                        /* give back event space */
 EVENT *e;
{
	cfree(e);
}

void lrel(a) 			/* give back list space */
 EVLIST *a;
{
	cfree(a);
}

void lrelev(a)			/* give back list and its event spaces */
 EVLIST *a;
{
register EVENT **p;
register int  n;

	n = lcount(a);
	p = &a->e[1];
	while (n--)
		cfree(*p++);
	cfree(a);
}
