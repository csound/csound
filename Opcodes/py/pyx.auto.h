
typedef struct {
    OPDS    h;
    STRINGDAT *string;
} PYEXEC;

typedef struct {
    OPDS    h;
    MYFLT   *trigger;
    STRINGDAT *string;
} PYEXECT;

typedef struct {
    OPDS    h;
    STRINGDAT *string;
} PYRUN;

typedef struct {
    OPDS    h;
    MYFLT   *trigger;
    STRINGDAT *string;
} PYRUNT;

typedef struct {
    OPDS    h;
    MYFLT   *result;
    STRINGDAT *string;
} PYEVAL;

typedef struct {
    OPDS    h;
    MYFLT   *result;
    MYFLT   *trigger;
    STRINGDAT *string;
    MYFLT   oresult;
} PYEVALT;

typedef struct {
    OPDS    h;
    STRINGDAT *string;
    MYFLT   *value;
} PYASSIGN;

typedef struct {
    OPDS    h;
    MYFLT   *trigger;
    STRINGDAT *string;
    MYFLT   *value;
} PYASSIGNT;

