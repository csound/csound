typedef struct _DelayLine { 
        float   *data;
        int     length; 
        float   *pointer;
        float   *end; 
} DelayLine; 

typedef struct  {
        OPDS    h;
        float   *ar, *plk, *xamp, *icps, *pickup, *reflect;
        AUXCH   upper;
        AUXCH   lower;
        AUXCH   up_data;
        AUXCH   down_data;
        float   state;
        int     scale;
        int     rail_len;
} WGPLUCK;

