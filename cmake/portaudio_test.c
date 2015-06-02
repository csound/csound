#include <portaudio.h>

int main(int argc, char** argv) {
  /* Test for 1899 or greater as 1899 is used for v19-devel */
  return Pa_GetVersion() >= 1899;
}
