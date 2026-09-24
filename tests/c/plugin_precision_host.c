/* Run each plugin check in a host linked with the build's sanitizer flags. */
#include "csound.h"
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition)                                                       \
  do {                                                                         \
    if (!(condition)) {                                                        \
      fprintf(stderr, "%s: failed: %s\n", argv[1], #condition);                 \
      failed = 1;                                                              \
      goto cleanup;                                                            \
    }                                                                          \
  } while (0)

int main(int argc, char **argv)
{
  if (argc != 5) {
    fprintf(stderr, "usage: %s plugin kind sample-size double-size\n", argv[0]);
    return 1;
  }
  const char *kind = argv[2];
  int sample_size = atoi(argv[3]), double_size = atoi(argv[4]);
  int cpp = strcmp(kind, "cpp") == 0;
  int linkage = strcmp(kind, "linkage") == 0;
  int modern = cpp || linkage || strcmp(kind, "current") == 0;
  int declares_sample = modern || strcmp(kind, "legacy") == 0;
  int accepted = csoundGetSizeOfCsDouble() == (modern ? double_size : 8) &&
                 (!declares_sample || csoundGetSizeOfCsFloat() == sample_size);
  int failed = 0, incompatible = 0;
  CSOUND *csound = NULL;
  char *option = NULL;
  void *plugin = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
  if (plugin == NULL) {
    fprintf(stderr, "%s\n", dlerror());
    return 1;
  }
  int (*entry_counts)(void) = (int (*)(void))dlsym(plugin, "entryCounts");
  if (modern) {
    int32_t (*info)(void) = (int32_t (*)(void))dlsym(plugin, "csoundModuleInfo");
    CHECK(info != NULL);
    /* Pin the legacy byte values for older loaders too. */
    CHECK((info() & 0xff) == (sample_size | (double_size == 4 ? 0x80 : 0)));
  }
  CHECK(linkage || entry_counts != NULL);
  csound = csoundCreate(NULL, NULL);
  CHECK(csound != NULL);
  csoundCreateMessageBuffer(csound, 0);
  option = malloc(strlen(argv[1]) + sizeof("--opcode-lib="));
  CHECK(option != NULL);
  sprintf(option, "--opcode-lib=%s", argv[1]);
  CHECK(csoundSetOption(csound, option) == 0);
  int result = csoundCompileOrc(csound, linkage
      ? "instr 1\nprecision_test_opcode\nendin\n"
      : "instr 1\nendin\n", 0);
  while (csoundGetMessageCnt(csound)) {
    const char *message = csoundGetFirstMessage(csound);
    fputs(message, stderr);
    incompatible |= strstr(message, "incompatible") != NULL;
    csoundPopFirstMessage(csound);
  }
  /* Rejection warns; only the missing opcode prevents compilation. */
  CHECK((result == 0) == (accepted || !linkage));
  if (!linkage)
    CHECK(entry_counts() == (accepted ? (cpp ? 10 : 11) : 0));
  CHECK(accepted || incompatible);
  csoundDestroy(csound);
  csound = NULL;
  if (!cpp && !linkage)
    CHECK(entry_counts() == (accepted ? 111 : 0));

cleanup:
  if (csound != NULL)
    csoundDestroy(csound);
  free(option);
  dlclose(plugin);
  return failed;
}
