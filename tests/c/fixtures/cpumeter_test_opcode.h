#ifndef CSOUND_TEST_CPUMETER_OPCODE_H
#define CSOUND_TEST_CPUMETER_OPCODE_H
#ifdef __cplusplus
extern "C" {
#endif
OENTRY *csound_test_linux_cpumeter_opcode(void);
void csound_test_linux_cpumeter_bind(void *, INSDS *, OPTXT *, MYFLT *, MYFLT **);
int32_t csound_test_linux_cpumeter_renew(CSOUND *, void *);
FILE *csound_test_linux_cpumeter_file(void *);
#if defined(__MACH__) && !defined(LINUX)
OENTRY *csound_test_mach_cpumeter_opcode(void);
void csound_test_mach_cpumeter_bind(void *, INSDS *, OPTXT *, MYFLT *, MYFLT **);
int32_t csound_test_mach_cpumeter_renew(CSOUND *, void *);
#endif
#ifdef __cplusplus
}
#endif
#endif
