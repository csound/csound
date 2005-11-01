
typedef struct {
    OPDS    h;
    MYFLT   *string;
} PYEXEC;

typedef struct {
    OPDS    h;
    MYFLT   *trigger;
    MYFLT   *string;
} PYEXECT;

typedef struct {
    OPDS    h;
    MYFLT   *string;
} PYRUN;

typedef struct {
    OPDS    h;
    MYFLT   *trigger;
    MYFLT   *string;
} PYRUNT;

typedef struct {
    OPDS    h;
    MYFLT   *result;
    MYFLT   *string;
} PYEVAL;

typedef struct {
    OPDS    h;
    MYFLT   *result;
    MYFLT   *trigger;
    MYFLT   *string;
    MYFLT   oresult;
} PYEVALT;

typedef struct {
    OPDS    h;
    MYFLT   *string;
    MYFLT   *value;
} PYASSIGN;

typedef struct {
    OPDS    h;
    MYFLT   *trigger;
    MYFLT   *string;
    MYFLT   *value;
} PYASSIGNT;

