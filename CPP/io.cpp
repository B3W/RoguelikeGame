#include <unistd.h>
#include <ncurses.h>
#include <cmath>
#include <ctype.h>
#include <stdlib.h>

#include "io.h"
#include "move.h"
#include "path.h"
#include "pc.h"
#include "utils.h"
#include "dungeon.h"

#define LIGHT_RADIUS 3

/* Same ugly hack we did in path.c */
static dungeon *dungeon_ptr;
static uint8_t fog_flag = 1;

class io_message {
public:
  /* Will print " --more-- " at end of line when another message follows. *
   * Leave 10 extra spaces for that.                                      */
  char msg[71];
  io_message *next;
};

static io_message *io_head, *io_tail;

void io_init_terminal(void)
{
  initscr();
  raw();
  noecho();
  curs_set(0);
  keypad(stdscr, TRUE);
  start_color();
  init_pair(COLOR_RED, COLOR_RED, COLOR_BLACK);
  init_pair(COLOR_GREEN, COLOR_GREEN, COLOR_BLACK);
  init_pair(COLOR_YELLOW, COLOR_YELLOW, COLOR_BLACK);
  init_pair(COLOR_BLUE, COLOR_BLUE, COLOR_BLACK);
  init_pair(COLOR_MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
  init_pair(COLOR_CYAN, COLOR_CYAN, COLOR_BLACK);
  init_pair(COLOR_WHITE, COLOR_WHITE, COLOR_BLACK);
}

void io_reset_terminal(void)
{
  endwin();

  while (io_head) {
    io_tail = io_head;
    io_head = io_head->next;
    free(io_tail);
  }
  io_tail = NULL;
}

void io_queue_message(const char *format, ...)
{
  io_message *tmp;
  va_list ap;

  if (!(tmp = (io_message *) malloc(sizeof (*tmp)))) {
    perror("malloc");
    exit(1);
  }

  tmp->next = NULL;

  va_start(ap, format);

  vsnprintf(tmp->msg, sizeof (tmp->msg), format, ap);

  va_end(ap);

  if (!io_head) {
    io_head = io_tail = tmp;
  } else {
    io_tail->next = tmp;
    io_tail = tmp;
  }
}

static void io_print_message_queue(uint32_t y, uint32_t x)
{
  while (io_head) {
    io_tail = io_head;
    attron(COLOR_PAIR(COLOR_CYAN));
    mvprintw(y, x, "%-80s", io_head->msg);
    attroff(COLOR_PAIR(COLOR_CYAN));
    io_head = io_head->next;
    if (io_head) {
      attron(COLOR_PAIR(COLOR_CYAN));
      mvprintw(y, x + 70, "%10s", " --more-- ");
      attroff(COLOR_PAIR(COLOR_CYAN));
      refresh();
      getch();
    }
    free(io_tail);
  }
  io_tail = NULL;
}

void io_display_tunnel(dungeon *d)
{
  uint32_t y, x;
  clear();
  for (y = 0; y < DUNGEON_Y; y++) {
    for (x = 0; x < DUNGEON_X; x++) {
      if (charxy(x, y) == &d->pc) {
        mvaddch(y + 1, x, charxy(x, y)->symbol);
      } else if (hardnessxy(x, y) == 255) {
        mvaddch(y + 1, x, '*');
      } else {
        mvaddch(y + 1, x, '0' + (d->pc_tunnel[y][x] % 10));
      }
    }
  }
  refresh();
}

void io_display_distance(dungeon *d)
{
  uint32_t y, x;
  clear();
  for (y = 0; y < DUNGEON_Y; y++) {
    for (x = 0; x < DUNGEON_X; x++) {
      if (charxy(x, y)) {
        mvaddch(y + 1, x, charxy(x, y)->symbol);
      } else if (hardnessxy(x, y) != 0) {
        mvaddch(y + 1, x, ' ');
      } else {
        mvaddch(y + 1, x, '0' + (d->pc_distance[y][x] % 10));
      }
    }
  }
  refresh();
}

static char hardness_to_char[] =
  "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

void io_display_hardness(dungeon *d)
{
  uint32_t y, x;
  clear();
  for (y = 0; y < DUNGEON_Y; y++) {
    for (x = 0; x < DUNGEON_X; x++) {
      /* Maximum hardness is 255.  We have 62 values to display it, but *
       * we only want one zero value, so we need to cover [1,255] with  *
       * 61 values, which gives us a divisor of 254 / 61 = 4.164.       *
       * Generally, we want to avoid floating point math, but this is   *
       * not gameplay, so we'll make an exception here to get maximal   *
       * hardness display resolution.                                   */
      mvaddch(y + 1, x, (d->hardness[y][x]                             ?
                         hardness_to_char[1 + (int) ((d->hardness[y][x] /
                                                      4.2))] : ' '));
    }
  }
  refresh();
}

static int compare_monster_distance(const void *v1, const void *v2)
{
  const character *const *c1 = (const character *const *) v1;
  const character *const *c2 = (const character *const *) v2;

  return (dungeon_ptr->pc_distance[(*c1)->position[dim_y]][(*c1)->position[dim_x]] -
          dungeon_ptr->pc_distance[(*c2)->position[dim_y]][(*c2)->position[dim_x]]);
}

uint8_t outside_light(dungeon *d, character *c)
{
  int32_t x_dist, y_dist;

  x_dist = abs(d->pc.position[dim_x] - c->position[dim_x]);
  y_dist = abs(d->pc.position[dim_y] - c->position[dim_y]);

  if ((0 <= x_dist && x_dist <= LIGHT_RADIUS) && (0 <= y_dist && y_dist <= LIGHT_RADIUS)) {
    return 0;
  }
  return 1;      
}

static character *io_nearest_visible_monster(dungeon *d)
{
  character **c, *n;
  uint32_t x, y, count, i;

  c = (character **) malloc(d->num_monsters * sizeof (*c));

  /* Get a linear list of monsters */
  for (count = 0, y = 1; y < DUNGEON_Y - 1; y++) {
    for (x = 1; x < DUNGEON_X - 1; x++) {
      if (d->character_arr[y][x] && d->character_arr[y][x] != &d->pc) {
        c[count++] = d->character_arr[y][x];
      }
    }
  }

  /* Sort it by distance from PC */
  dungeon_ptr = d;
  qsort(c, count, sizeof (*c), compare_monster_distance);

  for (n = NULL, i = 0; i < count; i++) {
    //if (can_see(d, &d->pc, c[i])) {
    if (!outside_light(d, c[i])) {
      n = c[i];
      break;
    }
  }

  free(c);

  return n;
}

void io_display(dungeon *d)
{
  uint32_t y, x;
  character *c;

  clear();
  
  if (fog_flag) {
    for (y = 0; y < 21; y++) {
      for (x = 0; x < 80; x++) {
	if (d->character_arr[y][x] && !outside_light(d, d->character_arr[y][x])) {
	  mvaddch(y + 1, x, d->character_arr[y][x]->symbol);
	} else {
	  switch (d->pc.pc->player_map[y][x]) {
	  case ter_wall:
	  case ter_wall_immutable:
	    mvaddch(y + 1, x, ' ');
	    break;
	  case ter_floor:
	  case ter_floor_room:
	    mvaddch(y + 1, x, '.');
	    break;
	  case ter_floor_hall:
	    mvaddch(y + 1, x, '#');
	    break;
	  case ter_debug:
	    mvaddch(y + 1, x, '*');
	    break;
	  case ter_stairs_up:
	    mvaddch(y + 1, x, '<');
	    break;
	  case ter_stairs_down:
	    mvaddch(y + 1, x, '>');
	    break;
	  default:
	    /* Use zero as an error symbol, since it stands out somewhat, and it's *
	     * not otherwise used.                                                 */
	    mvaddch(y + 1, x, '0');
	  }
	}
      }
    }
  } else {
    for (y = 0; y < 21; y++) {
      for (x = 0; x < 80; x++) {
	if (d->character_arr[y][x]) {
	  mvaddch(y + 1, x, d->character_arr[y][x]->symbol);
	} else {
	  switch (mapxy(x, y)) {
	  case ter_wall:
	  case ter_wall_immutable:
	    mvaddch(y + 1, x, ' ');
	    break;
	  case ter_floor:
	  case ter_floor_room:
	    mvaddch(y + 1, x, '.');
	    break;
	  case ter_floor_hall:
	    mvaddch(y + 1, x, '#');
	    break;
	  case ter_debug:
	    mvaddch(y + 1, x, '*');
	    break;
	  case ter_stairs_up:
	    mvaddch(y + 1, x, '<');
	    break;
	  case ter_stairs_down:
	    mvaddch(y + 1, x, '>');
	    break;
	  default:
	    /* Use zero as an error symbol, since it stands out somewhat, and it's *
	     * not otherwise used.                                                 */
	    mvaddch(y + 1, x, '0');
	  }
	}
      }
    }
  }
  mvprintw(23, 1, "PC position is (%2d,%2d).",
           d->pc.position[dim_x], d->pc.position[dim_y]);
  mvprintw(22, 1, "%d %s detected.", d->num_monsters,
           d->num_monsters > 1 ? "monsters" : "monster");
  mvprintw(22, 30, "Nearest visible monster: ");
  if ((c = io_nearest_visible_monster(d))) {
    attron(COLOR_PAIR(COLOR_RED));
    mvprintw(22, 55, "%c at %d %c by %d %c.",
             c->symbol,
             abs(c->position[dim_y] - d->pc.position[dim_y]),
             ((c->position[dim_y] - d->pc.position[dim_y]) <= 0 ?
              'N' : 'S'),
             abs(c->position[dim_x] - d->pc.position[dim_x]),
             ((c->position[dim_x] - d->pc.position[dim_x]) <= 0 ?
              'W' : 'E'));
    attroff(COLOR_PAIR(COLOR_RED));
  } else {
    attron(COLOR_PAIR(COLOR_BLUE));
    mvprintw(22, 55, "NONE.");
    attroff(COLOR_PAIR(COLOR_BLUE));
  }
           

  io_print_message_queue(0, 0);

  refresh();
}

void io_display_monster_list(dungeon *d)
{
  mvprintw(11, 33, " HP:    XXXXX ");
  mvprintw(12, 33, " Speed: XXXXX ");
  mvprintw(14, 27, " Hit any key to continue. ");
  refresh();
  getch();
}

uint32_t io_teleport_pc(dungeon *d)
{
  /* Just for fun. */
  uint8_t prev_fog_flag = fog_flag;
  fog_flag = 0;
  io_display(d);
  
  pair_t dest, prev;
  int key;
  uint8_t teleporting = 1;

  dest[dim_x] = prev[dim_x] = d->pc.position[dim_x];
  dest[dim_y] = prev[dim_y] = d->pc.position[dim_y];
  
  do {
    switch (key = getch())
      {
      case '7':
      case 'y':
      case KEY_HOME: 
	dest[dim_x]--;
	dest[dim_y]--;
	break;
      case '8':
      case 'k':
      case KEY_UP:
	dest[dim_y]--;
	break;
      case '9':
      case 'u':
      case KEY_PPAGE:
	dest[dim_x]++;
	dest[dim_y]--;
	break;
      case '6':
      case 'l':
      case KEY_RIGHT:
	dest[dim_x]++;
	break;
      case '3':
      case 'n':
      case KEY_NPAGE:
	dest[dim_x]++;
	dest[dim_y]++;
	break;
      case '2':
      case 'j':
      case KEY_DOWN:
	dest[dim_y]++;
      break;
      case '1':
      case 'b':
      case KEY_END:
	dest[dim_x]--;
	dest[dim_y]++;
	break;
      case '4':
      case 'h':
      case KEY_LEFT:
	dest[dim_x]--;
	break;
      case 'r':
	do {
	  dest[dim_x] = rand_range(1, DUNGEON_X - 2);
	  dest[dim_y] = rand_range(1, DUNGEON_Y - 2);
	} while (charpair(dest));
	teleporting = 0;
	break;
      case 'g':
	if (charpair(dest)) {
	  /* Another character is in the spot */
	  dest[dim_x] = prev[dim_x];
	  dest[dim_y] = prev[dim_y];
	  io_queue_message("Trying to teleport inside another monster?");
	  io_queue_message("Now that's cruel.");
	  io_print_message_queue(0, 0);
	  continue;
	}
	teleporting = 0;
	break;
      default:
	/* Don't need to redraw dungeon when unbound key hit */
	continue;
      }
    if (d->map[dest[dim_y]][dest[dim_x]] == ter_wall_immutable) {
      /* Out of bounds move */
      dest[dim_x] = prev[dim_x];
      dest[dim_y] = prev[dim_y];
      io_queue_message("Unknown dark magic prevents you from exiting dungeon.");
      io_print_message_queue(0, 0);
      continue;
    }    
    prev[dim_x] = dest[dim_x];
    prev[dim_y] = dest[dim_y];
    
    io_display(d);
    mvaddch(dest[dim_y] + 1, dest[dim_x], '*');
  } while (teleporting);
  
  d->character_arr[d->pc.position[dim_y]][d->pc.position[dim_x]] = NULL;
  d->character_arr[dest[dim_y]][dest[dim_x]] = &d->pc;

  d->pc.position[dim_y] = dest[dim_y];
  d->pc.position[dim_x] = dest[dim_x];

  if (mappair(dest) < ter_floor) {
    mappair(dest) = ter_floor_hall;
  }

  dijkstra(d);
  dijkstra_tunnel(d);

  update_player_map(d);
  
  fog_flag = prev_fog_flag;
  io_display(d);
  
  return 0;
}
/* Adjectives to describe our monsters */
static const char *adjectives[] = {
  "A menacing ",
  "A threatening ",
  "A horrifying ",
  "An intimidating ",
  "An aggressive ",
  "A frightening ",
  "A terrifying ",
  "A terrorizing ",
  "An alarming ",
  "A dangerous ",
  "A glowering ",
  "A glaring ",
  "A scowling ",
  "A chilling ",
  "A scary ",
  "A creepy ",
  "An eerie ",
  "A spooky ",
  "A slobbering ",
  "A drooling ",
  "A horrendous ",
  "An unnerving ",
  "A cute little ",  /* Even though they're trying to kill you, */
  "A teeny-weenie ", /* they can still be cute!                 */
  "A fuzzy ",
  "A fluffy white ",
  "A kawaii ",       /* For our otaku */
  "Hao ke ai de ",   /* And for our Chinese */
  "Eine liebliche "  /* For our Deutch */
  /* And there's one special case (see below) */
};

static void io_scroll_monster_list(char (*s)[40], uint32_t count)
{
  uint32_t offset;
  uint32_t i;

  offset = 0;

  while (1) {
    for (i = 0; i < 13; i++) {
      mvprintw(i + 6, 19, " %-40s ", s[i + offset]);
    }
    switch (getch()) {
    case KEY_UP:
      if (offset) {
        offset--;
      }
      break;
    case KEY_DOWN:
      if (offset < (count - 13)) {
        offset++;
      }
      break;
    case 27:
    case KEY_F(1):
      return;
    }

  }
}

static void io_list_monsters_display(dungeon *d,
                                     character **c,
                                     uint32_t count)
{
  uint32_t i;
  char (*s)[40]; /* pointer to array of 40 char */

  s = (char (*)[40]) malloc(count * sizeof (*s));

  mvprintw(3, 19, " %-40s ", "");
  /* Borrow the first element of our array for this string: */
  snprintf(s[0], 40, "You know of %d monsters:", count);
  mvprintw(4, 19, " %-40s ", s);
  mvprintw(5, 19, " %-40s ", "");

  for (i = 0; i < count; i++) {
    snprintf(s[i], 40, "%16s%c: %2d %s by %2d %s",
             (c[i]->symbol == 'd' ? "A tenacious " :
              adjectives[rand() % (sizeof (adjectives) /
                                   sizeof (adjectives[0]))]),
             c[i]->symbol,
             abs(c[i]->position[dim_y] - d->pc.position[dim_y]),
             ((c[i]->position[dim_y] - d->pc.position[dim_y]) <= 0 ?
              "North" : "South"),
             abs(c[i]->position[dim_x] - d->pc.position[dim_x]),
             ((c[i]->position[dim_x] - d->pc.position[dim_x]) <= 0 ?
              "West" : "East"));
    if (count <= 13) {
      /* Handle the non-scrolling case right here. *
       * Scrolling in another function.            */
      mvprintw(i + 6, 19, " %-40s ", s[i]);
    }
  }

  if (count <= 13) {
    mvprintw(count + 6, 19, " %-40s ", "");
    mvprintw(count + 7, 19, " %-40s ", "Hit escape/F1 to continue.");
    while (getch() != 27 /* escape */)
      ;
  } else {
    mvprintw(19, 19, " %-40s ", "");
    mvprintw(20, 19, " %-40s ",
             "Arrows to scroll, escape/F1 to continue.");
    io_scroll_monster_list(s, count);
  }

  free(s);
}

static void io_list_monsters(dungeon *d)
{
  character **c;
  uint32_t x, y, count;

  c = (character **) malloc(d->num_monsters * sizeof (*c));

  /* Get a linear list of monsters */
  for (count = 0, y = 1; y < DUNGEON_Y - 1; y++) {
    for (x = 1; x < DUNGEON_X - 1; x++) {
      if (d->character_arr[y][x] && d->character_arr[y][x] != &d->pc) {
	if (fog_flag) {
	  if (!outside_light(d, d->character_arr[y][x])) {
	    c[count++] = d->character_arr[y][x];
	  }
	}else {
	  c[count++] = d->character_arr[y][x];
	}
      }
    }
  }

  /* Sort it by distance from PC */
  dungeon_ptr = d;
  qsort(c, count, sizeof (*c), compare_monster_distance);

  /* Display it */
  io_list_monsters_display(d, c, count);
  free(c);

  /* And redraw the dungeon */
  io_display(d);
}

void io_handle_input(dungeon *d)
{
  uint32_t fail_code;
  int key;

  do {
    switch (key = getch()) {
    case '7':
    case 'y':
    case KEY_HOME:
      fail_code = move_pc(d, 7);
      break;
    case '8':
    case 'k':
    case KEY_UP:
      fail_code = move_pc(d, 8);
      break;
    case '9':
    case 'u':
    case KEY_PPAGE:
      fail_code = move_pc(d, 9);
      break;
    case '6':
    case 'l':
    case KEY_RIGHT:
      fail_code = move_pc(d, 6);
      break;
    case '3':
    case 'n':
    case KEY_NPAGE:
      fail_code = move_pc(d, 3);
      break;
    case '2':
    case 'j':
    case KEY_DOWN:
      fail_code = move_pc(d, 2);
      break;
    case '1':
    case 'b':
    case KEY_END:
      fail_code = move_pc(d, 1);
      break;
    case '4':
    case 'h':
    case KEY_LEFT:
      fail_code = move_pc(d, 4);
      break;
    case '5':
    case ' ':
    case '.':
    case KEY_B2:
      fail_code = 0;
      break;
    case '>':
      fail_code = move_pc(d, '>');
      break;
    case '<':
      fail_code = move_pc(d, '<');
      break;
    case 'Q':
      d->quit = 1;
      fail_code = 0;
      break;
    case 'T':
      /* New command.  Display the distances for tunnelers.             */
      io_display_tunnel(d);
      fail_code = 1;
      break;
    case 'D':
      /* New command.  Display the distances for non-tunnelers.         */
      io_display_distance(d);
      fail_code = 1;
      break;
    case 'H':
      /* New command.  Display the hardnesses.                          */
      io_display_hardness(d);
      fail_code = 1;
      break;
    case 's':
      /* New command.  Return to normal display after displaying some   *
       * special screen.                                                */
      io_display(d);
      fail_code = 1;
      break;
    case 'L':
      fail_code = 1;
      break;
    case 'f':
      /* Toggle fog */
      fog_flag = fog_flag ? 0 : 1;
      io_display(d);
      fail_code = 1;
      break;
    case 'g':
      /* Teleport the PC to a random place in the dungeon.              */
      io_teleport_pc(d);
      fail_code = 0;
      break;
    case 'm':
      io_list_monsters(d);
      fail_code = 1;
      break;
    case 'q':
      /* Demonstrate use of the message queue.  You can use this for *
       * printf()-style debugging (though gdb is probably a better   *
       * option.  Not that it matterrs, but using this command will  *
       * waste a turn.  Set fail_code to 1 and you should be able to *
       * figure out why I did it that way.                           */
      io_queue_message("This is the first message.");
      io_queue_message("Since there are multiple messages, "
                       "you will see \"more\" prompts.");
      io_queue_message("You can use any key to advance through messages.");
      io_queue_message("Normal gameplay will not resume until the queue "
                       "is empty.");
      io_queue_message("Long lines will be truncated, not wrapped.");
      io_queue_message("io_queue_message() is variadic and handles "
                       "all printf() conversion specifiers.");
      io_queue_message("Did you see %s?", "what I did there");
      io_queue_message("When the last message is displayed, there will "
                       "be no \"more\" prompt.");
      io_queue_message("Have fun!  And happy printing!");
      fail_code = 0;
      break;
    default:
      /* Also not in the spec.  It's not always easy to figure out what *
       * key code corresponds with a given keystroke.  Print out any    *
       * unhandled key here.  Not only does it give a visual error      *
       * indicator, but it also gives an integer value that can be used *
       * for that key in this (or other) switch statements.  Printed in *
       * octal, with the leading zero, because ncurses.h lists codes in *
       * octal, thus allowing us to do reverse lookups.  If a key has a *
       * name defined in the header, you can use the name here, else    *
       * you can directly use the octal value.                          */
      mvprintw(0, 0, "Unbound key: %#o ", key);
      fail_code = 1;
    }
  } while (fail_code);
}
