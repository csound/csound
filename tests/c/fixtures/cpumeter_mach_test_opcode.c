#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#if defined(__MACH__) && !defined(LINUX)
#include <mach/mach.h>
#include <mach/processor_info.h>
#include <mach/mach_host.h>
extern kern_return_t csound_test_processor_info(host_t, processor_flavor_t,
    natural_t *, processor_info_array_t *, mach_msg_type_number_t *);
extern kern_return_t csound_test_release_info(vm_map_t, vm_address_t, vm_size_t);
#define host_processor_info csound_test_processor_info
#define vm_deallocate csound_test_release_info
#undef LINKAGE_BUILTIN
#define LINKAGE_BUILTIN(name)
#include "../../../Opcodes/cpumeter.c"
#include "cpumeter_test_opcode.h"

OENTRY *csound_test_mach_cpumeter_opcode(void)
{
    return &cpumeter_localops[0];
}

void csound_test_mach_cpumeter_bind(void *data, INSDS *instrument, OPTXT *text,
                   MYFLT *interval, MYFLT **outputs)
{
    CPUMETER *p = (CPUMETER *)data;
    p->h.insdshead = instrument;
    p->h.optext = text;
    p->itrig = interval;
    memcpy(p->kk, outputs, sizeof(p->kk));
}

int32_t csound_test_mach_cpumeter_renew(CSOUND *csound, void *data)
{
    return cpupercent_renew(csound, (CPUMETER *)data);
}
#endif
