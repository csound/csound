
typedef struct	{
	OPDS	h;
	//MYFLT *ktrig, *kvalue, *ihandle;
	MYFLT *chan, *cc, *ihandle;
	int	ccVal, oldCCval;
	MYFLT log_base, min, max, range;
	void *WidgAddress, *opcode, *addrSetVal;
	int exp, widg_type;
} FL_MIDI_WIDGET_VALUE;


typedef struct	{
	OPDS	h;
	MYFLT   *kascii,*ifn;//, *ifnMap;
	MYFLT	*table;//, *tableMap;
	int		flag;
} FLKEYIN;

typedef struct	{
	OPDS	h;
	MYFLT  *group;
} FLSETSNAPGROUP;

typedef struct	{
	OPDS	h;
	MYFLT   *x,*y, *b1, *b2, *b3, *flagRaw;
	MYFLT height,width;
} FLMOUSE;


typedef struct	{
	OPDS	h;
	MYFLT *ihandle, *numlinesX, *numlinesY, *iwidth, *iheight, *ix, *iy,*image;
	int width, height;
} FL_HVSBOX;

typedef struct	{
	OPDS	h;
	MYFLT *kx, *ky, *ihandle;
	void *WidgAddress, *opcode;
	MYFLT old_x, old_y;
} FL_SET_HVS_VALUE;

typedef struct	{
	OPDS	h;
	MYFLT max[MAXCHNLS];
	unsigned long widg_address[MAXCHNLS];
	int dummycycles, dummycyc;
} VUMETER;
