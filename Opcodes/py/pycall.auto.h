
typedef struct {
    OPDS    h;
    MYFLT   *function;
    MYFLT   *args[VARGMAX-3];
} PYCALL0;

typedef struct {
    OPDS    h;
    MYFLT   *trigger;
    MYFLT   *function;
    MYFLT   *args[VARGMAX-3];
} PYCALL0T;

typedef struct {
    OPDS    h;
    MYFLT   *result;
    MYFLT   *function;
    MYFLT   *args[VARGMAX-3];
} PYCALL1;

typedef struct {
    OPDS    h;
    MYFLT   *result;
    MYFLT   *trigger;
    MYFLT   *function;
    MYFLT   *args[VARGMAX-3];
    MYFLT   oresult;
} PYCALL1T;

typedef struct {
    OPDS    h;
    MYFLT   *result1;
    MYFLT   *result2;
    MYFLT   *function;
    MYFLT   *args[VARGMAX-3];
} PYCALL2;

typedef struct {
    OPDS    h;
    MYFLT   *result1;
    MYFLT   *result2;
    MYFLT   *trigger;
    MYFLT   *function;
    MYFLT   *args[VARGMAX-3];
    MYFLT   oresult1;
    MYFLT   oresult2;
} PYCALL2T;

typedef struct {
    OPDS    h;
    MYFLT   *result1;
    MYFLT   *result2;
    MYFLT   *result3;
    MYFLT   *function;
    MYFLT   *args[VARGMAX-3];
} PYCALL3;

typedef struct {
    OPDS    h;
    MYFLT   *result1;
    MYFLT   *result2;
    MYFLT   *result3;
    MYFLT   *trigger;
    MYFLT   *function;
    MYFLT   *args[VARGMAX-3];
    MYFLT   oresult1;
    MYFLT   oresult2;
    MYFLT   oresult3;
} PYCALL3T;

typedef struct {
    OPDS    h;
    MYFLT   *result1;
    MYFLT   *result2;
    MYFLT   *result3;
    MYFLT   *result4;
    MYFLT   *function;
    MYFLT   *args[VARGMAX-3];
} PYCALL4;

typedef struct {
    OPDS    h;
    MYFLT   *result1;
    MYFLT   *result2;
    MYFLT   *result3;
    MYFLT   *result4;
    MYFLT   *trigger;
    MYFLT   *function;
    MYFLT   *args[VARGMAX-3];
    MYFLT   oresult1;
    MYFLT   oresult2;
    MYFLT   oresult3;
    MYFLT   oresult4;
} PYCALL4T;

typedef struct {
    OPDS    h;
    MYFLT   *result1;
    MYFLT   *result2;
    MYFLT   *result3;
    MYFLT   *result4;
    MYFLT   *result5;
    MYFLT   *function;
    MYFLT   *args[VARGMAX-3];
} PYCALL5;

typedef struct {
    OPDS    h;
    MYFLT   *result1;
    MYFLT   *result2;
    MYFLT   *result3;
    MYFLT   *result4;
    MYFLT   *result5;
    MYFLT   *trigger;
    MYFLT   *function;
    MYFLT   *args[VARGMAX-3];
    MYFLT   oresult1;
    MYFLT   oresult2;
    MYFLT   oresult3;
    MYFLT   oresult4;
    MYFLT   oresult5;
} PYCALL5T;

typedef struct {
    OPDS    h;
    MYFLT   *result1;
    MYFLT   *result2;
    MYFLT   *result3;
    MYFLT   *result4;
    MYFLT   *result5;
    MYFLT   *result6;
    MYFLT   *function;
    MYFLT   *args[VARGMAX-3];
} PYCALL6;

typedef struct {
    OPDS    h;
    MYFLT   *result1;
    MYFLT   *result2;
    MYFLT   *result3;
    MYFLT   *result4;
    MYFLT   *result5;
    MYFLT   *result6;
    MYFLT   *trigger;
    MYFLT   *function;
    MYFLT   *args[VARGMAX-3];
    MYFLT   oresult1;
    MYFLT   oresult2;
    MYFLT   oresult3;
    MYFLT   oresult4;
    MYFLT   oresult5;
    MYFLT   oresult6;
} PYCALL6T;

typedef struct {
    OPDS    h;
    MYFLT   *result1;
    MYFLT   *result2;
    MYFLT   *result3;
    MYFLT   *result4;
    MYFLT   *result5;
    MYFLT   *result6;
    MYFLT   *result7;
    MYFLT   *function;
    MYFLT   *args[VARGMAX-3];
} PYCALL7;

typedef struct {
    OPDS    h;
    MYFLT   *result1;
    MYFLT   *result2;
    MYFLT   *result3;
    MYFLT   *result4;
    MYFLT   *result5;
    MYFLT   *result6;
    MYFLT   *result7;
    MYFLT   *trigger;
    MYFLT   *function;
    MYFLT   *args[VARGMAX-3];
    MYFLT   oresult1;
    MYFLT   oresult2;
    MYFLT   oresult3;
    MYFLT   oresult4;
    MYFLT   oresult5;
    MYFLT   oresult6;
    MYFLT   oresult7;
} PYCALL7T;

typedef struct {
    OPDS    h;
    MYFLT   *result1;
    MYFLT   *result2;
    MYFLT   *result3;
    MYFLT   *result4;
    MYFLT   *result5;
    MYFLT   *result6;
    MYFLT   *result7;
    MYFLT   *result8;
    MYFLT   *function;
    MYFLT   *args[VARGMAX-3];
} PYCALL8;

typedef struct {
    OPDS    h;
    MYFLT   *result1;
    MYFLT   *result2;
    MYFLT   *result3;
    MYFLT   *result4;
    MYFLT   *result5;
    MYFLT   *result6;
    MYFLT   *result7;
    MYFLT   *result8;
    MYFLT   *trigger;
    MYFLT   *function;
    MYFLT   *args[VARGMAX-3];
    MYFLT   oresult1;
    MYFLT   oresult2;
    MYFLT   oresult3;
    MYFLT   oresult4;
    MYFLT   oresult5;
    MYFLT   oresult6;
    MYFLT   oresult7;
    MYFLT   oresult8;
} PYCALL8T;

