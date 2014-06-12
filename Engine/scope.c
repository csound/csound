#include "csoundCore.h"
#include "corfile.h"
#include "score_param.h"

extern void print_csound_prsdata(void *);
extern void csound_prslex_init(void *);
extern void csound_prsset_extra(void *, void *);

extern void csound_prslex(CSOUND*, void*);
extern void csound_prslex_destroy(void *);
extern void csound_sco_scan_buffer (const char *, size_t, void*);
extern int csound_scoparse(SCORE_PARM *, void *, CSOUND*, ScoreTree*);
extern void csound_scolex_init(void *);
extern void csound_scoset_extra(void *, void *);
extern void csound_scoset_lineno(int, void*);
extern void csound_scolex_destroy(void *);

int scope(CSOUND *csound, int argc, char **argv)
{
#ifdef SCORE_PARSER
    {
      PRS_PARM  qq;
      int n;
      char buff[1024];
      FILE *ff;
      /* Pre-process */
      memset(&qq, '\0', sizeof(PRS_PARM));
      csound->scorestr = corfile_create_w();
      csound_prslex_init(&qq.yyscanner);
      csound_prsset_extra(&qq, qq.yyscanner);
      csound->scorename = argv[1];
      ff = fopen(csound->scorename, "r");
      if (ff==NULL) {
        csound->Message(csound, "no input\n");
        exit(1);
      }
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
      snprintf(buff, 1024, "#line %d\n", csound->scoLineOffset);
      corfile_puts(buff, csound->expanded_sco);
      qq.line = 1;
      csound_prslex(csound, qq.yyscanner);
      csound->DebugMsg(csound, "yielding >>%s<<\n",
                       corfile_body(csound->expanded_sco));
      csound_prslex_destroy(qq.yyscanner);
      corfile_rm(&csound->scorestr);
    }
    {
      ScoreTree* scoTree = (ScoreTree *)csound->Calloc(csound, sizeof(ScoreTree));
      SCORE_PARM  pp;
      int err;
      /* Parse */
      memset(&pp, '\0', sizeof(SCORE_PARM));
      csound_scolex_init(&pp.yyscanner);
      csound_scoset_extra(&pp, pp.yyscanner);
      csound_sco_scan_buffer(corfile_body(csound->expanded_sco),
                             corfile_tell(csound->expanded_sco), pp.yyscanner);
      err = csound_scoparse(&pp, pp.yyscanner, csound, scoTree);
      corfile_rm(&csound->expanded_sco);
      if (LIKELY(err == 0))
        csound->Message(csound, "Parsing successful!\n");
    }
#endif     
    return 0;
}

#if 0
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
#endif

