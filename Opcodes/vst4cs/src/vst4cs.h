#ifndef VST4CS_H
#define VST4CS_H
#include "cs.h"

#if defined(WIN32)
#define PUBLIC __declspec(dllexport)
#define DIR_SEP '\\'
#else
#define PUBLIC
#define DIR_SEP '/'
#endif

extern MYFLT SCALING_FACTOR;

typedef struct VSTINIT_ {
	OPDS h;
	MYFLT *iVSThandle;
	MYFLT *iplugin;
	MYFLT *iverbose;
} VSTINIT ;

typedef struct VSTINFO_ {
	OPDS h;
	MYFLT *iVSThandle;
} VSTINFO ;

typedef struct VST_PLUGIN_ {
	OPDS h;
	MYFLT *aout1;
	MYFLT *aout2;
	MYFLT *iVSThandle;
	MYFLT *ain1;
	MYFLT *ain2;
	long nsamps;
} VST_PLUGIN ;

typedef struct VSTNOTE_ {
	OPDS h;
	MYFLT *iVSThandle;
	MYFLT *kchan;
	MYFLT *knote;
	MYFLT *kveloc;
	MYFLT *kdur;
	int frames_left;
} VSTNOTE ;

typedef struct OUTVST_ {
	OPDS h;
	MYFLT *iVSThandle;
	MYFLT *kstatus;
	MYFLT *kchan;
	MYFLT *kdata1;
	MYFLT *kdata2;
	MYFLT oldKstatus;
	MYFLT oldKchan;
	MYFLT oldKdata1;
	MYFLT oldKdata2;
} OUTVST ;

typedef struct VSTPRET_ {
	OPDS h;
	MYFLT *kvalue;
	MYFLT *iVSThandle;
	MYFLT *kparam;
} VSTPRET ;

typedef struct VSTPSEND_ {
	OPDS h;
	MYFLT *iVSThandle;
	MYFLT *kparam;
	MYFLT *kvalue;
	MYFLT oldparam;
	MYFLT oldvalue;
} VSTPSEND ;

#endif

