/* Repeated note-start benchmark. See optimizer/README.md for its limits. */
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
  HMODULE linked = GetModuleHandleA(CSOUND_BENCH_DLL);
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
  if (argc != 5 && argc != 6) {
    fprintf(stderr, "Usage: %s mask copies voices windows [audio_capture]\n", argv[0]);
    return 2;
  }
  int mask = count(argv[1], 0, 7), copies = count(argv[2], 0, 4096);
  int voices = count(argv[3], 1, 512), windows = count(argv[4], 1, 60);
  if (mask & ~5) {
    fputs("Unsupported optimizer mask in this build\n", stderr);
    return 2;
  }
  report_library();
  if (csoundGetSizeOfMYFLT() != sizeof(cs_float)) return 1;
  FILE *capture = argc == 6 ? fopen(argv[5], "wb") : NULL;
  if (argc == 6 && !capture) return 1;
  /* Each generated line needs fewer than 64 bytes at the allowed limits. */
  char *source = calloc((size_t)copies * 64 + 512, 1);
  if (!source) return 1;
  int used = sprintf(source, "sr=48000\nksmps=64\nnchnls=1\n0dbfs=1\n"
                     "instr 1\niValue=p4\n");
  for (int i = 0; i < copies; i++) {
    char input[32];
    if (i) snprintf(input, sizeof(input), "iValue%d", i - 1);
    else snprintf(input, sizeof(input), "p4");
    used += sprintf(source + used, "iValue%d=%s+%.6f\n", i, input, i * .001);
  }
  if (copies) used += sprintf(source + used, "iValue=iValue%d\n", copies - 1);
  sprintf(source + used, "aTone oscili .0001, iValue\nout aTone\nendin\n");
  CSOUND *csound = csoundCreate(NULL, "");
  if (!csound) return 1;
  csoundCreateMessageBuffer(csound, 0);
  check(csound, csoundSetOption(csound, "-n -d -m0 --optimize=none"));
  if (mask & 1) check(csound, csoundSetOption(csound, "--optimize-legacy"));
  if (mask & 4) check(csound, csoundSetOption(csound, "--optimize-copy-removal"));
  int32_t *selection = csoundQueryGlobalVariable(csound, "__orc_optimizer");
  if (!selection || *selection != mask) return 1;
  double start = wall_time();
  check(csound, csoundCompileOrc(csound, source, 0));
  double compile_ms = (wall_time() - start) * 1000;
  free(source);
  check(csound, csoundStart(csound));
  printf("{\"sample_bytes\":%zu,\"compile_ms\":%.9g,\"windows\":[",
         sizeof(cs_float), compile_ms);
  for (int window = -3; window < windows; window++) {
    double cpu_start = cpu_time();
    start = wall_time();
    /* 48 bursts of 16 blocks give 1.024 seconds of score time per window. */
    for (int block = 0; block < 768; block++) {
      if (block % 16 == 0) {
        for (int voice = 0; voice < voices; voice++) {
          cs_float event[] = {1, 0, (cs_float)(1024.0 / 48000),
                             (cs_float)(220 + voice)};
          csoundEvent(csound, CS_INSTR_EVENT, event, 4, 0);
        }
      }
      check(csound, csoundPerformKsmps(csound));
      /* Capturing uses separate, untimed comparison runs in the runner. */
      if (capture && fwrite(csoundGetSpout(csound), sizeof(cs_float), 64, capture) != 64)
        return 1;
    }
    double wall_ms = (wall_time() - start) * 1000;
    double cpu_ms = (cpu_time() - cpu_start) * 1000;
    if (window >= 0)
      printf("%s{\"score_seconds\":%.6f,\"cpu_ms\":%.9g,\"wall_ms\":%.9g}",
             window ? "," : "", (window + 1) * 1.024,
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
