#ifndef VST4CS_H
#define VST4CS_H
#include "cs.h"

typedef struct VSTINIT {
	OPDS h;
	// Inputs.
	MYFLT *iVSThandle;
	MYFLT *iplugin;
	MYFLT *iverbose;
} VSTINIT ;

typedef struct VSTINFO {
	OPDS h;
	// Inputs.
	MYFLT *iVSThandle;
} VSTINFO ;

typedef struct VSTPLUG_ {
	OPDS h;
	// Outputs.
	MYFLT *aout1;
	MYFLT *aout2;
	// Inputs.
	MYFLT *iVSThandle;
	MYFLT *ain1;
	MYFLT *ain2;
	// State.
	size_t framesPerBlock;
	size_t channels;
} VSTPLUG_ ;

typedef struct VSTNOTE {
	OPDS h;	
	// Inputs.
	MYFLT *iVSThandle;
	MYFLT *kchan;
	MYFLT *knote;
	MYFLT *kveloc;
	MYFLT *kdur;
	// State.
	MYFLT framesRemaining;
} VSTNOTE ;

typedef struct OUTVST_ {
	OPDS h;
	// Inputs.
	MYFLT *iVSThandle;
	MYFLT *kstatus;
	MYFLT *kchan;
	MYFLT *kdata1;
	MYFLT *kdata2;
	// State.
	MYFLT oldkstatus;
	MYFLT oldkchan;
	MYFLT oldkvalue;
} OUTVST ;

typedef struct VSTPRET {
	OPDS h;
	// Outputs.
	MYFLT *kvalue;
	// Intputs.
	MYFLT *iVSThandle;
	MYFLT *kparam;
} VSTPRET ;

typedef struct VSTPSEND {
	OPDS h;
	// Inputs.
	MYFLT *iVSThandle;
	MYFLT *kparam;
	MYFLT *kvalue;
	// State.
	MYFLT oldkparam;
	MYFLT oldkvalue;
} VSTPSEND ;

#endif

