/*
    cseditor.cxx :

    Copyright (C) 2006 by David Akbari

    This file is part of Csound.

    The Csound Library is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Double_Window.H>
#include <FL/fl_ask.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Text_Editor.H>


int                changed = 0;
char               filename[256] = "";
char               title[256];
Fl_Text_Buffer     *textbuf = 0;


// Syntax highlighting stuff...
Fl_Text_Buffer     *stylebuf = 0;
Fl_Text_Display::Style_Table_Entry
styletable[] = {      // Style table
                 { FL_BLACK,      FL_COURIER,        14 }, // A - Plain
                 { FL_DARK_GREEN, FL_COURIER_ITALIC, 14 }, // B - Line comments
                 { FL_DARK_GREEN, FL_COURIER_ITALIC, 14 }, // C - Block comments
                 { FL_RED,        FL_COURIER,        14 }, // D - Strings
                 { FL_DARK_RED,   FL_COURIER,        14 }, // E - Directives
                 { FL_DARK_RED,   FL_COURIER_BOLD,   14 }, // F - Types
                 { FL_BLUE,       FL_COURIER_BOLD,   14 }  // G - Keywords
};

const char         *code_keywords[] = { // List of known C/C++ keywords...
            "ATSadd",
            "ATSaddnz",
            "ATSbufread",
            "ATScross",
            "ATSinfo",
            "ATSinterpread",
            "ATSpartialtap",
            "ATSread",
            "ATSreadnz",
            "ATSsinnoi",
            "MixerClear",
            "MixerGetLevel",
            "MixerReceive",
            "MixerSend",
            "MixerSetLevel",
            "OSCinit",
            "OSClisten",
            "OSCrecv",
            "OSCsend",
            "STKBandedWG",
            "STKBeeThree",
            "STKBlowBotl",
            "STKBlowHole",
            "STKBowed",
            "STKBrass",
            "STKClarinet",
            "STKDrummer",
            "STKFMVoices",
            "STKFlute",
            "STKHevyMetl",
            "STKMandolin",
            "STKModalBar",
            "STKMoog",
            "STKPercFlut",
            "STKPlucked",
            "STKResonate",
            "STKRhodey",
            "STKSaxofony",
            "STKShakers",
            "STKSimple",
            "STKSitar",
            "STKStifKarp",
            "STKTubeBell",
            "STKVoicForm",
            "STKWhistle",
            "STKWurley",
            "abs",
            "active",
            "add",
            "adsr",
            "adsyn",
            "adsynt",
            "adsynt2",
            "aftouch",
            "alpass",
            "ampdb",
            "ampdbfs",
            "ampmidi",
            "and",
            "areson",
            "aresonk",
            "atone",
            "atonek",
            "atonex",
            "babo",
            "balance",
            "bamboo",
            "barmodel",
            "bbcutm",
            "bbcuts",
            "betarand",
            "bexprnd",
            "bformdec",
            "bformenc",
            "binit",
            "biquad",
            "biquada",
            "birnd",
            "bqrez",
            "butbp",
            "butbr",
            "buthp",
            "butlp",
            "butterbp",
            "butterbr",
            "butterhp",
            "butterlp",
            "button",
            "buzz",
            "cabasa",
            "cauchy",
            "ceil",
            "cent",
            "cggoto",
            "chanctrl",
            "changed",
            "chani",
            "chano",
            "checkbox",
            "chn_S",
            "chn_a",
            "chn_k",
            "chnclear",
            "chnexport",
            "chnget",
            "chnmix",
            "chnparams",
            "chnrecv",
            "chnsend",
            "chnset",
            "cigoto",
            "ckgoto",
            "clear",
            "clfilt",
            "clip",
            "clockoff",
            "clockon",
            "cngoto",
            "cogoto",
            "comb",
            "compress",
            "control",
            "convle",
            "convolve",
            "cos",
            "cosh",
            "cosinv",
            "cps2pch",
            "cpsmidi",
            "cpsmidib",
            "cpsoct",
            "cpspch",
            "cpstmid",
            "cpstun",
            "cpstuni",
            "cpsxpch",
            "cpuprc",
            "cross2",
            "crunch",
            "ctrl",
            "ctrl7",
            "ctrl14",
            "ctrl21",
            "cuserrnd",
            "dam",
            "dbdbamp",
            "dbfsamp",
            "dcblock",
            "dconv",
            "delay",
            "delay1",
            "delayk",
            "delayr",
            "delayw",
            "deltap",
            "deltap3",
            "deltapi",
            "deltapn",
            "deltapx",
            "deltapxw",
            "denorm",
            "diff",
            "diskin",
            "diskin2",
            "dispfft",
            "display",
            "distort",
            "distort1",
            "div",
            "divz",
            "downsamp",
            "dripwater",
            "dumpk",
            "dumpk2",
            "dumpk3",
            "dumpk4",
            "duserrnd",
            "endin",
            "endop",
            "envlpx",
            "envlpxr",
            "event",
            "event_i",
            "exitnow",
            "exp",
            "expcurve",
            "expon",
            "exprand",
            "expseg",
            "expsega",
            "expsegr",
            "ficlose",
            "filelen",
            "filenchnls",
            "filepeak",
            "filesr",
            "filter2",
            "fin",
            "fini",
            "fink",
            "fiopen",
            "flanger",
            "flashtxt",
            "flooper",
            "floor",
            "fluidAllOut",
            "fluidCCi",
            "fluidCCk",
            "fluidControl",
            "fluidEngine",
            "fluidLoad",
            "fluidNote",
            "fluidOut",
            "fluidProgramSelect",
            "fmb3",
            "fmbell",
            "fmmetal",
            "fmpercfl",
            "fmrhode",
            "fmvoice",
            "fmwurlie",
            "fof",
            "fof2",
            "fofilter",
            "fog",
            "fold",
            "follow",
            "follow2",
            "foscil",
            "foscili",
            "fout",
            "fouti",
            "foutir",
            "foutk",
            "fprintks",
            "fprints",
            "frac",
            "freeverb",
            "ftchnls",
            "ftconv",
            "ftfree",
            "ftgentmp",
            "ftlen",
            "ftload",
            "ftloadk",
            "ftlptim",
            "ftmorf",
            "ftsave",
            "ftsavek",
            "ftsr",
            "gain",
            "gainslider",
            "gauss",
            "gbuzz",
            "getcfg",
            "gogobel",
            "goto",
            "grain",
            "grain2",
            "grain3",
            "granule",
            "guiro",
            "harmon",
            "hilbert",
            "hrtfer",
            "hsboscili",
            "igoto",
            "ihold",
            "in",
            "in32",
            "inch",
            "inh",
            "init",
            "initc14",
            "initc21",
            "initc7",
            "ino",
            "inq",
            "ins",
            "instr",
            "int",
            "integ",
            "interp",
            "invalue",
            "inx",
            "inz",
            "jitter",
            "jitter2",
            "jspline",
            "kgoto",
            "ktableseg",
            "lfo",
            "limit",
            "line",
            "linen",
            "linenr",
            "lineto",
            "linrand",
            "linseg",
            "linsegr",
            "locsend",
            "locsig",
            "log",
            "log10",
            "logbtwo",
            "logcurve",
            "loop_ge",
            "loop_gt",
            "loop_le",
            "loop_lt",
            "loopseg",
            "loopsegp",
            "lorenz",
            "lorismorph",
            "lorisplay",
            "lorisread",
            "loscil",
            "loscil3",
            "lowpass2",
            "lowres",
            "lowresx",
            "lpf18",
            "lpfreson",
            "lphasor",
            "lpinterp",
            "lposcil",
            "lposcil3",
            "lpread",
            "lpreson",
            "lpshold",
            "lpsholdp",
            "lpslot",
            "mac",
            "maca",
            "madsr",
            "mandel",
            "mandol",
            "marimba",
            "max",
            "max_k",
            "maxabs",
            "maxabsaccum",
            "maxaccum",
            "maxalloc",
            "maxk",
            "mclock",
            "mdelay",
            "metro",
            "midic14",
            "midic21",
            "midic7",
            "midichannelaftertouch",
            "midichn",
            "midicontrolchange",
            "midictrl",
            "mididefault",
            "midiin",
            "midinoteoff",
            "midinoteoncps",
            "midinoteonkey",
            "midinoteonoct",
            "midinoteonpch",
            "midion",
            "midion2",
            "midiout",
            "midipgm",
            "midipitchbend",
            "midipolyaftertouch",
            "midiprogramchange",
            "miditempo",
            "min",
            "minabs",
            "minabsaccum",
            "minaccum",
            "mirror",
            "mod",
            "mode",
            "monitor",
            "moog",
            "moogladder",
            "moogvcf",
            "moogvcf2",
            "moscil",
            "mpulse",
            "mrtmsg",
            "mul",
            "multitap",
            "mute",
            "mxadsr",
            "nestedap",
            "nlalp",
            "nlfilt",
            "noise",
            "not",
            "noteoff",
            "noteon",
            "noteondur",
            "noteondur2",
            "notnum",
            "nreverb",
            "nrpn",
            "nsamp",
            "nstrnum",
            "ntrpol",
            "octave",
            "octcps",
            "octmidi",
            "octmidib",
            "octpch",
            "opcode",
            "or",
            "oscbnk",
            "oscil",
            "oscil1",
            "oscil1i",
            "oscil3",
            "oscili",
            "oscilikt",
            "osciliktp",
            "oscilikts",
            "osciln",
            "oscils",
            "oscilv",
            "oscilx",
            "out",
            "out32",
            "outc",
            "outch",
            "outh",
            "outiat",
            "outic",
            "outic14",
            "outipat",
            "outipb",
            "outipc",
            "outkat",
            "outkc",
            "outkc14",
            "outkpat",
            "outkpb",
            "outkpc",
            "outo",
            "outq",
            "outq1",
            "outq2",
            "outq3",
            "outq4",
            "outs",
            "outs1",
            "outs2",
            "outvalue",
            "outx",
            "outz",
            "p",
            "pan",
            "pareq",
            "partials",
            "pcauchy",
            "pchbend",
            "pchmidi",
            "pchmidib",
            "pchoct",
            "pconvolve",
            "peak",
            "phaser1",
            "phaser2",
            "phasor",
            "phasorbnk",
            "pinkish",
            "pitch",
            "pitchamdf",
            "planet",
            "pluck",
            "poisson",
            "polyaft",
            "pop",
            "pop_f",
            "port",
            "portk",
            "poscil",
            "poscil3",
            "pow",
            "powoftwo",
            "prealloc",
            "prepiano",
            "print",
            "printf",
            "printf_i",
            "printk",
            "printk2",
            "printks",
            "prints",
            "product",
            "push",
            "push_f",
            "puts",
            "pvadd",
            "pvbufread",
            "pvcross",
            "pvinterp",
            "pvoc",
            "pvread",
            "pvsadsyn",
            "pvsanal",
            "pvsarp",
            "pvsbin",
            "pvsblur",
            "pvscale",
            "pvscent",
            "pvscross",
            "pvsdemix",
            "pvsdisp",
            "pvsfilter",
            "pvsfread",
            "pvsfreeze",
            "pvsftr",
            "pvsftw",
            "pvsfwrite",
            "pvshift",
            "pvsifd",
            "pvsin",
            "pvsinfo",
            "pvsinit",
            "pvsmaska",
            "pvsmix",
            "pvsmooth",
            "pvsosc",
            "pvsout",
            "pvspitch",
            "pvstencil",
            "pvsvoc",
            "pvsynth",
            "pyassign",
            "pyassigni",
            "pyassignt",
            "pycall",
            "pycall1",
            "pycall1i",
            "pycall1t",
            "pycall2",
            "pycall2i",
            "pycall2t",
            "pycall3",
            "pycall3i",
            "pycall3t",
            "pycall4",
            "pycall4i",
            "pycall4t",
            "pycall5",
            "pycall5i",
            "pycall5t",
            "pycall6",
            "pycall6i",
            "pycall6t",
            "pycall7",
            "pycall7i",
            "pycall7t",
            "pycall8",
            "pycall8i",
            "pycall8t",
            "pycalli",
            "pycalln",
            "pycallni",
            "pycallt",
            "pyeval",
            "pyevali",
            "pyevalt",
            "pyexec",
            "pyexeci",
            "pyexect",
            "pyinit",
            "pylassign",
            "pylassigni",
            "pylassignt",
            "pylcall",
            "pylcall1",
            "pylcall1i",
            "pylcall1t",
            "pylcall2",
            "pylcall2i",
            "pylcall2t",
            "pylcall3",
            "pylcall3i",
            "pylcall3t",
            "pylcall4",
            "pylcall4i",
            "pylcall4t",
            "pylcall5",
            "pylcall5i",
            "pylcall5t",
            "pylcall6",
            "pylcall6i",
            "pylcall6t",
            "pylcall7",
            "pylcall7i",
            "pylcall7t",
            "pylcall8",
            "pylcall8i",
            "pylcall8t",
            "pylcalli",
            "pylcalln",
            "pylcallni",
            "pylcallt",
            "pyleval",
            "pylevali",
            "pylevalt",
            "pylexec",
            "pylexeci",
            "pylexect",
            "pylrun",
            "pylruni",
            "pylrunt",
            "pyrun",
            "pyruni",
            "pyrunt",
            "rand",
            "randh",
            "randi",
            "random",
            "randomh",
            "randomi",
            "rbjeq",
            "readclock",
            "readk",
            "readk2",
            "readk3",
            "readk4",
            "reinit",
            "release",
            "remove",
            "repluck",
            "reson",
            "resonk",
            "resonr",
            "resonx",
            "resonxk",
            "resony",
            "resonz",
            "resyn",
            "reverb",
            "reverb2",
            "reverbsc",
            "rezzy",
            "rigoto",
            "rireturn",
            "rms",
            "rnd",
            "rnd31",
            "round",
            "rspline",
            "rtclock",
            "s16b14",
            "s32b14",
            "samphold",
            "sandpaper",
            "scale",
            "scanhammer",
            "scans",
            "scantable",
            "scanu",
            "schedkwhen",
            "schedkwhennamed",
            "schedule",
            "schedwhen",
            "sekere",
            "semitone",
            "sense",
            "sensekey",
            "seqtime",
            "seqtime2",
            "setctrl",
            "setksmps",
            "sfilist",
            "sfinstr",
            "sfinstr3",
            "sfinstr3m",
            "sfinstrm",
            "sfload",
            "sfpassign",
            "sfplay",
            "sfplay3",
            "sfplay3m",
            "sfplaym",
            "sfplist",
            "sfpreset",
            "shaker",
            "shl",
            "shr",
            "sin",
            "sinh",
            "sininv",
            "sinsyn",
            "sleighbells",
            "slider16",
            "slider16f",
            "slider32",
            "slider32f",
            "slider64",
            "slider64f",
            "slider8",
            "slider8f",
            "sndloop",
            "sndwarp",
            "sndwarpst",
            "sockrecv",
            "sockrecvs",
            "socksend",
            "socksends",
            "soundin",
            "soundout",
            "soundouts",
            "space",
            "spat3d",
            "spat3di",
            "spat3dt",
            "spdist",
            "specaddm",
            "specdiff",
            "specdisp",
            "specfilt",
            "spechist",
            "specptrk",
            "specscal",
            "specsum",
            "spectrum",
            "splitrig",
            "sprintf",
            "sprintfk",
            "spsend",
            "sqrt",
            "stack",
            "statevar",
            "stix",
            "strcat",
            "strcatk",
            "strchar",
            "strchark",
            "strcmp",
            "strcmpk",
            "strcpy",
            "strcpyk",
            "strecv",
            "streson",
            "strget",
            "strindex",
            "strindexk",
            "strlen",
            "strlenk",
            "strlower",
            "strlowerk",
            "strrindex",
            "strrindexk",
            "strsub",
            "strsubk",
            "strtod",
            "strtodk",
            "strtol",
            "strtolk",
            "strupper",
            "strupperk",
            "stsend",
            "sub",
            "subinstr",
            "subinstrinit",
            "sum",
            "svfilter",
            "syncgrain",
            "tab",
            "tab_i",
            "table",
            "table3",
            "tablecopy",
            "tablegpw",
            "tablei",
            "tableicopy",
            "tableigpw",
            "tableikt",
            "tableimix",
            "tableiw",
            "tablekt",
            "tablemix",
            "tableng",
            "tablera",
            "tableseg",
            "tablew",
            "tablewa",
            "tablewkt",
            "tablexkt",
            "tablexseg",
            "tabplay",
            "tabrec",
            "tabw",
            "tabw_i",
            "tambourine",
            "tan",
            "tanh",
            "taninv",
            "taninv2",
            "tb0",
            "tb0_init",
            "tb1",
            "tb10",
            "tb10_init",
            "tb11",
            "tb11_init",
            "tb12",
            "tb12_init",
            "tb13",
            "tb13_init",
            "tb14",
            "tb14_init",
            "tb15",
            "tb15_init",
            "tb1_init",
            "tb2",
            "tb2_init",
            "tb3",
            "tb3_init",
            "tb4",
            "tb4_init",
            "tb5",
            "tb5_init",
            "tb6",
            "tb6_init",
            "tb7",
            "tb7_init",
            "tb8",
            "tb8_init",
            "tb9",
            "tb9_init",
            "tbvcf",
            "tempest",
            "tempo",
            "tempoval",
            "tigoto",
            "timedseq",
            "timeinstk",
            "timeinsts",
            "timek",
            "times",
            "timout",
            "tival",
            "tlineto",
            "tone",
            "tonek",
            "tonex",
            "tradsyn",
            "transeg",
            "trcross",
            "trfilter",
            "trhighest",
            "trigger",
            "trigseq",
            "trirand",
            "trlowest",
            "trmix",
            "trscale",
            "trshift",
            "trsplit",
            "turnoff",
            "turnoff2",
            "turnon",
            "unirand",
            "upsamp",
            "urd",
            "vadd",
            "vadd_i",
            "vaddv",
            "vaddv_i",
            "vaget",
            "valpass",
            "vaset",
            "vbap16",
            "vbap16move",
            "vbap4",
            "vbap4move",
            "vbap8",
            "vbap8move",
            "vbaplsinit",
            "vbapz",
            "vbapzmove",
            "vcella",
            "vco",
            "vco2",
            "vco2ft",
            "vco2ift",
            "vco2init",
            "vcomb",
            "vcopy",
            "vcopy_i",
            "vdel_k",
            "vdelay",
            "vdelay3",
            "vdelayk",
            "vdelayx",
            "vdelayxq",
            "vdelayxs",
            "vdelayxw",
            "vdelayxwq",
            "vdelayxws",
            "vdivv",
            "vecdelay",
            "veloc",
            "vexp",
            "vexpseg",
            "vexpv",
            "vibes",
            "vibr",
            "vibrato",
            "vincr",
            "vlimit",
            "vlinseg",
            "vlowres",
            "vmap",
            "vmirror",
            "vmult",
            "vmultv",
            "voice",
            "vport",
            "vpow",
            "vpowv",
            "vpvoc",
            "vrandh",
            "vrandi",
            "vsubv",
            "vtaba",
            "vtabi",
            "vtabk",
            "vtablea",
            "vtablei",
            "vtablek",
            "vtablewa",
            "vtablewi",
            "vtablewk",
            "vtabwa",
            "vtabwi",
            "vtabwk",
            "vwrap",
            "waveset",
            "weibull",
            "wgbow",
            "wgbowedbar",
            "wgbrass",
            "wgclar",
            "wgflute",
            "wgpluck",
            "wgpluck2",
            "wguide1",
            "wguide2",
            "wrap",
            "wterrain",
            "xadsr",
            "xin",
            "xor",
            "xout",
            "xscanmap",
            "xscans",
            "xscansmap",
            "xscanu",
            "xtratim",
            "xyin",
            "zacl",
            "zakinit",
            "zamod",
            "zar",
            "zarg",
            "zaw",
            "zawm",
            "zfilter2",
            "zir",
            "ziw",
            "ziwm",
            "zkcl",
            "zkmod",
            "zkr",
            "zkw",
            "zkwm"
                   };

const char         *code_types[] = {     // csd tags
                   "FLbox",
                   "FLbutBank",
                   "FLbutton",
                   "FLcolor",
                   "FLcolor2",
                   "FLcount",
                   "FLgetsnap",
                   "FLgroup",
                   "FLgroupEnd",
                   "FLgroup_end",
                   "FLhide",
                   "FLjoy",
                   "FLkeyb",
                   "FLknob",
                   "FLlabel",
                   "FLloadsnap",
                   "FLpack",
                   "FLpackEnd",
                   "FLpack_end",
                   "FLpanel",
                   "FLpanelEnd",
                   "FLpanel_end",
                   "FLprintk",
                   "FLprintk2",
                   "FLroller",
                   "FLrun",
                   "FLsavesnap",
                   "FLscroll",
                   "FLscrollEnd",
                   "FLscroll_end",
                   "FLsetAlign",
                   "FLsetBox",
                   "FLsetColor",
                   "FLsetColor2",
                   "FLsetFont",
                   "FLsetPosition",
                   "FLsetSize",
                   "FLsetText",
                   "FLsetTextColor",
                   "FLsetTextSize",
                   "FLsetTextType",
                   "FLsetVal",
                   "FLsetVal_i",
                   "FLsetVali",
                   "FLsetsnap",
                   "FLshow",
                   "FLslidBnk",
                   "FLslider",
                   "FLtabs",
                   "FLtabsEnd",
                   "FLtabs_end",
                   "FLtext",
                   "FLupdate",
                   "FLvalue",
                   "ctrlinit",
                   "else",
                   "elseif",
                   "endif",
                   "ftgen",
                   "if",
                   "kr",
                   "ksmps",
                   "massign",
                   "nchnls",
                   "pgmassign",
                   "pset",
                   "seed",
                   "sr",
                   "strset",
                   "then"
                   };


//
// 'compare_keywords()' - Compare two keywords...
//

extern "C" {
  int
  compare_keywords(const void *a,
                   const void *b) {
      return (strcmp(*((const char **)a), *((const char **)b)));
  }
}

//
// 'style_parse()' - Parse text and produce style data.
//

void
style_parse(const char *text,
            char       *style,
            int        length) {
    char        current;
    int         col;
    int         last;
    char        buf[255], *bufptr;
    const char *temp;

 // Style letters:
 //
 // A - Plain
 // B - Line comments
 // C - Block comments
 // D - Strings
 // E - Directives
 // F - Types
 // G - Keywords

   for (current = *style, col = 0, last = 0; length > 0; length --, text ++) {
     if (current == 'B' || current == 'F' || current == 'G') current = 'A';
     if (current == 'A') {
       // Check for directives, comments, strings, and keywords...
       if (col == 0 && (*text == '#' || *text == '<')) {
         // Set style to directive
         current = 'E';
       } else if (strncmp(text, ";", 1) == 0) {
         current = 'B';
         for (; length > 0 && *text != '\n'; length --, text ++) *style++ = 'B';

         if (length == 0) break;
       } else if (strncmp(text, "/*", 2) == 0) {
         current = 'C';
       } else if (strncmp(text, "\\\"", 2) == 0) {
         // Quoted quote...
         *style++ = current;
         *style++ = current;
         text ++;
         length --;
         col += 2;
         continue;
       } else if (*text == '\"') {
         current = 'D';
       } else if (!last && (( islower(*text) || isupper(*text) ) ||
                            *text == '_')) {
         // Might be a keyword...
         for (temp = text, bufptr = buf;
              (( islower(*temp)   ||
                 isupper(*temp) ) ||
               *temp == '_') && bufptr < (buf + sizeof(buf) - 1);
             *bufptr++ = *temp++);

       if (!( islower(*temp) || isupper(*temp) ) && *temp != '_') {
          *bufptr = '\0';

         bufptr = buf;

          if (bsearch(&bufptr, code_types,
                      sizeof(code_types) / sizeof(code_types[0]),
                      sizeof(code_types[0]), compare_keywords)) {
            while (text < temp) {
              *style++ = 'F';
              text ++;
              length --;
              col ++;
            }

            text --;
            length ++;
            last = 1;
            continue;
          } else if (bsearch(&bufptr, code_keywords,
                             sizeof(code_keywords) / sizeof(code_keywords[0]),
                             sizeof(code_keywords[0]), compare_keywords)) {
            while (text < temp) {
              *style++ = 'G';
              text ++;
              length --;
              col ++;
            }

            text --;
            length ++;
            last = 1;
            continue;
          }
        }
     }
   } else if (current == 'C' && strncmp(text, "*/", 2) == 0) {
     // Close a C comment...
     *style++ = current;
     *style++ = current;
     text ++;
     length --;
     current = 'A';
     col += 2;
     continue;
   } else if (current == 'D') {
     // Continuing in string...
     if (strncmp(text, "\\\"", 2) == 0) {
       // Quoted end quote...
        *style++ = current;
        *style++ = current;
        text ++;
        length --;
        col += 2;
        continue;
     } else if (*text == '\"') {
       // End quote...
        *style++ = current;
        col ++;
        current = 'A';
        continue;
     }
   }

   // Copy style info...
   if (current == 'A' && (*text == '{' || *text == '}')) *style++ = 'G';
   else *style++ = current;
   col ++;

   last = isalnum(*text) || *text == '_' || *text == '.';

   if (*text == '\n') {
     // Reset column and possibly reset the style
     col = 0;
     if (current == 'B' || current == 'E') current = 'A';
   }
 }
}


//
// 'style_init()' - Initialize the style buffer...
//

void
style_init(void) {
    char *style = new char[textbuf->length() + 1];
    char *text = textbuf->text();


    memset(style, 'A', textbuf->length());
    style[textbuf->length()] = '\0';

    if (!stylebuf) stylebuf = new Fl_Text_Buffer(textbuf->length());

    style_parse(text, style, textbuf->length());

    stylebuf->text(style);
    delete[] style;
    free(text);
}


//
// 'style_unfinished_cb()' - Update unfinished styles.
//

void
style_unfinished_cb(int, void*) {
}


//
// 'style_update()' - Update the style buffer...
//

void
style_update(int        pos,            // I - Position of update
             int        nInserted,       // I - Number of inserted chars
             int        nDeleted,       // I - Number of deleted chars
             int        /*nRestyled*/,   // I - Number of restyled chars
             const char * /*deletedText*/,// I - Text that was deleted
             void       *cbArg) {        // I - Callback data
    int    start,                          // Start of text
           end;                            // End of text
    char   last,                           // Last style on line
           *style,                         // Style data
           *text;                          // Text data


 // If this is just a selection change, just unselect the style buffer...
    if (nInserted == 0 && nDeleted == 0) {
      stylebuf->unselect();
      return;
    }

    // Track changes in the text buffer...
    if (nInserted > 0) {
      // Insert characters into the style buffer...
      style = new char[nInserted + 1];
      memset(style, 'A', nInserted);
      style[nInserted] = '\0';

      stylebuf->replace(pos, pos + nDeleted, style);
      delete[] style;
    } else {
      // Just delete characters in the style buffer...
      stylebuf->remove(pos, pos + nDeleted);
    }

    // Select the area that was just updated to avoid unnecessary
    // callbacks...
    stylebuf->select(pos, pos + nInserted - nDeleted);

    // Re-parse the changed region; we do this by parsing from the
    // beginning of the previous line of the changed region to the end of
    // the line of the changed region...  Then we check the last
    // style character and keep updating if we have a multi-line
    // comment character...
    start = textbuf->line_start(pos);
    //  if (start > 0) start = textbuf->line_start(start - 1);
    end   = textbuf->line_end(pos + nInserted);
    text  = textbuf->text_range(start, end);
    style = stylebuf->text_range(start, end);
    if (start==end)
      last = 0;
    else
      last  = style[end - start - 1];

//  printf("start = %d, end = %d, text = \"%s\", style = \"%s\", last='%c'...\n",
//         start, end, text, style, last);

    style_parse(text, style, end - start);

//  printf("new style = \"%s\", new last='%c'...\n",
//         style, style[end - start - 1]);

    stylebuf->replace(start, end, style);
    ((Fl_Text_Editor *)cbArg)->redisplay_range(start, end);

    if (start==end || last != style[end - start - 1]) {
//    printf("Recalculate the rest of the buffer style\n");
   // Either the user deleted some text, or the last character
   // on the line changed styles, so reparse the
   // remainder of the buffer...
      free(text);
      free(style);

      end   = textbuf->length();
      text  = textbuf->text_range(start, end);
      style = stylebuf->text_range(start, end);

      style_parse(text, style, end - start);

      stylebuf->replace(start, end, style);
      ((Fl_Text_Editor *)cbArg)->redisplay_range(start, end);
    }

    free(text);
    free(style);
}


// Editor window functions and class...
void save_cb();
void saveas_cb();
void find2_cb(Fl_Widget*, void*);
void replall_cb(Fl_Widget*, void*);
void replace2_cb(Fl_Widget*, void*);
void replcan_cb(Fl_Widget*, void*);

class EditorWindow : public Fl_Double_Window {
public:
  EditorWindow(int w, int h, const char* t);
  ~EditorWindow();

  Fl_Window          *replace_dlg;
  Fl_Input           *replace_find;
  Fl_Input           *replace_with;
  Fl_Button          *replace_all;
  Fl_Return_Button   *replace_next;
  Fl_Button          *replace_cancel;

  Fl_Text_Editor     *editor;
  char               search[256];
};

EditorWindow::EditorWindow(int w, int h, const char* t) :
  Fl_Double_Window(w, h, t) {
    replace_dlg = new Fl_Window(300, 105, "Replace");
    replace_find = new Fl_Input(80, 10, 210, 25, "Find:");
    replace_find->align(FL_ALIGN_LEFT);

    replace_with = new Fl_Input(80, 40, 210, 25, "Replace:");
    replace_with->align(FL_ALIGN_LEFT);

    replace_all = new Fl_Button(10, 70, 90, 25, "Replace All");
    replace_all->callback((Fl_Callback *)replall_cb, this);

    replace_next = new Fl_Return_Button(105, 70, 120, 25, "Replace Next");
    replace_next->callback((Fl_Callback *)replace2_cb, this);

    replace_cancel = new Fl_Button(230, 70, 60, 25, "Cancel");
    replace_cancel->callback((Fl_Callback *)replcan_cb, this);
    replace_dlg->end();
    replace_dlg->set_non_modal();
    editor = 0;
    *search = (char)0;
}

EditorWindow::~EditorWindow() {
    delete replace_dlg;
}

int check_save(void) {
    if (!changed) return 1;

    int r = fl_choice("The current file has not been saved.\n"
                      "Would you like to save it now?",
                      "Cancel", "Save", "Don't Save");

    if (r == 1) {
      save_cb(); // Save the file...
      return !changed;
    }

    return (r == 2) ? 1 : 0;
}

int loading = 0;
void load_file(char *newfile, int ipos) {
    loading = 1;
    int insert = (ipos != -1);
    changed = insert;
    if (!insert) strcpy(filename, "");
    int r;
    if (!insert) r = textbuf->loadfile(newfile);
    else r = textbuf->insertfile(newfile, ipos);
    if (r)
      fl_alert("Error reading from file \'%s\':\n%s.", newfile, strerror(errno));
    else
      if (!insert) strcpy(filename, newfile);
    loading = 0;
    textbuf->call_modify_callbacks();
}

void save_file(char *newfile) {
    if (textbuf->savefile(newfile))
      fl_alert("Error writing to file \'%s\':\n%s.", newfile, strerror(errno));
    else
      strcpy(filename, newfile);
    changed = 0;
    textbuf->call_modify_callbacks();
}

void copy_cb(Fl_Widget*, void* v) {
    EditorWindow* e = (EditorWindow*)v;
    Fl_Text_Editor::kf_copy(0, e->editor);
}

void cut_cb(Fl_Widget*, void* v) {
    EditorWindow* e = (EditorWindow*)v;
    Fl_Text_Editor::kf_cut(0, e->editor);
}

void delete_cb(Fl_Widget*, void*) {
    textbuf->remove_selection();
}

void find_cb(Fl_Widget* w, void* v) {
    EditorWindow* e = (EditorWindow*)v;
    const char *val;

    val = fl_input("Search String:", e->search);
    if (val != NULL) {
      // User entered a string - go find it!
      strcpy(e->search, val);
      find2_cb(w, v);
    }
}

void find2_cb(Fl_Widget* w, void* v) {
    EditorWindow* e = (EditorWindow*)v;
    if (e->search[0] == '\0') {
      // Search string is blank; get a new one...
      find_cb(w, v);
      return;
    }

    int pos = e->editor->insert_position();
    int found = textbuf->search_forward(pos, e->search, &pos);
    if (found) {
      // Found a match; select and update the position...
      textbuf->select(pos, pos+strlen(e->search));
      e->editor->insert_position(pos+strlen(e->search));
      e->editor->show_insert_position();
    }
    else fl_alert("No occurrences of \'%s\' found!", e->search);
}

void set_title(Fl_Window* w) {
    if (filename[0] == '\0') strcpy(title, "Untitled");
    else {
      char *slash;
      slash = strrchr(filename, '/');
#ifdef WIN32
      if (slash == NULL) slash = strrchr(filename, '\\');
#endif
      if (slash != NULL) strcpy(title, slash + 1);
      else strcpy(title, filename);
    }

    if (changed) strcat(title, " (modified)");

    w->label(title);
}

void changed_cb(int, int nInserted, int nDeleted,int, const char*, void* v) {
    if ((nInserted || nDeleted) && !loading) changed = 1;
    EditorWindow *w = (EditorWindow *)v;
    set_title(w);
    if (loading) w->editor->show_insert_position();
}

void new_cb(Fl_Widget*, void*) {
    if (!check_save()) return;

    filename[0] = '\0';
    textbuf->select(0, textbuf->length());
    textbuf->remove_selection();
    changed = 0;
    textbuf->call_modify_callbacks();
}

void open_cb(Fl_Widget*, void*) {
    if (!check_save()) return;

    char *newfile = fl_file_chooser("Open File?", "*", filename);
    if (newfile != NULL) load_file(newfile, -1);
}

void insert_cb(Fl_Widget*, void *v) {
    char *newfile = fl_file_chooser("Insert File?", "*", filename);
    EditorWindow *w = (EditorWindow *)v;
    if (newfile != NULL) load_file(newfile, w->editor->insert_position());
}

void paste_cb(Fl_Widget*, void* v) {
    EditorWindow* e = (EditorWindow*)v;
    Fl_Text_Editor::kf_paste(0, e->editor);
}

int num_windows = 0;

void close_cb(Fl_Widget*, void* v) {
    Fl_Window* w = (Fl_Window*)v;
    if (num_windows == 1 && !check_save()) {
      return;
    }

    w->hide();
    textbuf->remove_modify_callback(changed_cb, w);
    delete w;
    num_windows--;
    if (!num_windows) exit(0);
}

void quit_cb(Fl_Widget*, void*) {
    if (changed && !check_save())
      return;

    exit(0);
}

void replace_cb(Fl_Widget*, void* v) {
    EditorWindow* e = (EditorWindow*)v;
    e->replace_dlg->show();
}

void replace2_cb(Fl_Widget*, void* v) {
    EditorWindow* e = (EditorWindow*)v;
    const char *find = e->replace_find->value();
    const char *replace = e->replace_with->value();

    if (find[0] == '\0') {
      // Search string is blank; get a new one...
      e->replace_dlg->show();
      return;
    }

    e->replace_dlg->hide();

    int pos = e->editor->insert_position();
    int found = textbuf->search_forward(pos, find, &pos);

    if (found) {
      // Found a match; update the position and replace text...
      textbuf->select(pos, pos+strlen(find));
      textbuf->remove_selection();
      textbuf->insert(pos, replace);
      textbuf->select(pos, pos+strlen(replace));
      e->editor->insert_position(pos+strlen(replace));
      e->editor->show_insert_position();
    }
    else fl_alert("No occurrences of \'%s\' found!", find);
}

void replall_cb(Fl_Widget*, void* v) {
    EditorWindow* e = (EditorWindow*)v;
    const char *find = e->replace_find->value();
    const char *replace = e->replace_with->value();

    find = e->replace_find->value();
    if (find[0] == '\0') {
      // Search string is blank; get a new one...
      e->replace_dlg->show();
      return;
    }

    e->replace_dlg->hide();

    e->editor->insert_position(0);
    int times = 0;

    // Loop through the whole string
    for (int found = 1; found;) {
      int pos = e->editor->insert_position();
      found = textbuf->search_forward(pos, find, &pos);

      if (found) {
        // Found a match; update the position and replace text...
        textbuf->select(pos, pos+strlen(find));
        textbuf->remove_selection();
        textbuf->insert(pos, replace);
        e->editor->insert_position(pos+strlen(replace));
        e->editor->show_insert_position();
        times++;
      }
    }

    if (times) fl_message("Replaced %d occurrences.", times);
    else fl_alert("No occurrences of \'%s\' found!", find);
}

void replcan_cb(Fl_Widget*, void* v) {
    EditorWindow* e = (EditorWindow*)v;
    e->replace_dlg->hide();
}

void save_cb() {
    if (filename[0] == '\0') {
      // No filename - get one!
      saveas_cb();
      return;
    }
    else save_file(filename);
}

void saveas_cb() {
    char *newfile;

    newfile = fl_file_chooser("Save File As?", "*", filename);
    if (newfile != NULL) save_file(newfile);
}

Fl_Window* new_view();

void view_cb(Fl_Widget*, void*) {
    Fl_Window* w = new_view();
    w->show();
}

Fl_Menu_Item menuitems[] = {
  { "&File",              0, 0, 0, FL_SUBMENU },
  { "&New File",        0, (Fl_Callback *)new_cb },
  { "&Open File...",    FL_CTRL + 'o', (Fl_Callback *)open_cb },
  { "&Insert File...",  FL_CTRL + 'i', (Fl_Callback *)insert_cb, 0,
    FL_MENU_DIVIDER },
  { "&Save File",       FL_CTRL + 's', (Fl_Callback *)save_cb },
  { "Save File &As...", FL_CTRL + FL_SHIFT + 's',
    (Fl_Callback *)saveas_cb, 0, FL_MENU_DIVIDER },
  { "New &View", FL_ALT + 'v', (Fl_Callback *)view_cb, 0 },
  { "&Close View", FL_CTRL + 'w', (Fl_Callback *)close_cb, 0,
    FL_MENU_DIVIDER },
  { "E&xit", FL_CTRL + 'q', (Fl_Callback *)quit_cb, 0 },
  { 0 },

  { "&Edit", 0, 0, 0, FL_SUBMENU },
  { "Cu&t",        FL_CTRL + 'x', (Fl_Callback *)cut_cb },
  { "&Copy",       FL_CTRL + 'c', (Fl_Callback *)copy_cb },
  { "&Paste",      FL_CTRL + 'v', (Fl_Callback *)paste_cb },
  { "&Delete",     0, (Fl_Callback *)delete_cb },
  { 0 },

  { "&Search", 0, 0, 0, FL_SUBMENU },
  { "&Find...",       FL_CTRL + 'f', (Fl_Callback *)find_cb },
  { "F&ind Again",    FL_CTRL + 'g', find2_cb },
  { "&Replace...",    FL_CTRL + 'r', replace_cb },
  { "Re&place Again", FL_CTRL + 't', replace2_cb },
  { 0 },

  { 0 }
};

Fl_Window* new_view() {
    EditorWindow* w = new EditorWindow(660, 400, title);
    w->begin();
    Fl_Menu_Bar* m = new Fl_Menu_Bar(0, 0, 660, 30);
    m->copy(menuitems, w);
    w->editor = new Fl_Text_Editor(0, 30, 660, 370);
    w->editor->buffer(textbuf);
    w->editor->highlight_data(stylebuf, styletable,
                              sizeof(styletable) / sizeof(styletable[0]),
                              'A', style_unfinished_cb, 0);
    w->editor->textfont(FL_COURIER);
    w->end();
    w->resizable(w->editor);
    w->callback((Fl_Callback *)close_cb, w);

    textbuf->add_modify_callback(style_update, w->editor);
    textbuf->add_modify_callback(changed_cb, w);
    textbuf->call_modify_callbacks();
    num_windows++;
    return w;
}

int main(int argc, char **argv) {
    textbuf = new Fl_Text_Buffer;
    style_init();

    Fl_Window* window = new_view();

    window->show(1, argv);

    if (argc > 1) load_file(argv[1], -1);

    return Fl::run();
}
