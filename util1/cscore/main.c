#include <csound/cscore.h>

main()
{
	struct event *e, **p;
	struct evlist *a, *b;
        int  n;

	e = createv(5); 	/* alloc p0-5 and init pcnt */
	e->op = 'f';
	e->p[1] = 1;            /* construct an event */
	e->p[2] = 0;
	e->p[3] = 256;
	e->p[4] = 10;
	e->p[5] = 1;
	putev(e);		/*   and write it out */
	relev(e);
	putstr("s");
	a = lget();		/* read sect from score */
	lput(a);		/* write it out as is */
	putstr("s");
	lsort(a);
	lput(a);		/* write out a sorted version */
	putstr("s");
	b = lcopyev(a); 	/* duplicate the notes */
	n = lcount(b);
	p = &b->e[1];		/* point at first one */
	while (n--)
	   (*p++)->p[2] += 2.0; /* delay each by two beats */
	a = lcat(a,b);
	lsort(a);		/* combine and sort the two sets */
	lput(a);		/*	and write out */
	lrelev(a);		/* release all of these notes */
	putstr("s");
	e = defev("f 2 0 256 10 5 6 ");
	a = lcreat(0);
        a = lappev(a,e);
	lput(a);
	lrelev(a);
	putstr("e");
}


/*	This program named main.c can be compiled with: 	*/
/*	  $ cc -Ixxxx main.c -o cscore -lcscore			*/
/*	 where -Ixxxx is -I/usr/local/include or equivalent,	*/
/*	 designed to find the include file <csound/cscore.h>,	*/
/*	 and may be omitted depending on the system default	*/

/*	The resulting executable can be run with:		*/
/*	  $ cscore <scorin					*/


