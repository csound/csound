/* Native scalar CSE benchmark. See optimizer/README.md for its limits. */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "csound.h"
#include "csound_misc.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

static void report_library(void)
{
#ifdef _WIN32
  HMODULE module;
  char path[32768];
  HMODULE linked = GetModuleHandleA(CSOUND_CSE_BENCH_DLL);
  FARPROC create = linked ? GetProcAddress(linked, "csoundCreate") : NULL;
  if (!create) exit(1);
  if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                         GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                         (LPCSTR)create, &module)) exit(1);
  DWORD size = GetModuleFileNameA(module, path, sizeof(path));
  if (!size || size >= sizeof(path)) exit(1);
#else
  Dl_info info;
  void *create = dlsym(RTLD_DEFAULT, "csoundCreate");
  if (!create || !dladdr(create, &info) || !info.dli_fname) exit(1);
  const char *path = info.dli_fname;
#endif
  fprintf(stderr, "CSOUND_BENCH_LIBRARY %s\n", path);
}

static double wall_time(void)
{
#ifdef _WIN32
  LARGE_INTEGER ticks, frequency;
  if (!QueryPerformanceCounter(&ticks) || !QueryPerformanceFrequency(&frequency))
    exit(1);
  return (double)ticks.QuadPart / frequency.QuadPart;
#else
  struct timespec ticks;
  if (clock_gettime(CLOCK_MONOTONIC, &ticks)) exit(1);
  return ticks.tv_sec + ticks.tv_nsec * 1e-9;
#endif
}

static double cpu_time(void)
{
#ifdef _WIN32
  FILETIME created, exited, kernel, user;
  ULARGE_INTEGER k, u;
  if (!GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user))
    exit(1);
  k.LowPart = kernel.dwLowDateTime;
  k.HighPart = kernel.dwHighDateTime;
  u.LowPart = user.dwLowDateTime;
  u.HighPart = user.dwHighDateTime;
  return (double)(k.QuadPart + u.QuadPart) * 1e-7;
#else
  clock_t ticks = clock();
  if (ticks == (clock_t)-1) exit(1);
  return (double)ticks / CLOCKS_PER_SEC;
#endif
}

static int count(const char *text, int minimum, int maximum)
{
  char *end;
  errno = 0;
  long value = strtol(text, &end, 10);
  if (errno || end == text || *end || value < minimum || value > maximum) {
    fprintf(stderr, "Invalid benchmark argument: %s\n", text);
    exit(2);
  }
  return (int)value;
}

static void check(CSOUND *csound, int status)
{
  if (!status) return;
  while (csoundGetMessageCnt(csound)) {
    fputs(csoundGetFirstMessage(csound), stderr);
    csoundPopFirstMessage(csound);
  }
  fprintf(stderr, "Csound returned %d\n", status);
  exit(1);
}

int main(int argc, char **argv)
{
  if (argc != 7 && argc != 8) {
    fprintf(stderr, "Usage: %s mask expressions voices windows unique rate [audio_capture]\n", argv[0]);
    return 2;
  }
  int mask = count(argv[1], 0, 9), expressions = count(argv[2], 0, 4096);
  int voices = count(argv[3], 1, 512), windows = count(argv[4], 1, 60);
  report_library();
  if (csoundGetSizeOfMYFLT() != sizeof(cs_float)) return 1;
  FILE *capture = argc == 8 ? fopen(argv[7], "wb") : NULL;
  if (argc == 8 && !capture) return 1;
  int unique = count(argv[5], 0, 1);
  char rate = argv[6][0];
  if ((rate != 'i' && rate != 'k') || argv[6][1]) return 2;
  /* Each generated pair of lines needs fewer than 128 bytes at these limits. */
  char *source = calloc((size_t)expressions * 128 + 512, 1);
  if (!source) return 1;
  int used = sprintf(source, "sr=48000\nksmps=64\nnchnls=1\n0dbfs=1\n"
                     "instr 1\n%cInput init p4\n%cSum=0\n", rate, rate);
  for (int i = 0; i < expressions; i++)
    used += sprintf(source + used,
        "%cValue%d=(%cInput+%.12f)*0.75-0.25\n%cSum=%cSum+%cValue%d\n",
        rate, i, rate, .125 + (unique ? i / 4096.0 : 0), rate, rate, rate, i);
  sprintf(source + used, "aTone oscili .0001, 220+%cSum*.001\nout aTone\nendin\n", rate);
  CSOUND *csound = csoundCreate(NULL, "");
  if (!csound) return 1;
  csoundCreateMessageBuffer(csound, 0);
  check(csound, csoundSetOption(csound, "-n -d -m0 --optimize=none"));
  if (mask & ~9) return 2;
  if (mask & 1) check(csound, csoundSetOption(csound, "--optimize-legacy"));
  if (mask & 8)
    check(csound, csoundSetOption(csound, "--optimize-common-subexpression-elimination"));
  int32_t *selection = csoundQueryGlobalVariable(csound, "__orc_optimizer");
  if (!selection || *selection != mask) return 1;
  double start = wall_time();
  check(csound, csoundCompileOrc(csound, source, 0));
  double compile_ms = (wall_time() - start) * 1000;
  free(source);
  check(csound, csoundStart(csound));
  for (int voice = 0; voice < voices; voice++) {
    cs_float event[] = {1, 0, 60, (cs_float)(2 + voice)};
    csoundEvent(csound, CS_INSTR_EVENT, event, 4, 0);
  }
  start = wall_time();
  check(csound, csoundPerformKsmps(csound));
  double activation_ms = (wall_time() - start) * 1000;
  if (capture && fwrite(csoundGetSpout(csound), sizeof(cs_float), 64, capture) != 64)
    return 1;
  printf("{\"sample_bytes\":%zu,\"compile_ms\":%.9g,\"activation_ms\":%.9g,\"windows\":[",
         sizeof(cs_float), compile_ms, activation_ms);
  for (int window = -3; window < windows; window++) {
    double cpu_start = cpu_time();
    start = wall_time();
    /* 128 blocks at ksmps=64 and 48 kHz per window. */
    for (int block = 0; block < 128; block++) {
      check(csound, csoundPerformKsmps(csound));
      /* Capturing uses separate, untimed comparison runs in the runner. */
      if (capture && fwrite(csoundGetSpout(csound), sizeof(cs_float), 64, capture) != 64)
        return 1;
    }
    double wall_ms = (wall_time() - start) * 1000;
    double cpu_ms = (cpu_time() - cpu_start) * 1000;
    if (window >= 0)
      printf("%s{\"score_seconds\":%.6f,\"cpu_ms\":%.9g,\"wall_ms\":%.9g}",
             window ? "," : "", (window + 1) * (128.0 * 64 / 48000),
             cpu_ms, wall_ms);
  }
  puts("]}");
  /* Keep compiler/startup/performance messages, excluding destruction timings. */
  while (csoundGetMessageCnt(csound)) {
    fputs(csoundGetFirstMessage(csound), stderr);
    csoundPopFirstMessage(csound);
  }
  csoundDestroy(csound);
  if (capture && fclose(capture)) return 1;
  return 0;
}
