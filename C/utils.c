#include <ncurses.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>

#include "dungeon.h"
#include "utils.h"

void clear_status()
{
  uint16_t i;

  for (i = 0; i < DUNGEON_X; i++) {
    mvaddch(0, i, ' ');
  }
  refresh();
}

void display_message(char *str)
{
  uint8_t null_byte = 0;
  uint16_t i;

  for (i = 0; i < DUNGEON_X; i++) {
    if (null_byte) {
      mvaddch(0, i, ' ');
    } else {
      if (*str == 0) {
	null_byte = 1;
	mvaddch(0, i, '.');
      } else {
	mvaddch(0, i, *str);
      }
    }
    refresh();
    usleep(1000);
    str++;
  }
}

int makedirectory(char *dir)
{
  char *slash;

  for (slash = dir + strlen(dir); slash > dir && *slash != '/'; slash--)
    ;

  if (slash == dir) {
    return 0;
  }

  if (mkdir(dir, 0700)) {
    if (errno != ENOENT && errno != EEXIST) {
      fprintf(stderr, "mkdir(%s): %s\n", dir, strerror(errno));
      return 1;
    }
    if (*slash != '/') {
      return 1;
    }
    *slash = '\0';
    if (makedirectory(dir)) {
      *slash = '/';
      return 1;
    }

    *slash = '/';
    if (mkdir(dir, 0700) && errno != EEXIST) {
      fprintf(stderr, "mkdir(%s): %s\n", dir, strerror(errno));
      return 1;
    }
  }

  return 0;
}
