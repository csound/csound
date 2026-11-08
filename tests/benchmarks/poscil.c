/* Measure poscil through the host API. See README.md for usage and limits. */
#include "csound.h"
#include "msg_attr.h"
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif

static double now(void)
{
#ifdef _WIN32
  LARGE_INTEGER ticks, frequency;
  if (!QueryPerformanceCounter(&ticks) || !QueryPerformanceFrequency(&frequency))
    return -1;
  return (double)ticks.QuadPart / (double)frequency.QuadPart;
#else
  struct timespec ticks;
  if (clock_gettime(CLOCK_MONOTONIC, &ticks))
    return -1;
  return ticks.tv_sec + ticks.tv_nsec * 1e-9;
#endif
}

static double number(const char *text, double minimum, double maximum)
{
  char *end;
  errno = 0;
  double value = strtod(text, &end);
  if (errno || end == text || *end || !isfinite(value) ||
      value < minimum || value > maximum) {
    fprintf(stderr, "Invalid benchmark argument: %s\n", text);
    exit(2);
  }
  return value;
}

static int count(const char *text, int minimum, int maximum)
{
  double value = number(text, minimum, maximum);
  if (value != floor(value)) {
    fprintf(stderr, "Expected a whole number: %s\n", text);
    exit(2);
  }
  return (int)value;
}

static void messages(CSOUND *csound, int32_t attr, const char *text)
{
  (void)csound;
  if ((attr & CSOUNDMSG_TYPE_MASK) == CSOUNDMSG_ERROR)
    fputs(text, stderr);
}

int main(int argc, char **argv)
{
  if (argc != 6) {
    fprintf(stderr, "Usage: %s ksmps voices table_size frequency seconds\n",
            argv[0]);
    return 2;
  }
  int block = count(argv[1], 1, 4096);
  int voices = count(argv[2], 1, 1024);
  int length = count(argv[3], 2, 1048576);
  double frequency = number(argv[4], -192000, 192000);
  double seconds = number(argv[5], 1, 3600);
  const int rate = 48000;
  int warmups = (rate + block - 1) / block;
  int blocks = (int)(seconds * rate / block);
  char source[512];
  int size = snprintf(source, sizeof(source),
      "sr=%d\nksmps=%d\nnchnls=1\n0dbfs=1\n"
      "giWave ftgen 1, 0, -%d, 10, 1\n"
      "instr 1\n"
      "kAmp init %.17g\nkFrequency init p4\n"
      "aSignal poscil kAmp, kFrequency, giWave, .1234567\n"
      "out aSignal\nendin\n", rate, block, length, .1 / voices);
  if (size < 0 || (size_t)size >= sizeof(source))
    return 2;

  CSOUND *csound = csoundCreate(NULL, "");
  if (csound == NULL)
    return 1;
  int result = 1;
  csoundSetMessageStringCallback(csound, messages);
  if (csoundSetOption(csound, "-n -d -m0") ||
      csoundCompileOrc(csound, source, 0) || csoundStart(csound))
    goto done;
  for (int voice = 0; voice < voices; voice++) {
    cs_float event[] = {1, 0, (cs_float)(seconds + 2),
                       (cs_float)(frequency + voice * .25)};
    csoundEvent(csound, CS_INSTR_EVENT, event, 4, 0);
  }
  for (int i = 0; i < warmups; i++)
    if (csoundPerformKsmps(csound))
      goto done;

  double start = now();
  if (start < 0)
    goto done;
  for (int i = 0; i < blocks; i++)
    if (csoundPerformKsmps(csound))
      goto done;
  double end = now();
  if (end <= start)
    goto done;

  /* Read the output outside the timed loop and reject a silent failed setup. */
  double energy = 0;
  const cs_float *output = csoundGetSpout(csound);
  for (int i = 0; i < block; i++) {
    if (!isfinite(output[i]))
      goto done;
    energy += output[i] * output[i];
  }
  if (!(energy > 0))
    goto done;
  printf("sample_bytes=%zu ksmps=%d voices=%d table_size=%d frequency=%.9g "
         "audio_seconds=%.9g elapsed_seconds=%.9g\n",
         sizeof(cs_float), block, voices, length, frequency,
         (double)blocks * block / rate, end - start);
  result = 0;
done:
  if (result)
    fputs("Benchmark failed or ran too briefly to time.\n", stderr);
  csoundDestroy(csound);
  return result;
}
