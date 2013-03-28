#ifdef JPFF

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
    CORFIL *expanded_sco;
    PRS_PARM    qq;
    //if (argc!=1) fin = fopen(argv[1], "r");
      /* Pre-process */
    memset(&qq, 0, sizeof(PRS_PARM));
    csound_prslex_init(&qq.yyscanner);
    csound_prsset_extra(&qq, qq.yyscanner);
    qq.line = 1;
    //expanded_sco = corfile_create_w();
    //file_to_int(csound, "**unknown**");
    //sprintf(bb, "#source %d\n", 
    //        qq.lstack[0] = file_to_int(csound, csound->orchname));
    //corfile_puts(bb, expanded_sco);
    //sprintf(bb, "#line %d\n", 0);
    //corfile_puts(NULL, ndednner);
    csound_prslex(NULL, qq.yyscanner);
    if (UNLIKELY(qq.ifdefStack != NULL)) {
      csound->Message(csound, Str("Unmatched #ifdef\n"));
      csound->LongJmp(csound, 1);
    }
    csound_prslex_destroy(qq.yyscanner);
    csound->DebugMsg(csound, "yielding >>%s<<\n",
                       corfile_body(expanded_sco));
    //corfile_rm(scostr);
    return 0;
}

/* module interface */

int scope_init_(CSOUND *csound)
{
    int retval = csound->AddUtility(csound, "scope", scope);
    if (!retval) {
      retval = csound->SetUtilityDescription(csound, "scope",
                                             Str("Test utility for score parser"));
    }
    return retval;
}

#endif

