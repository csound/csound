typedef struct _DelayLine {
        FLOAT   *data;
        int     length;
        FLOAT   *pointer;
        FLOAT   *end;
} DelayLine;

typedef struct  {
        OPDS    h;
        FLOAT   *ar, *plk, *xamp, *icps, *pickup, *reflect;
        FLOAT   *ain;
        AUXCH   upper;
        AUXCH   lower;
        AUXCH   up_data;
        AUXCH   down_data;
        FLOAT   state;
        int     scale;
        int     rail_len;
} WGPLUCK;

