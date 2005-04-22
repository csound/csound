#include "csdl.h"
#include <lo/lo.h>

typedef struct
{
    OPDS h;             /* default header */
    MYFLT *host;
    MYFLT *port;       /* UDP port */
    MYFLT *dest;
    MYFLT *type;
    MYFLT *d1;
    MYFLT *d2;
    lo_address addr;
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
    hh = p->STRARG;
    if (*hh=='\0') hh = NULL;
    t = lo_address_new(hh, pp);
    p->addr = t;
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
       7) float string
       8) float float
       9) string int
       10) string float
       11) string string
    */
    switch ((int)(*p->type+FL(0.5))) {
    default:
    case 0:
      lo_send(p->addr, p->STRARG2, "i", (int)(FL(0.5)+*p->d1));
      return OK;
    case 1:
      lo_send(p->addr, p->STRARG2, "s", p->STRARG3);
        return OK;
    case 2:
        lo_send(p->addr, p->STRARG2, "f", (float)(*p->d1));
        return OK;
    case 3:
      lo_send(p->addr, p->STRARG2, "ii", (int)(*p->d1), (int)(*p->d2));
        return OK;
    case 4:
        lo_send(p->addr, p->STRARG2, "is", (int)(*p->d1), p->STRARG3);
        return OK;
    case 5:
        lo_send(p->addr, p->STRARG2, "if", (int)(*p->d1), (float)(*p->d2));
        return OK;
    case 6:
        lo_send(p->addr, p->STRARG2, "fi", (float)(*p->d1), (int)(*p->d2));
        return OK;
    case 7:
        lo_send(p->addr, p->STRARG2, "ff", (float)(*p->d1), (float)(*p->d2));
        return OK;
    case 8:
        lo_send(p->addr, p->STRARG2, "fs", (float)(*p->d1), p->STRARG3);
        return OK;
    case 9:
        lo_send(p->addr, p->STRARG2, "si", p->STRARG3, (int)(*p->d2));
        return OK;
    case 10:
        lo_send(p->addr, p->STRARG2, "sf", p->STRARG3, (float)(*p->d2));
        return OK;
    case 11:
        lo_send(p->addr, p->STRARG2, "ss", p->STRARG3, p->STRARG4);
        return OK;
    }
}

#define S(x) sizeof(x)

static OENTRY localops[] = {
{ "OSCsend", S(OSCSEND),  3, "",  "SiSiSS", (SUBR)osc_send_set, (SUBR)osc_send }
};

LINKAGE
