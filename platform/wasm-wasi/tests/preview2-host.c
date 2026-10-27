#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char **argv) {
  assert(sizeof(void *) == 4);
  assert(argc == 2 && strcmp(argv[1], "preview2") == 0);
  assert(mkdir("files", 0700) == 0);
  FILE *file = fopen("files/audio", "w+");
  assert(file);
  const float samples[] = {0.125f, 0.25f, 0.5f, 1.0f};
  assert(fwrite(samples, sizeof(float), 4, file) == 4);
  assert(fseek(file, 4, SEEK_SET) == 0);
  float value = 0;
  assert(fread(&value, sizeof(float), 1, file) == 1 && value == 0.25f);
  assert(fclose(file) == 0);
  struct stat info;
  assert(stat("files/audio", &info) == 0 && info.st_size == 16);
  assert(rename("files/audio", "files/renamed") == 0);
  DIR *dir = opendir("files");
  assert(dir);
  int found = 0;
  for (struct dirent *entry; (entry = readdir(dir)); )
    found |= strcmp(entry->d_name, "renamed") == 0;
  assert(found && closedir(dir) == 0);
  assert(truncate("files/renamed", 4) == 0);
  file = fopen("files/renamed", "r");
  assert(file && fread(&value, sizeof(float), 1, file) == 1 && value == 0.125f);
  assert(fread(&value, sizeof(float), 1, file) == 0 && feof(file));
  assert(fclose(file) == 0);
  assert(unlink("files/renamed") == 0 && rmdir("files") == 0);
  errno = 0;
  assert(fopen("missing", "r") == NULL && errno == ENOENT);
  struct timespec now;
  assert(clock_gettime(CLOCK_REALTIME, &now) == 0 && now.tv_sec > 0);
  assert(clock_gettime(CLOCK_MONOTONIC, &now) == 0);
  unsigned char random[32];
  assert(getentropy(random, sizeof(random)) == 0);
  puts("Preview 2 arguments, files, streams, clocks and random passed");
  return 0;
}
