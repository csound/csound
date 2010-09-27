#include <stdio.h>
extern int yyline;
extern int last_note;
extern int last_integer;
extern double last_duration;
extern double bpm;
extern int permeasure;
extern int yydebug;
FILE *myout;
FILE *yyin;
int debug = 0;

double pt[13] = { 8.1757989156,  8.6619572180,  9.1770239974,  9.7227182413,
                 10.3008611535, 10.9133822323, 11.5623257097, 12.2498573744,
                 12.9782717994, 13.7500000000, 14.5676175474, 15.4338631643,
                 16.3516084259
};

/* int input(void) */
/* { */
/*     int ch = getc(yyin); */
/*     return ch; */
/* } */

int main(int argc, char **argv)
{
    bpm = 60;
    permeasure = 4;
    yydebug = 0;
    if (argc==3) {
      yyin = fopen(argv[1], "r");
      myout = fopen(argv[2], "w");
    }
    else if (argc==4) {
      debug = 1;
      yyin = fopen(argv[2], "r");
      myout = fopen(argv[3], "w");
    }
    else if (argc==2) {
      debug = 1;
      myout = stdout;
    } else
      myout = stdout;
    yyparse();
    return 0;
}
