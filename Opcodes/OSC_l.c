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
    MYFLT *arg[PMAX-5];
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
       3) double
       4) char
    */
    if (p->cnt++ && *p->kwhen!=p->last) {
      lo_message msg = lo_message_new();
      char *type = (char*)p->type;
      MYFLT **arg = &(p->arg[0]);
      p->last = *p->kwhen;
      while (*type) {
        switch (*type) {
        case 'i':
          lo_message_add_int32(msg, (int32_t)(**arg+FL(0.5)));
          break;
        case 'c':
          lo_message_add_char(msg, (char)(**arg+FL(0.5)));
          break;
        case 'f':
          lo_message_add_float(msg, (float)**arg);
          break;
        case 'd':
          lo_message_add_double(msg, (double)**arg);
          break;
        case 's':
          lo_message_add_string(msg, (char*) *arg);
          break;
        default:
          csound->Message(csound, "Unknown OSC type %c\n", *p);
        }
        p++; arg++;
      }
      lo_send_message(p->addr, (char*)p->dest, msg);
      lo_message_free(msg);
    }
    return OK;
}

#define S(x) sizeof(x)

static OENTRY localops[] = {
{ "OSCsend", S(OSCSEND),  3, "",  "kSiSSN", (SUBR)osc_send_set, (SUBR)osc_send }
};

LINKAGE

