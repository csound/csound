#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "std_util.h"                                   /*  HETRO.C   */
#include "corfile.h"
#include "score_param.h"

extern void print_csound_prsdata(void *);
extern void csound_prslex_init(void *);
extern void csound_prsset_extra(void *, void *);

extern void csound_prslex(CSOUND*, void*);
extern void csound_prslex_destroy(void *);

static int scope(CSOUND *csound, int argc, char **argv)
{
#if 0
    PRS_PARM  qq;
    int len=100, p=0, n;
    char buff[1024];
    FILE *ff;
      /* Pre-process */
    memset(&qq, '\0', sizeof(PRS_PARM));
    csound->scorestr = corfile_create_w();
    csound_prslex_init(&qq.yyscanner);
    csound_prsset_extra(&qq, qq.yyscanner);
    csound->scorename = argv[1];
    ff = fopen(csound->scorename, "r");
    memset(buff, '\0', 1024);
    while ((n = fread(buff, 1, 1023, ff))) {
      corfile_puts(buff, csound->scorestr);
      memset(buff, '\0', 1024);
    }
    corfile_putc('\0', csound->scorestr);     /* For use in bison/flex */
    corfile_putc('\0', csound->scorestr);     /* For use in bison/flex */

    csound->expanded_sco = corfile_create_w();
    snprintf(buff, 1024, "#source %d\n",
            qq.lstack[0] = file_to_int(csound, csound->scorename));
    corfile_puts(buff, csound->expanded_sco);
    snprintf(buff, "#line %d\n", csound->orcLineOffset);
    corfile_puts(buff, csound->expanded_sco);
    qq.line = 1;
    csound_prslex(csound, qq.yyscanner);
    csound->DebugMsg(csound, "yielding >>%s<<\n",
                     corfile_body(csound->expanded_sco));
    csound_prslex_destroy(&qq.yyscanner);
#endif
    return 0;
}


/* module interface */

PUBLIC int scope_init_(CSOUND *csound)
{
    int retval = csound->AddUtility(csound, "scope", scope);
    if (!retval) {
      retval = csound->SetUtilityDescription(csound, "scope",
                                             Str("Test utility for score parser"));
    }
    return retval;
}
