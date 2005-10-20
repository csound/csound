#include "filebuilding.h"
#include <string>
#include <vector>
#include <map>

struct CsoundFile
{
        std::string options;
        std::string orchestra;
        std::vector<std::string> score;
};

static std::map<CSOUND *, CsoundFile> files;

#ifdef __cplusplus
extern "C" {
#endif

PUBLIC void csoundCsdCreate(CSOUND *csound)
        {
                CsoundFile csoundFile;
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

PUBLIC void csoundCsdAddEvent11(CSOUND *csound, double p1, double p2, double p3, double p4, double p5, double p6, double p7, double p8, double p9, double p10, double p11)
        {
                char buffer[0x100];
        std::string note = "i ";
        gcvt(p1, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p2, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p3, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p4, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p5, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p6, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p7, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p8, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p9, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p10, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p11, 10, buffer);
        note += buffer;
                files[csound].score.push_back(note);
        }

PUBLIC void csoundCsdAddEvent10(CSOUND *csound, double p1, double p2, double p3, double p4, double p5, double p6, double p7, double p8, double p9, double p10)
        {
                char buffer[0x100];
        std::string note = "i ";
        gcvt(p1, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p2, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p3, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p4, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p5, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p6, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p7, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p8, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p9, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p10, 10, buffer);
        note += buffer;
                files[csound].score.push_back(note);
        }

PUBLIC void csoundCsdAddEvent9(CSOUND *csound, double p1, double p2, double p3, double p4, double p5, double p6, double p7, double p8, double p9)
        {
                char buffer[0x100];
        std::string note = "i ";
        gcvt(p1, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p2, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p3, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p4, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p5, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p6, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p7, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p8, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p9, 10, buffer);
        note += buffer;
                files[csound].score.push_back(note);
        }

PUBLIC void csoundCsdAddEvent8(CSOUND *csound, double p1, double p2, double p3, double p4, double p5, double p6, double p7, double p8)
        {
                char buffer[0x100];
        std::string note = "i ";
        gcvt(p1, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p2, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p3, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p4, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p5, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p6, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p7, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p8, 10, buffer);
        note += buffer;
                files[csound].score.push_back(note);
        }

PUBLIC void csoundCsdAddEvent7(CSOUND *csound, double p1, double p2, double p3, double p4, double p5, double p6, double p7)
        {
                char buffer[0x100];
        std::string note = "i ";
        gcvt(p1, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p2, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p3, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p4, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p5, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p6, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p7, 10, buffer);
        note += buffer;
                files[csound].score.push_back(note);
        }

PUBLIC void csoundCsdAddEvent6(CSOUND *csound, double p1, double p2, double p3, double p4, double p5, double p6)
        {
                char buffer[0x100];
        std::string note = "i ";
        gcvt(p1, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p2, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p3, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p4, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p5, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p6, 10, buffer);
        note += buffer;
                files[csound].score.push_back(note);
        }

PUBLIC void csoundCsdAddEvent5(CSOUND *csound, double p1, double p2, double p3, double p4, double p5)
        {
                char buffer[0x100];
        std::string note = "i ";
        gcvt(p1, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p2, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p3, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p4, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p5, 10, buffer);
        note += buffer;
                files[csound].score.push_back(note);
        }

PUBLIC void csoundCsdAddEvent4(CSOUND *csound, double p1, double p2, double p3, double p4)
        {
                char buffer[0x100];
        std::string note = "i ";
        gcvt(p1, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p2, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p3, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p4, 10, buffer);
        note += buffer;
                files[csound].score.push_back(note);
        }

PUBLIC void csoundCsdAddEvent3(CSOUND *csound, double p1, double p2, double p3)
        {
                char buffer[0x100];
        std::string note = "i ";
        gcvt(p1, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p2, 10, buffer);
        note += buffer;
        note += " ";
        gcvt(p3, 10, buffer);
        note += buffer;
                files[csound].score.push_back(note);
        }

PUBLIC int csoundCsdSave(CSOUND *csound, char *filename)
        {
                CsoundFile &csoundFile = files[csound];
                FILE *file = fopen(filename, "w+");
                fprintf(file, "<CsoundSynthesizer>");
                fprintf(file, "<CsOptions>");
                fprintf(file, "%s", csoundFile.options.c_str());
                fprintf(file, "<CsoundSynthesizer>");
                fprintf(file, "<CsInstruments>");
                fprintf(file, "%s", csoundFile.orchestra.c_str());
                fprintf(file, "</CsInstruments>");
                fprintf(file, "<CsScore>");
                for (std::vector<std::string>::iterator it = csoundFile.score.begin(); it != csoundFile.score.end(); ++it) {
                        fprintf(file, it->c_str());
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

PUBLIC int csoundCompileCsd(CSOUND *csound, char *csdFilename)
{
        char *argv[2];
        argv[0] = "csound";
        argv[1] = csdFilename;
        return csoundCompile(csound, 2, argv);
}

PUBLIC int csoundPerformCsd(CSOUND *csound, char *csdFilename)
{
        char *argv[2];
        argv[0] = "csound";
        argv[1] = csdFilename;
        return csoundPerform(csound, 2, argv);
}

#ifdef __cplusplus
};
#endif

