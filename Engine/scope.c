#include "csoundCore.h"
#include "corfile.h"
#include "score_param.h"

extern uint8_t file_to_int(CSOUND*, const char*);
int scope(CSOUND *csound)
{
extern void print_csound_prsdata(void *);
extern int csound_prslex_init(void *);
extern void csound_prsset_extra(void *, void *);

extern int csound_prslex(CSOUND*, void*);
extern int csound_prslex_destroy(void *);
extern void csound_sco_scan_buffer (const char *, size_t, void*);
extern int csound_scoparse(SCORE_PARM *, void *, CSOUND*, ScoreTree*);
extern void csound_scolex_init(void *);
extern void csound_scoset_extra(void *, void *);
extern void csound_scoset_lineno(int, void*);
extern void csound_scolex_destroy(void *);
#if 0
    {
      PRS_PARM  qq;
      char buff[1024];
      /* Pre-process */
      memset(&qq, '\0', sizeof(PRS_PARM));
      csound_prslex_init(&qq.yyscanner);
      csound_prsset_extra(&qq, qq.yyscanner);
      //printf("depth = %d\n", qq.depth);

      csound->expanded_sco = corfile_create_w();
      printf("Input:\n%s<<<\n", corfile_body(csound->sreadStatics.str->cf));
      snprintf(buff, 1024, "#source %d\n",
               qq.lstack[0] = file_to_int(csound,
                                          csound->scorename?
                                          csound->scorename:"**unknown**"));
      corfile_puts(buff, csound->expanded_sco);
      snprintf(buff, 1024, "#line %d\n", csound->scoLineOffset);
      corfile_puts(buff, csound->expanded_sco);
      qq.line = 1;
      csound_prslex(csound, qq.yyscanner);
      csound->Message(csound, "yielding >>%s<<\n",
                       corfile_body(csound->expanded_sco));
      csound_prslex_destroy(qq.yyscanner);
      corfile_rm(&csound->scorestr);
    }
    {
      ScoreTree* scoTree = (ScoreTree *)csound->Calloc(csound, sizeof(ScoreTree));
      SCORE_PARM  pp;
      extern int csound_scodebug;
      int err;
      /* Parse */
      memset(&pp, '\0', sizeof(SCORE_PARM));
      csound_scolex_init(&pp.yyscanner);
      csound_scoset_extra(&pp, pp.yyscanner);
      csound_sco_scan_buffer(corfile_body(csound->expanded_sco),
                             corfile_tell(csound->expanded_sco), pp.yyscanner);
      csound_scodebug = 0;
      err = csound_scoparse(&pp, pp.yyscanner, csound, scoTree);
      corfile_rm(&csound->expanded_sco);
      if (LIKELY(err == 0))
        csound->Message(csound, "Parsing successful!\n");
      {
        ScoreTree* s = scoTree;
        while (s) {
          printf("(%d,%s): opode = %c\n",
                 s->line, csound->filedir[s->locn&0xff], s->op);
          s = s->next;
        }
      }
    }
#endif
    return 0;
}
