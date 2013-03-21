#include <unistd.h>
#include "csdl.h"
#include "linux/joystick.h"

typedef struct
{
  OPDS h;
  MYFLT *kresult, *kdev, *ktable;
  int devFD;
  unsigned int numk, numb;
  int timeout, initme;
  MYFLT table;
  int dev;
  FUNC *ftp;
} LINUXJOYSTICK;
