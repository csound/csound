# include <stdio.h>                     /*             CSCORE.H     */

# define PMAX 50

typedef struct event {
	char op;
	char tnum;        /* we presume total sizeof these */
	short pcnt;       /*  3 members <= sizeof(float)   */
	float p[PMAX+1];
} EVENT;

typedef struct evlist {
	int nslots;     /* we presume sizeof(int) <= sizeof(EVENT *)  */
	EVENT *e[1];
} EVLIST;

EVENT *createv(), *defev(), *getev(), *copyev();
EVLIST *lcreat(), *lappev(), *lget(), *lcopy(), *lcopyev();
EVLIST *lxins(), *lxtimev(), *lsepf(), *lcat();
void putev(), putstr(), relev();
void lput(), lsort(), lrel(), lrelev();
int  lcount();


