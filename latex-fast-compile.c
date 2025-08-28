#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define EVENT_SIZE (sizeof(struct inotify_event))

#define cmd_make_preamble                                                      \
  "pdflatex -ini -jobname=preamble \"&pdflatex\" mylatexformat.ltx "
#define cmd_use_preamble "pdflatex -fmt preamble "

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: latex-fast-compile <filename>\n");
    exit(EXIT_FAILURE);
  }
  char *file_path = argv[1];

  // file exists
  FILE *file = fopen(file_path, "r");
  if (file == NULL) {
    perror("!! Couldn't find file");
    exit(EXIT_FAILURE);
  }
  fclose(file);

  int length, i = 0;
  int wd, fd;
  char buf[EVENT_SIZE + NAME_MAX];

  fd = inotify_init();
  if (fd < 0) {
    perror("!! Couldn't initialize inotify");
    exit(EXIT_FAILURE);
  }

  // Watch its dir. https://unix.stackexchange.com/a/312359
  wd = inotify_add_watch(fd, dirname(file_path), IN_MODIFY | IN_DELETE);

  char *cmd = malloc(64 + strlen(file_path));
  strcat(cmd, cmd_make_preamble);
  strcat(cmd, file_path);

  fprintf(stderr, ":: Compiling preamble with %s\n", cmd);
  if (system(cmd) != 0) {
    fprintf(stderr, "!! Failed to compile preamble.\n");
    exit(1);
  }
  printf(":: Compiled preamble.\n");

  memset(cmd, 0, (50 + strlen(file_path)) * sizeof(char));
  strcat(cmd, cmd_use_preamble);
  strcat(cmd, file_path);

  printf(":: Watching %s...\n", file_path);

  while (1) {
    i = 0;
    length = read(fd, buf, EVENT_SIZE + NAME_MAX);
    if (length < 0) {
      perror("!! Couldn't read file descriptor");
      exit(EXIT_FAILURE);
    }

    while (i < length) {
      struct inotify_event *event = (struct inotify_event *)&buf[i];
      if (!event->len)
        continue;
      i += EVENT_SIZE + event->len;

      int is_file_arg = strcmp(event->name, basename(file_path)) == 0 ? 1 : 0;
      if (event->mask & IN_MODIFY && !(event->mask & IN_ISDIR) && is_file_arg)
        system(cmd);
    }
  }

  inotify_rm_watch(fd, wd);
  close(fd);

  return EXIT_SUCCESS;
}
