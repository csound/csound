#include "csdl.h"
#include <lo/lo.h>

typedef struct
{
    OPDS h;             /* default header */
    MYFLT *kwhen;
    MYFLT *host;
    MYFLT *port;       /* UDP port */
    MYFLT *dest;
    MYFLT *type;
    MYFLT *d1;
    MYFLT *d2;
    lo_address addr;
    MYFLT last;
    int   cnt;
} OSCSEND;

int osc_send_set(ENVIRON *csound, OSCSEND *p)
{
    char port[8];
    char *pp= port;
    char *hh;
    lo_address t;
    if (*p->port<0)
      pp = NULL;
    else
      sprintf(port, "%d", (int)(*p->port+FL(0.5)));
    hh = (char*) p->host;
    if (*hh=='\0') hh = NULL;
    t = lo_address_new(hh, pp);
    p->addr = t;
    p->cnt = 0;
    return OK;
}

int osc_send(ENVIRON *csound, OSCSEND *p)
{
    /* Types I allow at present:
       0) int
       1) float
       2) string
       3) int int
       4) int float
       5) int string
       6) float int
       7) float float
       8) float string
       9) string int
       10) string float
       11) string string
    */
    if (p->cnt++ && *p->kwhen!=p->last) {
      p->last = *p->kwhen;
      switch ((int)(*p->type+FL(0.5))) {
      default:
      case 0:
        lo_send(p->addr, (char*) p->dest, "i", (int)(FL(0.5)+*p->d1));
        return OK;
      case 1:
        lo_send(p->addr, (char*) p->dest, "f", (float) *p->d1);
        return OK;
      case 2:
        lo_send(p->addr, (char*) p->dest, "s", (char*) p->d1);
        return OK;
      case 3:
        lo_send(p->addr, (char*) p->dest, "ii", (int) *p->d1, (int) *p->d2);
        return OK;
      case 4:
        lo_send(p->addr, (char*) p->dest, "if", (int) *p->d1, (float) *p->d2);
        return OK;
      case 5:
        lo_send(p->addr, (char*) p->dest, "is", (int) *p->d1, (char*) p->d2);
        return OK;
      case 6:
        lo_send(p->addr, (char*) p->dest, "fi", (float) *p->d1, (int) *p->d2);
        return OK;
      case 7:
        lo_send(p->addr, (char*) p->dest, "ff", (float) *p->d1, (float) *p->d2);
        return OK;
      case 8:
        lo_send(p->addr, (char*) p->dest, "fs", (float) *p->d1, (char*) p->d2);
        return OK;
      case 9:
        lo_send(p->addr, (char*) p->dest, "si", (char*) p->d1, (int) *p->d2);
        return OK;
      case 10:
        lo_send(p->addr, (char*) p->dest, "sf", (char*) p->d1, (float) *p->d2);
        return OK;
      case 11:
        lo_send(p->addr, (char*) p->dest, "ss", (char*) p->d1, (char*) p->d2);
        return OK;
      }
    }
    return OK;
}

#define S(x) sizeof(x)

static OENTRY localops[] = {
{ "OSCsend", S(OSCSEND),  3, "",  "kSiSiUU", (SUBR)osc_send_set, (SUBR)osc_send }
};

LINKAGE

