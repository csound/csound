/* Adds carriage returns into text files. */

#include <stdio.h>
#include <unistd.h>

static void
usage(const char *prog)
{
  fprintf(stderr,
	  "Usage: %s [options] [input]\n"
	  "Options:\n"
	  "  -o file -- output to file (default is standard output)\n"
	  "  -h      -- print this message\n",
	  prog);
}

int
main(int argc, char **argv)
{
  extern char *optarg;
  extern int optind;

  char *output = 0;

  for (;;) {
    int c = getopt(argc, argv, "o:h");
    if (c == -1)
      break;
    switch (c) {
    case 'o':
      output = optarg;
      break;
    case 'h':
      usage(argv[0]);
      return 0;
    default:
      usage(argv[0]);
      return 1;
    }
  }

  switch (argc - optind) {
  case 0:			/* Use stdin */
    break;
  case 1:
    if (!freopen(argv[optind], "r", stdin)) {
      perror(argv[optind]);
      return 1;
    }
    break;
  default:
    fprintf(stderr, "Bad arg count\n");
    usage(argv[0]);
    return 1;
  }

  if (output && !freopen(output, "w", stdout)) {
    perror(output);
    return 1;
  }

  for (;;) {
    int c = getc(stdin);
    switch (c) {
    case EOF:
      return 0;
    case '\r':			/* Ignore carriage returns */
      continue;			/* so u2d is idempotent. */
    case '\n':
      putc('\r', stdout);
      /* fall thru okay*/
    default:
      putc(c, stdout);
    }
  }
}
