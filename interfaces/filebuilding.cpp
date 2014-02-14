
#include "filebuilding.h"
#include <stdlib.h>
#include <string>
#include <vector>
#include <map>

#ifdef MACOSX
#define gcvt(val,dig,buf) snprintf(buf,dig,"%f",val)
#endif

struct CsoundFile_
{
  std::string options;
  std::string orchestra;
  std::vector<std::string> score;
};

static std::map<CSOUND *, CsoundFile_> files;

#ifdef __cplusplus
extern "C" {
#endif
  uintptr_t perfthread(void *data){
    CSOUND *cs = (CSOUND *)data;
    int res = 0;
    while(res == 0) res = csoundPerformKsmps(cs);
    return 0;
  }

  PUBLIC void csoundNewCSD(char *path) {
    char *argv[2];
    CSOUND *instance;
   argv[0] = (char *)malloc(7);
   argv[1] = (char *)malloc(strlen(path)+1);
   strcpy(argv[0], "csound");
   strcpy(argv[1], path);

   //argv[0] = "csound";
   //argv[1] = path;

   printf("%s \n", argv[1]);
   instance = csoundCreate(NULL);
   csoundCompile(instance,2,argv);
   perfthread((void *) instance);
   csoundReset(instance);
   // csoundDestroy(instance);
   free(argv[0]);
   free(argv[1]);

  }


  PUBLIC int csoundPerformLoop(CSOUND *cs){
    csoundCreateThread(perfthread, (void *)cs);
    return 1;
  }

  PUBLIC void csoundCsdCreate(CSOUND *csound)
  {
    CsoundFile_ csoundFile;
    files[csound] = csoundFile;
  }

  PUBLIC void csoundCsdSetOptions(CSOUND *csound, char *options)
  {
    files[csound].options = options;
  }

  PUBLIC const char* csoundCsdGetOptions(CSOUND *csound)
  {
    return files[csound].options.c_str();
  }

  PUBLIC void csoundCsdSetOrchestra(CSOUND *csound, char *orchestra)
  {
    files[csound].orchestra = orchestra;
  }

  PUBLIC const char* csoundCsdGetOrchestra(CSOUND *csound)
  {
    return files[csound].orchestra.c_str();
  }

  PUBLIC void csoundCsdAddScoreLine(CSOUND *csound, char *line)
  {
    files[csound].score.push_back(line);
  }

  PUBLIC void csoundCsdAddEvent11(CSOUND *csound, double p1, double p2, double p3,
                                  double p4, double p5, double p6, double p7,
                                  double p8, double p9, double p10, double p11)
  {
    char note[0x100];
    sprintf(note, "i %-.10g %-.10g %-.10g %-.10g %-.10g %-.10g %-.10g %-.10g "
            "%-.10g %-.10g %-.10g", p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11);
    files[csound].score.push_back(note);
  }

  PUBLIC void csoundCsdAddEvent10(CSOUND *csound, double p1, double p2, double p3,
                                  double p4, double p5, double p6, double p7,
                                  double p8, double p9, double p10)
  {
    char note[0x100];
    sprintf(note, "i %-.10g %-.10g %-.10g %-.10g %-.10g %-.10g %-.10g %-.10g "
            "%-.10g %-.10g", p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);
    files[csound].score.push_back(note);
  }

  PUBLIC void csoundCsdAddEvent9(CSOUND *csound, double p1, double p2, double p3,
                                 double p4, double p5, double p6, double p7,
                                 double p8, double p9)
  {
    char note[0x100];
    sprintf(note, "i %-.10g %-.10g %-.10g %-.10g %-.10g %-.10g %-.10g %-.10g "
            "%-.10g", p1, p2, p3, p4, p5, p6, p7, p8, p9);
    files[csound].score.push_back(note);
  }

  PUBLIC void csoundCsdAddEvent8(CSOUND *csound, double p1, double p2, double p3,
                                 double p4, double p5, double p6, double p7,
                                 double p8)
  {
    char note[0x100];
    sprintf(note, "i %-.10g %-.10g %-.10g %-.10g %-.10g %-.10g %-.10g %-.10g",
            p1, p2, p3, p4, p5, p6, p7, p8);
    files[csound].score.push_back(note);
  }
  PUBLIC void csoundCsdAddEvent7(CSOUND *csound, double p1, double p2, double p3,
                                 double p4, double p5, double p6, double p7)
  {
    char note[0x100];
    sprintf(note, "i %-.10g %-.10g %-.10g %-.10g %-.10g %-.10g %-.10g",
            p1, p2, p3, p4, p5, p6, p7);
    files[csound].score.push_back(note);
  }

  PUBLIC void csoundCsdAddEvent6(CSOUND *csound, double p1, double p2, double p3,
                                 double p4, double p5, double p6)
  {
    char note[0x100];
    sprintf(note, "i %-.10g %-.10g %-.10g %-.10g %-.10g %-.10g",
            p1, p2, p3, p4, p5, p6);
    files[csound].score.push_back(note);
  }

  PUBLIC void csoundCsdAddEvent5(CSOUND *csound, double p1, double p2, double p3,
                                 double p4, double p5)
  {
    char note[0x100];
    sprintf(note, "i %-.10g %-.10g %-.10g %-.10g %-.10g", p1, p2, p3, p4, p5);
    files[csound].score.push_back(note);
  }

  PUBLIC void csoundCsdAddEvent4(CSOUND *csound, double p1, double p2, double p3,
                                 double p4)
  {
    char note[0x100];
    sprintf(note, "i %-.10g %-.10g %-.10g %-.10g", p1, p2, p3, p4);
    files[csound].score.push_back(note);
  }

  PUBLIC void csoundCsdAddEvent3(CSOUND *csound, double p1, double p2, double p3)
  {
    char note[0x100];
    sprintf(note, "i %-.10g %-.10g %-.10g", p1, p2, p3);
    files[csound].score.push_back(note);
  }

  PUBLIC int csoundCsdSave(CSOUND *csound, char *filename)
  {
    CsoundFile_ &csoundFile = files[csound];
    FILE *file = fopen(filename, "w+");
    fprintf(file, "<CsoundSynthesizer>");
    fprintf(file, "<CsOptions>");
    fprintf(file, "%s", csoundFile.options.c_str());
    fprintf(file, "<CsoundSynthesizer>");
    fprintf(file, "<CsInstruments>");
    fprintf(file, "%s", csoundFile.orchestra.c_str());
    fprintf(file, "</CsInstruments>");
    fprintf(file, "<CsScore>");
    for (std::vector<std::string>::iterator it = csoundFile.score.begin();
         it != csoundFile.score.end(); ++it) {
      fprintf(file, "%s", (it->c_str()));
    }
    fprintf(file, "</CsScore>");
    fprintf(file, "</CsoundSynthesizer>");
    return fclose(file);
  }

  PUBLIC int csoundCsdCompile(CSOUND *csound, char *filename)
  {
    csoundCsdSave(csound, filename);
    return csoundCompileCsd(csound, filename);
  }

  PUBLIC int csoundCsdPerform(CSOUND *csound, char *filename)
  {
    csoundCsdSave(csound, filename);
    return csoundPerformCsd(csound, filename);
  }

  /* VL: a new version of this function has been added to the main
     Csound library.
  PUBLIC int csoundCompileCsd(CSOUND *csound, char *csdFilename)
  {
    char *argv[2];
    argv[0] = (char*)"csound";
    argv[1] = csdFilename;
    return csoundCompile(csound, 2, argv);
  }
  */

  PUBLIC int csoundPerformCsd(CSOUND *csound, char *csdFilename)
  {
    int retval = csoundCompileCsd(csound, csdFilename);
    if (!retval)
      retval = csoundPerform(csound);
    csoundCleanup(csound);
    return (retval >= 0 ? 0 : retval);
  }

#ifdef __cplusplus
}
#endif

