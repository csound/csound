#include <csound.h>

int main(void)
{
    CSOUND *csound = csoundCreate(NULL, NULL);
    if (csound == NULL) {
        return 1;
    }

    csoundDestroy(csound);
    return 0;
}
