#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

static int EVENT_SIZE = sizeof(struct inotify_event);

// the actual commands (excluding filename, e.g. main.tex).
// if you want to integrate this into a different workflow, use these.
static char *cmd_make_preamble =
    "pdflatex -ini -jobname=preamble \"&pdflatex\" mylatexformat.ltx ";
static char *cmd_use_preamble = "pdflatex -synctex=1 -fmt preamble ";
static int cmd_len = 64;

// colored fprintf helper using ANSI escape codes. format_str must contain
// exactly one '%s'.
static inline int color_fprintf(FILE *stream, char *format_str, char *str) {
  if (isatty(fileno(stream)))
    fprintf(stream, stream == stderr ? "\033[1;31m!! \033[1;37m"
                                     : "\033[1;36m:: \033[1;37m");
  fprintf(stream, format_str, str);
  // FIXME: the two lines below should be replaceable with the more succinct
  //          if (isatty(fileno(stream)) fprintf(stream, "\033[0m");
  //        but in testing this doesn't seem to reset the color code to default
  //        :/
  if (isatty(fileno(stdout)))
    fprintf(stdout, "\033[0m");
  if (isatty(fileno(stderr)))
    fprintf(stderr, "\033[0m");
  return 0;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: latex-fast-compile <filename>\n"
                    "See 'man latex-fast-compile'.\n");
    exit(EXIT_FAILURE);
  }
  char *file_path = argv[1];

  // make sure file exists
  FILE *file = fopen(file_path, "r");
  if (file == NULL) {
    color_fprintf(stderr, "Couldn't find file: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  fclose(file);

  int length, i = 0;
  int wd, fd;
  char buf[EVENT_SIZE + NAME_MAX];

  fd = inotify_init();
  if (fd < 0) {
    color_fprintf(stderr, "Couldn't initialize inotify: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  // Watch its dir using inotify. https://unix.stackexchange.com/a/312359
  wd = inotify_add_watch(fd, dirname(file_path), IN_MODIFY | IN_DELETE);

  char *cmd = malloc(cmd_len + strlen(file_path));
  if (cmd == NULL) {
    color_fprintf(stderr, "Memory error on malloc: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  strcpy(cmd, cmd_make_preamble);
  strcat(cmd, file_path);

  color_fprintf(stdout, "Compiling preamble with %s...\n", cmd);
  if (system(cmd) != 0) {
    color_fprintf(stderr, "Failed to compile preamble.%s\n", "");
    exit(1);
  }
  color_fprintf(stdout, "Compiled preamble.\n%s", "");

  strcpy(cmd, cmd_use_preamble);
  strcat(cmd, file_path);
  color_fprintf(stdout, "Compiling %s for the first time...\n", file_path);
  system(cmd);
  color_fprintf(stdout, "Compiled %s.\n", file_path);

  color_fprintf(stdout, "Watching %s...\n", file_path);

  // start infinite watch, recompiling whenever file changes
  while (1) {
    i = 0;
    length = read(fd, buf, EVENT_SIZE + NAME_MAX);
    if (length < 0) {
      color_fprintf(stderr, "Couldn't read file descriptor: %s",
                    strerror(errno));
      exit(EXIT_FAILURE);
    }

    while (i < length) {
      struct inotify_event *event = (struct inotify_event *)&buf[i];
      if (!event->len)
        continue;
      i += EVENT_SIZE + event->len;

      int is_file_arg = strcmp(event->name, basename(file_path)) == 0 ? 1 : 0;
      if (event->mask & IN_MODIFY && !(event->mask & IN_ISDIR) && is_file_arg) {
        color_fprintf(stdout, "Detected change. Recompiling %s...\n",
                      file_path);
        system(cmd);
        color_fprintf(stdout, "Compiled %s.\n", file_path);
      }
    }
  }

  // TODO: this is unreachable, since i don't have logic to
  // break the loop cleanly on file deletion. however,
  // modern linux cleans this stuff up anyway.
  inotify_rm_watch(fd, wd);
  close(fd);

  return EXIT_SUCCESS;
}
