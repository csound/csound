#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

int verbose = 1;

int process_file(char *input, FILE* outp)
{
    char buffer[126];
    char tag[64];
    char opcode[64];
    char *p, *q;

    FILE* inp = fopen(input, "r");
    if (inp == NULL) {
      fprintf(stderr, "Failed to open input\n");
      return 1;
    }
    // Find start of OENTRY table
    do {
      fgets(buffer, 120, inp);
      if (feof(inp)) {
        fclose(inp);
        return 0;
      }
    } while ((p = strstr(buffer, "OENTRY"))==NULL);

    // Process fior opcode/tag pairs
 gettag:


    do {
      fgets(buffer, 120, inp);
      if (feof(inp)) {
        fclose(inp);
        return 0;
      }
    } while ((p = strstr(buffer, "#ifdef INC_"))==NULL);
    strncpy(tag, p+11, 60);
    if (p=strchr(tag, '\r')) *p = '\0';
    if (p=strchr(tag, '\n')) *p = '\0';
    if (verbose) printf("Tag:>>%s<<\n", tag);
    // Read opcode
 opcode:
    fgets(buffer, 120, inp);
    if (p = strstr(buffer, "#endif")) goto gettag;
    if (p = strchr(buffer, '"')) {
      // Expect opcode
      q = strchr(p+1,'"');
      if (q) *(q+1) = '\0';
      if (verbose) printf("opcode>>%s<<\n", p+1);
      fprintf(outp, "\t{\"%s, \"INC_%s\", 0 },\n", p+1, tag);
      goto opcode;
    }
    goto opcode;
}

int main(void)
{
    FILE* outp = fopen("Dictionary.c", "w");
    if (outp == NULL) {
      fprintf(stderr, "Failed to open output\n");
      exit(1);
    }
    fprintf(outp, "typedef struct {\n\tchar* opcode;\n\tchar* tag;\n\tint data; } DICTIONARY;");
    fprintf(outp, "\nDICTIONARY dict[] = {\n");
    process_file("Engine/entry1.c", outp);
    process_file("Opcodes/afilters.c", outp);
    process_file("Opcodes/babo.c", outp);
    process_file("Opcodes/uggab.c", outp);
    process_file("Opcodes/scansyn.c", outp);
    process_file("Opcodes/ambicode1.c", outp);
    process_file("Opcodes/bbcut.c", outp);
    process_file("Opcodes/bilbar.c", outp);
    process_file("Opcodes/biquad.c", outp);
    process_file("Opcodes/ugmoss.c", outp);
    /* reverbsc */ fprintf(outp, "\t{\"reverbsc\", \"INC_REVERBSC\", 0 },\n");
    process_file("Opcodes/ugsc.c", outp);
    process_file("Opcodes/ugens7.c", outp);
    process_file("Opcodes/ugens9.c", outp);
    process_file("Opcodes/ugensa.c", outp);
    process_file("Opcodes/space.c", outp);
    process_file("Opcodes/spat3d.c", outp);
    process_file("Opcodes/squinewave.c", outp);
    process_file("Opcodes/vaops.c", outp);
    process_file("Opcodes/wave-terrain.c", outp);
    process_file("Opcodes/wterrain2.c", outp);
    process_file("Opcodes/sterrain.c", outp);
    process_file("Opcodes/sndwarp.c", outp);
    process_file("Opcodes/system_call.c", outp);
    process_file("Opcodes/tabsum.c", outp);
    process_file("Opcodes/repluck.c", outp);
    process_file("Opcodes/pinker.c", outp);
    process_file("Opcodes/wpfilters.c", outp);
    process_file("Opcodes/urandom.c", outp);
    // ugnorman is not minimal; ATS code all together
    process_file("Opcodes/ugnorman.c", outp);
    process_file("Opcodes/Vosim.c", outp);
    process_file("Opcodes/zak.c", outp);
    process_file("Opcodes/ugakbari.c", outp);
    process_file("Opcodes/partikkel.c", outp);
    process_file("Opcodes/paulstretch.c", outp);
    process_file("Opcodes/flanger.c", outp);
    process_file("Opcodes/tl/sc_noise.c", outp);
    process_file("Opcodes/date.c", outp);
    process_file("Opcodes/dcblockr.c", outp);
    process_file("Opcodes/dam.c", outp);
    process_file("Opcodes/butter.c", outp);
    process_file("Opcodes/follow.c", outp);
    process_file("Opcodes/pan2.c", outp);
    process_file("Opcodes/eqfil.c", outp);
    process_file("Opcodes/exciter.c", outp);
    process_file("Opcodes/cellular.c", outp);
    process_file("Opcodes/harmon.c", outp);
    process_file("Opcodes/gammatone.c", outp);
    process_file("Opcodes/gendy.c", outp);
    process_file("Opcodes/cpumeter.c", outp);
    process_file("Opcodes/metro.c", outp);
    process_file("Opcodes/compress.c", outp);
    process_file("Opcodes/linuxjoystick.c", outp);
    process_file("Opcodes/select.c", outp);
    process_file("Opcodes/seqtime.c", outp);
    process_file("Opcodes/pluck.c", outp);
    process_file("Opcodes/sequencer.c", outp);
    process_file("Opcodes/scoreline.c", outp);
    process_file("Opcodes/shape.c", outp);
    process_file("Opcodes/physmod.c", outp); /* Incomplete */
    process_file("Opcodes/phisem.c", outp);
    process_file("Opcodes/buchla.c", outp);
    
    fprintf(outp, "\t{ NULL, NULL}\n};\n");
}

