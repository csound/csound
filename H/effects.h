
typedef struct {
        OPDS    h;
        FLOAT   *aoutL, *aoutR, *ain;
        FLOAT   *cur;           /* current value of delay address register */
        AUXCH   auxch;
} LRGHALL;

void lh_set(LRGHALL *p);
void lh_proc(LRGHALL *p);
