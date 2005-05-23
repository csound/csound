#include <stdio.h>
#include <stdlib.h>
#include "jsnd.yacc.tab.h"
#include "jsnd.h"

typedef struct {
  char *opcode;
  char *ans;
  char *args;
} CHECKS;

typedef struct {
  char *opcode;
  char *args;
} CHECKS0;

/*
                i       init time
                k       kontrol rate
                a       audio rate
                w       omega
                f       pv-frame
                x       k or a
                S       string or init
                m       begins an indef list of iargs (any count)
                n       begins an indef list of iargs (nargs odd)
                o       optional, defaulting to 0
                p          "            "       1
                q          "            "       10
                v          "            "       .5
                j          "            "       -1
                h          "            "       127
                y       begins indef list of aargs (any count)
                z       begins indef list of kargs (any count)
                Z       begins alternating kakaka...list (any count)
   outarg types include:
                m       multiple outargs (1 to 16 allowed)
   (these types must agree with rdorch.c)                       */

CHECKS0 check0[] = {
  { "clear",	"y"},
  { "clockoff",	"i"},
  { "clockon",	"i"},
  { "cpuprc",	"ii"},
  { "ctrlinit",	"im"},
  { "delayw",	"a"},
  { "deltapxw",	"aai"},
  { "dispfft",	"siiooo"},
  { "display",	"sioo"},
  { "dumpk",	"kSii"},
  { "dumpk2",	"kkSii"},
  { "dumpk3",	"kkkSii"},
  { "dumpk4",	"kkkkSii"},
  { "event",	"Sz"},
  { "fin",	"Siiy"},
  { "fini",	"Siim"},
  { "fink",	"Siiz"},
  { "flashtxt",	"iS"},
  { "fout",	"Siy"},
  { "fouti",	"iiim"},
  { "foutir",	"iiim"},
  { "foutk",	"Siz"},
  { "ftmorf",	"kii"},
  { "initc14",	"iiii"},
  { "initc21",	"iiiii"},
  { "initc7",	"iii"},
  { "inz",	"k"},
  { "ktableseg","iin"},
  { "lpinterp",	"iik"},
  { "lpslot",	"i"},
  { "massign",	"ii"},
  { "maxalloc",	"ii"},
  { "mclock",	"i"},
  { "mdelay",	"kkkkk"},
  { "midion",	"kkk"},
  { "midion2",	"kkkk"},
  { "midiout",	"kkkk"},
  { "moscil",	"kkkkk"},
  { "mrtmsg",	"i"},
  { "noteoff",	"iii"},
  { "noteon",	"iii"},
  { "noteon",	"iiii"},
  { "nrpn",	"kkk"},
  { "out",	"a"},
  { "out32",	"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"},
  { "outc",	"y"},
  { "outch",	"Z"},
  { "outh",	"aaaaaa"},
  { "outiat",	"iiii"},
  { "outic",	"iiiii"},
  { "outic14",	"iiiiii"},
  { "outipat",  "iiiii"},
  { "outipb",	"iiii"},
  { "outipc",	"iiii"},
  { "outkat",	"kkkk"},
  { "outkc",	"kkkkk"},
  { "outkc14",	"kkkkkk"},
  { "outkpat",  "kkkkk"},
  { "outkpb",	"kkkk"},
  { "outkpc",	"kkkk"},
  { "outo",	"aaaaaaaa"},
  { "outq",	"aaaa"},
  { "outq1",	"a"},
  { "outq2",	"a"},
  { "outq3",	"a"},
  { "outq4",	"a"},
  { "outs",	"aa"},
  { "outs1",	"a"},
  { "outs2",	"a"},
  { "outx",	"aaaaaaaaaaaaaaaa"},
  { "outz",	"k"},
  { "prealloc",	"ii"},
  { "print",	"m"},
  { "printk",	"iko"},
  { "printk2",	"ko"},
  { "printks",	"Sikkkk"},
  { "pset",	"m"},
  { "pvsftr",	"fio"},
  { "reinit",	"l"},
  { "rigoto",	"l"},
  { "rireturn",	""},
  { "scanu",	"iiiiiiikkkkiikkaii"},
  { "schedkwhen","kkkkkz"},
  { "schedule",	"iiim"},
  { "schedwhen","kkkkm"},
  { "seed",	"i"},
  { "setctrl",	"iSi"},
  { "sfilist",	"i"},
  { "sfpassign","ii"},
  { "sfplist",	"i"},
  { "soundout",	"aSo"},
  { "spat3dt",	"iiiiiiiio"},
  { "specdisp",	"wio"},
  { "strset",	"iS"},
  { "tablecopy","kk"},
  { "tablegpw",	"k"},
  { "tableicopy","ii"},
  { "tableigpw","i"},
  { "tableimix","iiiiiiiii"},
  { "tableiw",	"iiiooo"},
  { "tablemix",	"kkkkkkkkk"},
  { "tableseg",	"iin"},
  { "tablew",	"aaiooo"},
  { "tablew",	"kkiooo"},
  { "tablewkt",	"aakooo"},
  { "tablewkt",	"kkkooo"},
  { "tablexseg","iin"},
  { "tempo",	"ki"},
  { "tigoto",	"l"},
  { "timout",	"iil"},
  { "trigseq",	"kkkkkz"},
  { "turnoff",	""},
  { "turnon",	"io"},
  { "vbaplsinit","iioooooooooooooooooooooooooooooooo"},
  { "vbapz",	"iiaioo"},
  { "vincr",	"aa"},
  { "xscanu",	"iiiiSiikkkkiikkaii"},
  { "xtratim",	"i"},
  { "zacl",	"kk"},
  { "zakinit",	"ii"},
  { "zaw",	"ak"},
  { "zawm",	"akp"},
  { "ziw",	"ii"},
  { "ziwm",	"iip"},
  { "zkcl",	"kk"},
  { "zkw",	"kk"},
  { "zkwm",	"kkp"},
  { "",		""}
};

CHECKS checks[] = {
  { "abs",	"a",	"a"},
  { "abs",	"i",	"i"},
  { "abs",	"k",	"k"},
  { "active",	"i",	""},
  { "active",	"k",	"k"},
  { "adsr",	"s",	"iiiio"},
  { "adsyn",	"a",	"kkkSo"},
  { "adsynt",	"a",	"kkiiiio"},
  { "aftouch",	"k",	"oh"},
  { "alpass",	"a",	"akioo"},
  { "ampdb",	"a",	"a"},
  { "ampdb",	"i",	"i"},
  { "ampdb",	"k",	"k"},
  { "ampdbfs",	"a",	"a"},
  { "ampdbfs",	"i",	"i"},
  { "ampdbfs",	"k",	"k"},
  { "ampmidi",	"i",	"io"},
  { "areson",	"a",	"akkoo"},
  { "aresonk",	"k",	"kkkpo"},
  { "atone",	"a",	"ako"},
  { "atonek",	"k",	"kko"},
  { "atonex",	"a",	"akoo"},
  { "babo",	"aa",	"akkkiiijj"},
  { "balance",	"a",	"aaqo"},
  { "bamboo",	"a",	"kioooooo"},
  { "bbcutm",	"a","aiiiiipop"},
  { "bbcuts",	"aa","aaiiiiipop"},
  { "betarand",	"a",	"kkk"},
  { "betarand",	"i",	"kkk"},
  { "betarand",	"k",	"kkk"},
  { "bexprnd",	"a",	"k"},
  { "bexprnd",	"i",	"k"},
  { "bexprnd",	"k",	"k"},
  { "biquad",	"a",	"akkkkkko"},
  { "biquada",	"a",	"aaaaaaao"},
  { "birnd",	"i",	"i"},
  { "birnd",	"k",	"k"},
  { "butbp",	"a",	"akko"},
  { "butbr",	"a",	"akko"},
  { "buthp",	"a",	"ako"},
  { "butlp",	"a",	"ako"},
  { "butterbp",	"a",	"akko"},
  { "butterbr",	"a",	"akko"},
  { "butterhp",	"a",	"ako"},
  { "butterlp",	"a",	"ako"},
  { "button",	"k",	"k"},
  { "buzz",	"a",	"xxkio"},
  { "cabasa",	"a",	"iiooo"},
  { "cauchy",	"a",	"k"},
  { "cauchy",	"i",	"k"},
  { "cauchy",	"k",	"k"},
  { "cent",	"a",	"a"},
  { "cent",	"i",	"i"},
  { "cent",	"k",	"k"},
  { "chanctrl",	"i",	"iioh"},
  { "chanctrl",	"k",	"iioh"},
  { "checkbox",	"k",	"k"},
  { "clip",	"a",	"aiiv"},
  { "comb",	"a",	"akioo"},
  { "control",	"k",	"k"},
  { "convle",	"mmmm",	"aSo"},
  { "convolve",	"mmmm",	"aSo"},
  { "cos",	"a",	"a"},
  { "cos",	"i",	"i"},
  { "cos",	"k",	"k"},
  { "cosh",	"a",	"a"},
  { "cosh",	"i",	"i"},
  { "cosh",	"k",	"k"},
  { "cosinv",	"a",	"a"},
  { "cosinv",	"i",	"i"},
  { "cosinv",	"k",	"k"},
  { "cps2pch",	"i",	"ii"},
  { "cpsmidi",	"i",	""},
  { "cpsmidib",	"i",	"o"},
  { "cpsmidib",	"k",	"o"},
  { "cpsoct",	"a",	"a"},
  { "cpsoct",	"i",	"i"},
  { "cpsoct",	"k",	"k"},
  { "cpspch",	"i",	"i"},
  { "cpspch",	"k",	"k"},
  { "cpstmid",	"i",	"i"},
  { "cpstun",	"k",	"kkk"},
  { "cpstuni",	"i",	"ii"},
  { "cpsxpch",	"i",	"iiii"},
  { "cross2",	"a",	"aaiiik"},
  { "crunch",	"a",	"iiooo"},
  { "ctrl14",	"i",	"iiiiio"},
  { "ctrl14",	"k",	"iiikko"},
  { "ctrl21",	"i",	"iiiiiio"},
  { "ctrl21",	"k",	"iiiikko"},
  { "ctrl7",	"i",	"iiiio"},
  { "ctrl7",	"k",	"iikko"},
  { "cuserrnd",	"a",	"kkk"},
  { "cuserrnd",	"k",	"kkk"},
  { "dam",	"a",	"akiiii"},
  { "db",	"a",	"a"},
  { "db",	"i",	"i"},
  { "db",	"k",	"k"},
  { "dbamp",	"i",	"i"},
  { "dbamp",	"k",	"k"},
  { "dbfsamp",	"i",	"i"},
  { "dbfsamp",	"k",	"k"},
  { "dcblock",	"a",	"ao"},
  { "dconv",	"a",	"aii"},
  { "delay",	"a",	"aio"},
  { "delay1",	"a",	"ao"},
  { "delayr",	"a",	"io"},
  { "deltap",	"a",	"k"},
  { "deltap3",	"a",	"x"},
  { "deltapi",	"a",	"x"},
  { "deltapn",	"a",	"x"},
  { "deltapx",	"a",	"ai"},
  { "diff",	"s",	"xo"},
  { "diskin",	"mmmm",	"Skooo"},
  { "distort1",	"a",	"appoo"},
  { "divz",	"a",	"aak"},
  { "divz",	"a",	"akk"},
  { "divz",	"a",	"kak"},
  { "divz",	"i",	"iii"},
  { "divz",	"k",	"kkk"},
  { "downsamp",	"k",	"ao"},
  { "dripwater","a",	"kioooooo"},
  { "duserrnd",	"a",	"k"},
  { "duserrnd",	"i",	"i"},
  { "duserrnd",	"k",	"k"},
  { "envlpx",	"s","xiiiiiio"},
  { "envlpxr",	"s","xiiiiioo"},
  { "exp",	"a",	"a"},
  { "exp",	"i",	"i"},
  { "exp",	"k",	"k"},
  { "expon",	"s",	"iii"},
  { "exprand",	"a",	"k"},
  { "exprand",	"i",	"k"},
  { "exprand",	"k",	"k"},
  { "expseg",	"s",	"iin"},
  { "expsega",	"a",	"iin"},
  { "expsegr",	"s",	"iin"},
  { "filelen",	"i",	"S"},
  { "filenchnls","i",	"S"},
  { "filepeak",	"i",	"So"},
  { "filesr",	"i",	"S"},
  { "filter2",	"a",	"aiim"},
  { "filter2",	"k",	"kiim"},
  { "fiopen",	"i",	"Si"},
  { "flanger",	"a",	"aakv"},
  { "fmb3",	"a",	"kkkkkkiiiii"},
  { "fmbell",	"a",	"kkkkkkiiiii"},
  { "fmmetal",	"a",	"kkkkkkiiiii"},
  { "fmpercfl",	"a",	"kkkkkkiiiii"},
  { "fmrhode",	"a",	"kkkkkkiiiii"},
  { "fmvoice",	"a",	"kkkkkkiiiii"},
  { "fmwurlie",	"a",	"kkkkkkiiiii"},
  { "fof",	"a","xxxkkkkkiiiioo"},
  { "fof2",	"a","xxxkkkkkiiiikk"},
  { "fog",	"a","xxxakkkkkiiiioo"},
  { "fold",	"a",	"ak"},
  { "follow",	"a",	"ai"},
  { "follow2",	"a",	"akk"},
  { "foscil",	"a",	"xkxxkio"},
  { "foscili",	"a",	"xkxxkio"},
  { "frac",	"i",	"i"},
  { "frac",	"k",	"k"},
  { "ftchnls",	"i",	"i"},
  { "ftgen",	"i",	"iiiiSm"},
  { "ftlen",	"i",	"i"},
  { "ftlptim",	"i",	"i"},
  { "ftsr",	"i",	"i"},
  { "gain",	"a",	"akqo"},
  { "gauss",	"a",	"k"},
  { "gauss",	"i",	"k"},
  { "gauss",	"k",	"k"},
  { "gbuzz",	"a",	"xxkkkio"},
  { "gogobel",	"a",	"kkiiikki"},
  { "grain",	"a",	"xxxkkkiiio"},
  { "grain2",	"a",	"kkkikiooo"},
  { "grain3",	"a",	"kkkkkkikikkoo"},
  { "granule",	"a",	"xiiiiiiiiikikiiivppppo"},
  { "guiro",	"a",	"kiooooo"},
  { "harmon",	"a",	"akkkkiii"},
  { "hilbert",	"aa",	"a"},
  { "hrtfer",	"aa",	"akkS"},
  { "hsboscil",	"a",	"kkkiiioo"},
  { "i",	"i",	"k"},
  { "in",	"a",	""},
  { "in32",	"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",	""},
  { "inch",	"a",	"k"},
  { "inh",	"aaaaaa",""},
  { "init",	"a",	"i"},
  { "init",	"i",	"i"},
  { "init",	"k",	"i"},
  { "ino",	"aaaaaaaa",""},
  { "inq",	"aaaa",	""},
  { "ins",	"aa",	""},
  { "int",	"i",	"i"},
  { "int",	"k",	"k"},
  { "integ",	"s",	"xo"},
  { "interp",	"a",	"ko"},
  { "inx",	"aaaaaaaaaaaaaaaa",""},
  { "jitter",	"k",	"kkk"},
  { "jitter2",	"k",	"kkkkkkk"},
  { "jspline",	"s",	"xkk"},
  { "lfo",	"s",	"kko"},
  { "limit",	"a",	"xkk"},
  { "limit",	"i",	"iii"},
  { "limit",	"k",	"xkk"},
  { "line",	"s",	"iii"},
  { "linen",	"s",	"xiii"},
  { "linenr",	"s",	"xiii"},
  { "lineto",	"k",	"kk"},
  { "linrand",	"a",	"k"},
  { "linrand",	"i",	"k"},
  { "linrand",	"k",	"k"},
  { "linseg",	"s",	"iin"},
  { "linsegr",	"s",	"iin"},
  { "locsend",	"mmmm",	""},
  { "locsig",	"mmmm",	"akkk"},
  { "log",	"a",	"a"},
  { "log",	"i",	"i"},
  { "log",	"k",	"k"},
  { "log10",	"a",	"a"},
  { "log10",	"i",	"i"},
  { "log10",	"k",	"k"},
  { "logbtwo",	"a",	"a"},
  { "logbtwo",	"i",	"i"},
  { "logbtwo",	"k",	"k"},
  { "loopseg",	"k",	"kkz"},
  { "lorenz",	"aaa",	"kkkkiiii"},
  { "loscil",	"mm","xkiojoojoo"},
  { "loscil3",	"mm","xkiojoojoo"},
  { "lowpass2",	"a",	"akko"},
  { "lowres",	"a",	"akko"},
  { "lowresx",	"a",	"akkoo"},
  { "lpf18",	"a",	"akkk"},
  { "lpfreson",	"a",	"ak"},
  { "lphasor",	"a",	"xooooooo"},
  { "lposcil",	"a",	"kkkkio"},
  { "lposcil3",	"a",	"kkkkio"},
  { "lpread",	"kkkk",	"kSoo"},
  { "lpreson",	"a",	"a"},
  { "lpshold",	"k",	"kkz"},
  { "mac",	"a",	"Z"},
  { "maca",	"a",	"y"},
  { "madsr",	"s",	"iiiioj"},
  { "mandol",	"a",	"kkkkkkio"},
  { "marimba",	"a",	"kkiiikkiioo"},
  { "midic14",	"i",	"iiiio"},
  { "midic14",	"k",	"iikko"},
  { "midic21",	"i",	"iiiiio"},
  { "midic21",	"k",	"iiikko"},
  { "midic7",	"i",	"iiio"},
  { "midic7",	"k",	"ikko"},
  { "midictrl",	"i",	"ioh"},
  { "midictrl",	"k",	"ioh"},
  { "midiin",	"kkkk",	""},
  { "mirror",	"i",	"iii"},
  { "mirror",	"s",	"xkk"},
  { "moog",	"a",	"kkkkkkiii"},
  { "moogvcf",	"a",	"axxp"},
  { "mpulse",	"a",	"kko"},
  { "multitap",	"a",	"am"},
  { "mxadsr",	"s",	"iiiioj"},
  { "nestedap",	"a",	"aiiiiooooo"},
  { "nlfilt",	"a",	"akkkkk"},
  { "noise",	"a",	"xk"},
  { "notnum",	"i",	""},
  { "nreverb",	"a",	"akkoojoj"},
  { "nsamp",	"i",	"i"},
  { "ntrpol",	"a",	"aakop"},
  { "ntrpol",	"i",	"iiiop"},
  { "ntrpol",	"k",	"kkkop"},
  { "octave",	"a",	"a"},
  { "octave",	"k",	"k"},
  { "octcps",	"i",	"i"},
  { "octcps",	"k",	"k"},
  { "octmidi",	"i",	""},
  { "octmidib",	"i",	"o"},
  { "octmidib",	"k",	"o"},
  { "octpch",	"i",	"i"},
  { "octpch",	"k",	"k"},
  { "oscbnk",	"a",	"kkkkiikkkkikkkkkkikooooooo"},
  { "oscil",	"a",	"aaio"},
  { "oscil",	"a",	"akio"},
  { "oscil",	"a",	"kaio"},
  { "oscil",	"s",	"kkio"},
  { "oscil1",	"k",	"ikii"},
  { "oscil1i",	"k",	"ikii"},
  { "oscil3",	"a",	"aaio"},
  { "oscil3",	"a",	"akio"},
  { "oscil3",	"a",	"kaio"},
  { "oscil3",	"s",	"kkio"},
  { "oscili",	"a",	"aaio"},
  { "oscili",	"a",	"akio"},
  { "oscili",	"a",	"kaio"},
  { "oscili",	"s",	"kkio"},
  { "osciln",	"a",	"kiii"},
  { "oscils",	"a",	"iiio"},
  { "oscilx",	"a",	"kiii"},
  { "p",	"i",	"i"},
  { "p",	"k",	"k"},
  { "pan",	"aaaa",	"akkioo"},
  { "pareq",	"a",	"akkko"},
  { "pcauchy",	"a",	"k"},
  { "pcauchy",	"i",	"k"},
  { "pcauchy",	"k",	"k"},
  { "pchbend",	"i",	"jp"},
  { "pchbend",	"k",	"jp"},
  { "pchmidi",	"i",	""},
  { "pchmidib",	"i",	"o"},
  { "pchmidib",	"k",	"o"},
  { "pchoct",	"i",	"i"},
  { "pchoct",	"k",	"k"},
  { "peak",	"k",	"a"},
  { "peak",	"k",	"k"},
  { "phaser1",	"a",	"akkko"},
  { "phaser2",	"a",	"akkkkkk"},
  { "phasor",	"s",	"xo"},
  { "phasorbnk","s",	"xkio"},
  { "pinkish",	"a",	"xoooo"},
  { "pitch",	"kk",	"aiiiiqooooojo"},
  { "pitchamdf","kk","aiioppoo"},
  { "planet",	"aaa",	"kkkiiiiiiio"},
  { "pluck",	"a",	"kkiiioo"},
  { "poisson",	"a",	"k"},
  { "poisson",	"i",	"k"},
  { "poisson",	"k",	"k"},
  { "polyaft",	"i",	"ioh"},
  { "polyaft",	"k",	"ioh"},
  { "port",	"k",	"kio"},
  { "portk",	"k",	"kko"},
  { "poscil",	"s",	"kkio"},
  { "poscil3",	"s",	"kkio"},
  { "pow",	"a",	"akp"},
  { "pow",	"i",	"ii"},
  { "pow",	"k",	"kkp"},
  { "powoftwo",	"a",	"a"},
  { "powoftwo",	"i",	"i"},
  { "powoftwo",	"k",	"k"},
  { "product",	"a",	"y"},
  { "pvadd",	"a",	"kkSiiopooo"},
  { "pvbufread","",	"kS"},
  { "pvcross",	"a",	"kkSkko"},
  { "pvinterp",	"a",	"kkSkkkkkk"},
  { "pvoc",	"a",	"kkSoooo"},
  { "pvread",	"kk",	"kSi"},
  { "pvsadsyn",	"a",	"fikopo"},
  { "pvsanal",	"f",	"aiiiioo"},
  { "pvscross",	"f",	"ffkk"},
  { "pvsfread",	"f",	"kSo"},
  { "pvsftw",	"k",	"fio"},
  { "pvsinfo",	"iiii","f"},
  { "pvsmaska",	"f",	"fik"},
  { "pvsynth",	"a",	"fo"},
  { "rand",	"s",	"xvoo"},
  { "randh",	"s",	"xxvoo"},
  { "randi",	"s",	"xxvoo"},
  { "random",	"a",	"kk"},
  { "random",	"i",	"ii"},
  { "random",	"k",	"kk"},
  { "randomh",	"s",	"kkx"},
  { "randomi",	"s",	"kkx"},
  { "readclock","i",	"i"},
  { "readk",	"k",	"Siio"},
  { "readk2",	"kk",	"Siio"},
  { "readk3",	"kkk",	"Siio"},
  { "readk4",	"kkkk",	"Siio"},
  { "release",	"k",	""},
  { "repluck",	"a",	"ikikka"},
  { "reson",	"a",	"akkoo"},
  { "resonk",	"k",	"kkkpo"},
  { "resonr",	"a",	"akkoo"},
  { "resonx",	"a",	"akkooo"},
  { "resony",	"a",	"akkikooo"},
  { "resonz",	"a",	"akkoo"},
  { "reverb",	"a",	"ako"},
  { "reverb2",	"a",	"akkoojoj"},
  { "rezzy",	"a",	"axxo"},
  { "rms",	"k",	"aqo"},
  { "rnd",	"i",	"i"},
  { "rnd",	"k",	"k"},
  { "rnd31",	"a",	"kko"},
  { "rnd31",	"i",	""},
  { "rnd31",	"k",	"kko"},
  { "rspline",	"s",	"xxkk"},
  { "rtclock",	"i",	""},
  { "rtclock",	"k",	""},
  { "s16b14",	"iiiiiiiiiiiiiiii",	"iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"},
  { "s16b14",	"kkkkkkkkkkkkkkkk",	"iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"},
  { "s32b14",	"iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii",	"iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"},
  { "s32b14",	"kkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkk",	"iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"},
  { "samphold",	"s",	"xxoo"},
  { "sandpaper","a",	"iiooo"},
  { "scans",	"a",	"kkiio"},
  { "sekere",	"a",	"iiooo"},
  { "semitone",	"a",	"a"},
  { "semitone",	"i",	"i"},
  { "semitone",	"k",	"k"},
  { "sense",	"k",	""},
  { "sensekey",	"k",	""},
  { "seqtime",	"k",	"kkkkk"},
  { "sfinstr",	"aa",	"iixxiio"},
  { "sfinstr3",	"aa",	"iixxiio"},
  { "sfinstr3m","a",	"iixxiio"},
  { "sfinstrm",	"a",	"iixxiio"},
  { "sfload",	"i",	"S"},
  { "sfplay",	"aa",	"iixxio"},
  { "sfplay3",	"aa",	"iixxio"},
  { "sfplay3m",	"a",	"iixxio"},
  { "sfplaym",	"a",	"iixxio"},
  { "sfpreset",	"i",	"iiii"},
  { "shaker",	"a",	"kkkkko"},
  { "sin",	"a",	"a"},
  { "sin",	"i",	"i"},
  { "sin",	"k",	"k"},
  { "sinh",	"a",	"a"},
  { "sinh",	"i",	"i"},
  { "sinh",	"k",	"k"},
  { "sininv",	"a",	"a"},
  { "sininv",	"i",	"i"},
  { "sininv",	"k",	"k"},
  { "sleighbells","a",  "kioooooo"},
  { "slider16",	"iiiiiiiiiiiiiiii",	"iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"},
  { "slider16",	"kkkkkkkkkkkkkkkk",	"iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"},
  { "slider16f","kkkkkkkkkkkkkkkk",	"iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"},
  { "slider32",	"iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii",	"iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"},
  { "slider32",	"kkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkk",	"iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"},
  { "slider32f","kkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkk",	"iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"},
  { "slider64",	"iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii",	"iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"},
  { "slider64",	"kkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkk",	"iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"},
  { "slider64f","kkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkk",	"iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"},
  { "slider8",	"kkkkkkkk",	"iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"},
  { "slider8",  "iiiiiiii",	"iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"},
  { "slider8f",	"kkkkkkkk",     "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"},
  { "sndwarp",	"mm",	"xxxiiiiiii"},
  { "sndwarpst","mmmm","xxxiiiiiii"},
  { "soundin",  "mmmmmmmmmmmmmmmmmmmmmmmm",	"Soo"},
  { "space",	"aaaa",	"aikkkk"},
  { "spat3d",	"aaaa",	"akkkiiiiio"},
  { "spat3di",	"aaaa",	"aiiiiiio"},
  { "spdist",	"k",	"ikkk"},
  { "specaddm",	"w",	"wwp"},
  { "specdiff",	"w",	"w"},
  { "specfilt",	"w",	"wi"},
  { "spechist",	"w",	"w"},
  { "specptrk",	"kk",	"wkiiiiiioqooo"},
  { "specscal",	"w",	"wii"},
  { "specsum",	"k",	"wo"},
  { "spectrum",	"w",	"siiiqoooo"},
  { "spsend",	"aaaa",	""},
  { "sqrt",	"a",	"a"},
  { "sqrt",	"i",	"i"},
  { "sqrt",	"k",	"k"},
  { "stix",	"a",	"iiooo"},
  { "streson",	"a",	"aki"},
  { "sum",	"a",	"y"},
  { "svfilter",	"aaa",	"akko"},
  { "table",	"i",	"iiooo"},
  { "table",	"s",	"xiooo"},
  { "table3",	"i",	"iiooo"},
  { "table3",	"s",	"xiooo"},
  { "tablei",	"i",	"iiooo"},
  { "tablei",	"s",	"xiooo"},
  { "tableikt",	"s",	"xkooo"},
  { "tablekt",	"s",	"xkooo"},
  { "tableng",	"i",	"i"},
  { "tableng",	"k",	"k"},
  { "tablera",	"a",	"kkk"},
  { "tablewa",	"k",	"kak"},
  { "tablexkt",	"a",	"xkkiooo"},
  { "tambourine","a",	"kioooooo"},
  { "tan",	"a",	"a"},
  { "tan",	"i",	"i"},
  { "tan",	"k",	"k"},
  { "tanh",	"a",	"a"},
  { "tanh",	"i",	"i"},
  { "tanh",	"k",	"k"},
  { "taninv",	"a",	"a"},
  { "taninv",	"i",	"i"},
  { "taninv",	"k",	"k"},
  { "taninv2",	"a",	"aa"},
  { "taninv2",	"i",	"ii"},
  { "taninv2",	"k",	"kk"},
  { "tbvcf",	"a",	"axxkk"},
  { "tempest",	"k","kiiiiiiiiiop"},
  { "tempoval",	"k",	""},
  { "timeinstk","k",	""},
  { "timeinsts","k",	""},
  { "timek",	"i",	""},
  { "timek",	"k",	""},
  { "times",	"i",	""},
  { "times",	"k",	""},
  { "tival",	"i",	""},
  { "tlineto",	"k",	"kkk"},
  { "tone",	"a",	"ako"},
  { "tonek",	"k",	"kko"},
  { "tonex",	"a",	"akoo"},
  { "transeg",	"s",	"iiim"},
  { "trigger",	"k",	"kkk"},
  { "trirand",	"a",	"k"},
  { "trirand",	"i",	"k"},
  { "trirand",	"k",	"k"},
  { "unirand",	"a",	"k"},
  { "unirand",	"i",	"k"},
  { "unirand",	"k",	"k"},
  { "upsamp",	"a",	"k"},
  { "urd",	"a",	"k"},
  { "urd",	"i",	"i"},
  { "urd",	"k",	"k"},
  { "valpass",	"a",	"axkioo"},
  { "vbap16",	"aaaaaaaaaaaaaaaa","aioo"},
  { "vbap16move","aaaaaaaaaaaaaaaa","aiiim"},
  { "vbap4",	"aaaa","aioo"},
  { "vbap4move","aaaa","aiiim"},
  { "vbap8",	"aaaaaaaa","aioo"},
  { "vbap8move","aaaaaaaa","aiiim"},
  { "vbapzmove","","iiaiiim"},
  { "vco",	"a",	"xxppppovo"},
  { "vcomb",	"a",	"axkioo"},
  { "vdelay",	"a",	"axio"},
  { "vdelay3",	"a",	"axio"},
  { "vdelayx",	"a",	"aaiio"},
  { "vdelayxq",	"aaaa",	"aaaaaiio"},
  { "vdelayxs",	"aa",	"aaaiio"},
  { "vdelayxw",	"a",	"aaiio"},
  { "vdelayxwq","aaaa",	"aaaaaiio"},
  { "vdelayxws","aa",	"aaaiio"},
  { "veloc",	"i",	"oh"},
  { "vibes",	"a",	"kkiiikkii"},
  { "vibr",	"k",	"kki"},
  { "vibrato",	"k",	"kkkkkkkkio"},
  { "vlowres",	"a",	"akkik"},
  { "voice",	"a",	"kkkkkkii"},
  { "vpvoc",	"a",	"kkSoo"},
  { "waveset",	"a",	"ako"},
  { "weibull",	"a",	"kk"},
  { "weibull",	"i",	"kk"},
  { "weibull",	"k",	"kk"},
  { "wgbow",	"a",	"kkkkkkio"},
  { "wgbowedbar","a",	"kkkkkoooo"},
  { "wgbrass",	"a",	"kkkikkio"},
  { "wgclar",	"a",	"kkkiikkkio"},
  { "wgflute",	"a",	"kkkiikkkiovv"},
  { "wgpluck",	"a",	"iikiiia"},
  { "wgpluck2",	"a",	"ikikk"},
  { "wguide1",	"a",	"axkk"},
  { "wguide2",	"a",	"axxkkkk"},
  { "wrap",	"i",	"iii"},
  { "wrap",	"s",	"xkk"},
  { "wterrain",	"a",	"kkkkkkii"},
  { "xadsr",	"s",	"iiiio"},
  { "xscans",	"a",	"kkiio"},
  { "xyin",	"kk",	"iiiiioo"},
  { "zamod",	"a",	"ak"},
  { "zar",	"a",	"k"},
  { "zarg",	"a",	"kk"},
  { "zfilter2",	"a",	"akkiim"},
  { "zir",	"i",	"i"},
  { "zkmod",	"k",	"kk"},
  { "zkr",	"k",	"k"},
  { "",		""}
};

int length_list(TREE* l)
{
    int len = 0;
    while (l!=NULL) {
      len += 1;
      l = l->left;
    }
    return len;
}

static int spread_args(TREE *args, TREE *av[], int n)
{
    if (args==NULL) return n;
    if (args->type != S_COM) {
      av[n] = args;
      return n+1;
    }
    else {
      n = spread_args(args->left, av, n);
      av[n] = args->right;
      free(args);               /* as now in array */
      return n+1;
    }
}

static TREE* collect_args(TREE *av[], int n)
{
    TREE *a;
    int i;
    if (n==0) return NULL;
    if (n==1) return av[0];
    a = make_node(S_COM, av[0], av[1]);
    for (i=2; i<n; i++)
      a = make_node(S_COM, a, av[i]);
    return a;
}

TREE* force_rate(TREE* a, char t)
{                               /* Ensure a is of type t */
    return a;
}

TREE* check_opcode(TREE *op, TREE* ans, TREE* args)
{
    /* Check that the number of ans and arguments is acceptable */
    printf("Ans length = %d\n", length_list(ans));
    printf("Arg length = %d\n", length_list(args));
    return args;
}

TREE *check_opcode0(TREE *op, TREE* args)
{
    /* Note that there is no multiple case here except the odd case of out */
    CHECKS0 *th = check0;
    TOKEN *xx = op->value;
    char *name;
    TREE *av[100], *a;
    int i, n;
    int len = length_list(args);
    char *atype;
    printf("check_opcode0\n");
    if (op->type == T_OPCODE0) {
      name = xx->lexeme;
    }
    else {
      switch (op->type) {
      case T_STRSET:
        name = "strset"; break;
      case T_PSET:
        name = "pset"; break;
      case T_CTRLINIT:
        name = "ctrlinit"; break;
      case T_MASSIGN:
        name = "massign"; break;
      case T_TURNON:
        name = "turnon"; break;
      case T_PREALLOC:
        name = "prealloc"; break;
      case T_ZAKINIT:
        name = "zakinit"; break;
      default:
        name = "???"; break;
      }
    }
    printf("name=%s type=%d\n", name, op->type);
    for (i=0; strlen(check0[i].opcode) != 0; i++) {
      printf("...looking at %s\n",check0[i].opcode);
      if (strcmp(check0[i].opcode,name)==0) goto found;
    }
    printf("Opcode not found -- collapse\n"); exit(1);
 found:
    n = 0;
    spread_args(args, av, 0);
    th = &check0[i];
    /* Check that the number of arguments is acceptable */
    printf("Arg length = %d\n", len);
    printf("match against %s\n", th->args);
    atype = th->args;
    while (*atype!='\0') {
      switch (*atype) {
      case 'i':       /* init time */
        if (n>len) goto tryagain;
        av[n] = force_rate(av[n], 'i');
        break;
      case 'k':       /* kontrol rate */
        if (n>len) goto tryagain;
        av[n] = force_rate(av[n], 'k');
        break;
      case 'a':       /* audio rate */
        if (n>len) goto tryagain;
        av[n] = force_rate(av[n], 'a');
        break;
      case 'w':       /* omega */
        if (n>len) goto tryagain;
        av[n] = force_rate(av[n], 'w');
        break;
      case 'f':       /* pv-frame */
        if (n>len) goto tryagain;
        av[n] = force_rate(av[n], 'f');
        break;
      case 'x':       /* k or a */
        if (n>len) goto tryagain;
        av[n] = force_rate(av[n], 'x');
        break;
      case 'S':       /* string or init */
        if (n>len) goto tryagain;
        av[n] = force_rate(av[n], 'S');
        break;
      case 'm':       /* begins an indef list of iargs (any count) */
        break;
      case 'n':       /* begins an indef list of iargs (nargs odd) */
        break;
      case 'o':       /* optional, defaulting to 0 */
        if (n>len) {
          av[n] = make_leaf(T_INTGR, make_int("0"));
        }
        else {
          av[n] = force_rate(av[n], 'i');
        }
        break;
      case 'p':       /*   "            "        1 */
        if (n>len) {
          av[n] = make_leaf(T_INTGR, make_int("1"));
        }
        else {
          av[n] = force_rate(av[n], 'i');
        }
        break;
      case 'q':       /*   "            "       10 */
        if (n>len) {
          av[n] = make_leaf(T_INTGR, make_int("10"));
        }
        else {
          av[n] = force_rate(av[n], 'i');
        }
        break;
      case 'v':       /*   "            "       .5 */
        if (n>len) {
          av[n] = make_leaf(T_INTGR, make_int("0.5"));
        }
        else {
          av[n] = force_rate(av[n], 'i');
        }
        break;
      case 'j':       /*   "            "       -1 */
        if (n>len) {
          av[n] = make_leaf(T_INTGR, make_int("-1"));
        }
        else {
          av[n] = force_rate(av[n], 'i');
        }
        break;
      case 'h':       /*   "            "      127 */
        if (n>len) {
          av[n] = make_leaf(T_INTGR, make_int("127"));
        }
        else {
          av[n] = force_rate(av[n], 'i');
        }
        break;
      case 'y':       /* begins indef list of aargs (any count) */
        break;
      case 'z':       /* begins indef list of kargs (any count) */
        break;
      case 'Z':       /* begins alternating kakaka...list (any count) */
        break;
      }
      n++; atype++;
    }
    if (n!=len) {
      printf("Additional arguments\n");
      exit(1);
    }
    return collect_args(av,n);
 tryagain:
    printf("Semantic failure\n");
    exit(1);
}

