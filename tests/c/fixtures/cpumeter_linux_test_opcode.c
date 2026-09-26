#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#ifndef WIN32
#ifndef LINUX
#define LINUX
#endif
#include <stdio.h>
extern FILE *csound_test_open_proc(const char *, const char *);
extern int csound_test_close_proc(FILE *);
#define fopen csound_test_open_proc
#define fclose csound_test_close_proc
#undef LINKAGE_BUILTIN
#define LINKAGE_BUILTIN(name)
#include "../../../Opcodes/cpumeter.c"
#include "cpumeter_test_opcode.h"

OENTRY *csound_test_linux_cpumeter_opcode(void)
{
    return &cpumeter_localops[0];
}

void csound_test_linux_cpumeter_bind(void *data, INSDS *instrument, OPTXT *text,
                   MYFLT *interval, MYFLT **outputs)
{
    CPUMETER *p = (CPUMETER *)data;
    p->h.insdshead = instrument;
    p->h.optext = text;
    p->itrig = interval;
    memcpy(p->kk, outputs, sizeof(p->kk));
}

int32_t csound_test_linux_cpumeter_renew(CSOUND *csound, void *data)
{
    return cpupercent_renew(csound, (CPUMETER *)data);
}

FILE *csound_test_linux_cpumeter_file(void *data)
{
    return ((CPUMETER *)data)->fp;
}
#endif
