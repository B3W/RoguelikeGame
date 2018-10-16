#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

/* Very slow seed: 686846853 */

#include "dungeon.h"

#include "pc.h"
#include "npc.h"
#include "move.h"
#include "utils.h"

const char *victory =
  "\n                                       o\n"
  "                                      $\"\"$o\n"
  "                                     $\"  $$\n"
  "                                      $$$$\n"
  "                                      o \"$o\n"
  "                                     o\"  \"$\n"
  "                oo\"$$$\"  oo$\"$ooo   o$    \"$    ooo\"$oo  $$$\"o\n"
  "   o o o o    oo\"  o\"      \"o    $$o$\"     o o$\"\"  o$      \"$  "
  "\"oo   o o o o\n"
  "   \"$o   \"\"$$$\"   $$         $      \"   o   \"\"    o\"         $"
  "   \"o$$\"    o$$\n"
  "     \"\"o       o  $          $\"       $$$$$       o          $  ooo"
  "     o\"\"\n"
  "        \"o   $$$$o $o       o$        $$$$$\"       $o        \" $$$$"
  "   o\"\n"
  "         \"\"o $$$$o  oo o  o$\"         $$$$$\"        \"o o o o\"  "
  "\"$$$  $\n"
  "           \"\" \"$\"     \"\"\"\"\"            \"\"$\"            \""
  "\"\"      \"\"\" \"\n"
  "            \"oooooooooooooooooooooooooooooooooooooooooooooooooooooo$\n"
  "             \"$$$$\"$$$$\" $$$$$$$\"$$$$$$ \" \"$$$$$\"$$$$$$\"  $$$\""
  "\"$$$$\n"
  "              $$$oo$$$$   $$$$$$o$$$$$$o\" $$$$$$$$$$$$$$ o$$$$o$$$\"\n"
  "              $\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\""
  "\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"$\n"
  "              $\"                                                 \"$\n"
  "              $\"$\"$\"$\"$\"$\"$\"$\"$\"$\"$\"$\"$\"$\"$\"$\"$\"$\"$\""
  "$\"$\"$\"$\"$\"$\"$\"$\n"
  "                                   You win!\n\n";

const char *tombstone =
  "\n\n\n\n                /\"\"\"\"\"/\"\"\"\"\"\"\".\n"
  "               /     /         \\             __\n"
  "              /     /           \\            ||\n"
  "             /____ /   Rest in   \\           ||\n"
  "            |     |    Pieces     |          ||\n"
  "            |     |               |          ||\n"
  "            |     |   A. Luser    |          ||\n"
  "            |     |               |          ||\n"
  "            |     |     * *   * * |         _||_\n"
  "            |     |     *\\/* *\\/* |        | TT |\n"
  "            |     |     *_\\_  /   ...\"\"\"\"\"\"| |"
  "| |.\"\"....\"\"\"\"\"\"\"\".\"\"\n"
  "            |     |         \\/..\"\"\"\"\"...\"\"\""
  "\\ || /.\"\"\".......\"\"\"\"...\n"
  "            |     |....\"\"\"\"\"\"\"........\"\"\"\"\""
  "\"^^^^\".......\"\"\"\"\"\"\"\"..\"\n"
  "            |......\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"......"
  "..\"\"\"\"\"....\"\"\"\"\"..\"\"...\"\"\".\n\n"
  "            You're dead.  Better luck in the next life.\n\n\n";

void usage(char *name)
{
  fprintf(stderr,
          "Usage: %s [-r|--rand <seed>] [-l|--load [<file>]]\n"
          "          [-s|--save [<file>]] [-i|--image <pgm file>]\n"
          "          [-n|--nummon <count>]",
          name);

  exit(-1);
}


/*
 * Initialize ncurses for the terminal
 */
void init_io(void)
{
  initscr();             // Initialize terminal
  raw();                 // Turn off buffered IO
  noecho();              // Don't echo input
  curs_set(0);           // Make cursor invisible
  keypad(stdscr, TRUE);  // Turn on keypad for the terminal
}


/*
 * Overlay the dungeon with of list of monsters and
 * their relative position to the PC.
 *
 * KEY_DOWN will scroll down if monster list too big
 * KEY_UP will scroll up to beginning of list
 * Escape or F1 will close out the list
 */
void display_monster_list(dungeon_t *d)
{
  /* New window parameters */
  uint8_t win_x = 10;
  uint8_t win_y = 1;
  uint8_t win_width = DUNGEON_X - (win_x << 1);
  uint8_t win_height = DUNGEON_Y;

  /* Create new window */
  WINDOW *monster_win;
  monster_win = newwin(win_height, win_width, win_y, win_x);
  keypad(monster_win, TRUE);
  box(monster_win, 0, 0);
  
  /* Create buffer containing type and    *
   * location of monsters in the dungeon  */
  char buffer[d->num_monsters+4][win_width-2];  // Four lines for title, monster count, and escape directions; 2 bytes x direction for border
  sprintf(buffer[0], "%s", "DUNGEON DOSSIER");
  mvwprintw(monster_win, 0, 1, "%s", buffer[0]);  // mvwprintw(WINDOW, y, x, Format, args)
  sprintf(buffer[1], "Presence of %d monsters detected!", d->num_monsters);
  mvwprintw(monster_win, 1, 1, "%s", buffer[1]);
  
  character_t *tmp_c;
  char *x_dir, *y_dir;
  uint8_t line_num = 2;
  uint8_t max_print_height = win_height - 1;
  uint8_t i, j;
  int8_t x_dist, y_dist;
  for (j = 0; j < DUNGEON_Y; j++) {
    for (i = 0; i < DUNGEON_X; i++) {
      tmp_c = d->character[j][i];

      // Make sure character is not the pc and it is alive (alive check may not be necessary)
      if (tmp_c != NULL && tmp_c != &d->pc && tmp_c->alive) {

	// Calculate distance to PC
	x_dist = tmp_c->position[dim_x] - d->pc.position[dim_x];
	y_dist = d->pc.position[dim_y] - tmp_c->position[dim_y];
	if (x_dist < 0) {
	  x_dir = "West";
	} else {
	  x_dir = "East";
	}
	if (y_dist < 0) {
	  y_dir = "South";
	} else {
	  y_dir = "North";
	}

	// Write data into buffer and print onto the window if room available
	sprintf(buffer[line_num], "%c: %d %s, %d %s", tmp_c->symbol, abs(x_dist), x_dir, abs(y_dist), y_dir);
	if (line_num < max_print_height) {
	  mvwprintw(monster_win, line_num, 1, "%s", buffer[line_num]);
	}
	line_num++;
      }
    }
  }
  sprintf(buffer[line_num], "%s", " ");
  if (line_num < max_print_height) {
    mvwprintw(monster_win, line_num, 1, "%s", buffer[line_num]);
  }
  line_num++;
  sprintf(buffer[line_num], "%s", "Press ESCAPE or F1 to Continue Quest!");
  if (line_num < max_print_height) {
    mvwprintw(monster_win, line_num, 1, "%s", buffer[line_num]);
  }

  /* Display window */
  wrefresh(monster_win);

  /* Use wgetch for input for windows other than stdscr */
  int mwin_input;
  uint8_t exit_flag = 0;
  uint8_t curLine;
  uint8_t buffer_frame_start = 1;
  uint8_t buffer_frame_end = (line_num < max_print_height) ? line_num : max_print_height-1; 
  
  while (!exit_flag) {
    mwin_input = wgetch(monster_win);
    switch(mwin_input)
      {
      case KEY_UP:    // Scroll up when needed
	if (buffer_frame_start != 1) {
	  // Clear window
	  wclear(monster_win);
	  // Increment the frame
	  buffer_frame_start--;
	  buffer_frame_end--;
	  curLine = buffer_frame_start;
	  for (j = 1; j < max_print_height; j++) {
	    // Print buffer
	    mvwprintw(monster_win, j, 1, "%s", buffer[curLine]);
	    curLine++;
	  }
	  // Restore border
	  box(monster_win, 0, 0);
	  // Restore title
	  mvwprintw(monster_win, 0, 1, "%s", buffer[0]);

	  wrefresh(monster_win);
	}
	break;

      case KEY_DOWN:  // Scroll down when needed
	if (buffer_frame_end != line_num) {
	  // Clear window
	  wclear(monster_win);
	  // Increment the frame
	  buffer_frame_start++;
	  buffer_frame_end++;
	  curLine = buffer_frame_start;
	  for (j = 1; j < max_print_height; j++) {
	    // Print buffer
	    mvwprintw(monster_win, j, 1, "%s", buffer[curLine]);
	    curLine++;
	  }
	  // Restore border
	  box(monster_win, 0, 0);
	  // Restore title
	  mvwprintw(monster_win, 0, 1, "%s", buffer[0]);

	  wrefresh(monster_win);
	}
        break;

      case 27:  // Close window
      case KEY_F(1):
	exit_flag = 1;
      }
  }

  /* Clear the border then deallocate memory for monster window */
  wborder(monster_win, ' ', ' ', ' ',' ',' ',' ',' ',' ');
  wrefresh(monster_win);
  delwin(monster_win);
}


int main(int argc, char *argv[])
{
  dungeon_t d;
  time_t seed;
  struct timeval tv;
  uint32_t i;
  uint32_t do_load, do_save, do_seed, do_image, do_save_seed, do_save_image;
  uint32_t long_arg;
  char *save_file;
  char *load_file;
  char *pgm_file;

  /* Quiet a false positive from valgrind. */
  memset(&d, 0, sizeof (d));
  
  /* Default behavior: Seed with the time, generate a new dungeon, *
   * and don't write to disk.                                      */
  do_load = do_save = do_image = do_save_seed = do_save_image = 0;
  do_seed = 1;
  save_file = load_file = NULL;
  d.max_monsters = MAX_MONSTERS;

  /* The project spec requires '--load' and '--save'.  It's common  *
   * to have short and long forms of most switches (assuming you    *
   * don't run out of letters).  For now, we've got plenty.  Long   *
   * forms use whole words and take two dashes.  Short forms use an *
    * abbreviation after a single dash.  We'll add '--rand' (to     *
   * specify a random seed), which will take an argument of it's    *
   * own, and we'll add short forms for all three commands, '-l',   *
   * '-s', and '-r', respectively.  We're also going to allow an    *
   * optional argument to load to allow us to load non-default save *
   * files.  No means to save to non-default locations, however.    *
   * And the final switch, '--image', allows me to create a dungeon *
   * from a PGM image, so that I was able to create those more      *
   * interesting test dungeons for you.                             */
 
 if (argc > 1) {
    for (i = 1, long_arg = 0; i < argc; i++, long_arg = 0) {
      if (argv[i][0] == '-') { /* All switches start with a dash */
        if (argv[i][1] == '-') {
          argv[i]++;    /* Make the argument have a single dash so we can */
          long_arg = 1; /* handle long and short args at the same place.  */
        }
        switch (argv[i][1]) {
        case 'n':
          if ((!long_arg && argv[i][2]) ||
              (long_arg && strcmp(argv[i], "-nummon")) ||
              argc < ++i + 1 /* No more arguments */ ||
              !sscanf(argv[i], "%hu", &d.max_monsters)) {
            usage(argv[0]);
          }
          break;
        case 'r':
          if ((!long_arg && argv[i][2]) ||
              (long_arg && strcmp(argv[i], "-rand")) ||
              argc < ++i + 1 /* No more arguments */ ||
              !sscanf(argv[i], "%lu", &seed) /* Argument is not an integer */) {
            usage(argv[0]);
          }
          do_seed = 0;
          break;
        case 'l':
          if ((!long_arg && argv[i][2]) ||
              (long_arg && strcmp(argv[i], "-load"))) {
            usage(argv[0]);
          }
          do_load = 1;
          if ((argc > i + 1) && argv[i + 1][0] != '-') {
            /* There is another argument, and it's not a switch, so *
             * we'll treat it as a save file and try to load it.    */
            load_file = argv[++i];
          }
          break;
        case 's':
          if ((!long_arg && argv[i][2]) ||
              (long_arg && strcmp(argv[i], "-save"))) {
            usage(argv[0]);
          }
          do_save = 1;
          if ((argc > i + 1) && argv[i + 1][0] != '-') {
            /* There is another argument, and it's not a switch, so *
             * we'll save to it.  If it is "seed", we'll save to    *
	     * <the current seed>.rlg327.  If it is "image", we'll  *
	     * save to <the current image>.rlg327.                  */
	    if (!strcmp(argv[++i], "seed")) {
	      do_save_seed = 1;
	      do_save_image = 0;
	    } else if (!strcmp(argv[i], "image")) {
	      do_save_image = 1;
	      do_save_seed = 0;
	    } else {
	      save_file = argv[i];
	    }
          }
          break;
        case 'i':
          if ((!long_arg && argv[i][2]) ||
              (long_arg && strcmp(argv[i], "-image"))) {
            usage(argv[0]);
          }
          do_image = 1;
          if ((argc > i + 1) && argv[i + 1][0] != '-') {
            /* There is another argument, and it's not a switch, so *
             * we'll treat it as a save file and try to load it.    */
            pgm_file = argv[++i];
          }
          break;
        default:
          usage(argv[0]);
        }
      } else { /* No dash */
        usage(argv[0]);
      }
    }
  }

  if (do_seed) {
    /* Allows me to generate more than one dungeon *
     * per second, as opposed to time().           */
    gettimeofday(&tv, NULL);
    seed = (tv.tv_usec ^ (tv.tv_sec << 20)) & 0xffffffff;
  }

  srand(seed);

  /* Configure terminal for user input */
  init_io();

  int user_input;
  int invalid_op;
  pair_t pc_move;

  ///////// TODO SAVE KILL COUNT ACCROSS DUNGEON INSTANCES \\\\\\\\
  
 NEW_DUNGEON:
  clear();

  /* Begin Dungeon Generation */
  init_dungeon(&d);

  if (do_load) {
    read_dungeon(&d, load_file);
  } else if (do_image) {
    read_pgm(&d, pgm_file);
  } else {
    gen_dungeon(&d);
  }

  /* Ignoring PC position in saved dungeons.  Not a bug. */
  config_pc(&d);
  gen_monsters(&d);
  place_stairs(&d);
  
  render_dungeon(&d);  // Place the dungeon and monsters with status bar on top into terminal
  do_moves(&d);        // Place PC in starting position of event queue
  refresh();           // Show the dungeon
  
  while (pc_is_alive(&d) && dungeon_has_npcs(&d)) {
    invalid_op = 1;
    
    while (invalid_op) {
      pc_move[dim_y] = pc_move[dim_x] = 0;

      /* Get user input */
      user_input = getch();

      if (mvinch(0, 0) != ' ') {
	clear_status();
      }
      
      switch(user_input)
	{
	case 81: // Quit game
	  goto EXIT;
	  
	case 60: // Go up '<' ladder if possible
	  if (d.map[d.pc.position[dim_y]][d.pc.position[dim_x]] >= ter_stair) {
	    if (d.map[d.pc.position[dim_y]][d.pc.position[dim_x]] == ter_stair_up) {
	      pc_delete(d.pc.pc);
	      delete_dungeon(&d);
	      goto NEW_DUNGEON;
	    }
	  } else {
	    display_message("Ground seems firm. No staircase here.");
	  }
	  break;
	  
	case 62: // Go down '>' ladder if possible
	  if (d.map[d.pc.position[dim_y]][d.pc.position[dim_x]] >= ter_stair) {
	    if(d.map[d.pc.position[dim_y]][d.pc.position[dim_x]] == ter_stair_down) {
	      pc_delete(d.pc.pc);
	      delete_dungeon(&d);
	      goto NEW_DUNGEON;
	    }
	  } else {
	    display_message("Hmmm, no way up from here.");
	  }
	  break;
	  
	case 109: // Display the monster list
	  display_monster_list(&d);	  
	  render_dungeon(&d);
	  refresh();
	  break;

	default: // Wanting to move character. Check move and act accordingly
	  invalid_op = check_move(&d, user_input, pc_move);
	  
	}
    }
    move_pc(&d, pc_move);
    do_moves(&d);
    refresh();
    usleep(33000);
  }

 EXIT:

  if (do_save) {
    if (do_save_seed) {
       /* 10 bytes for number, please dot, extention and null terminator. */
      save_file = malloc(18);
      sprintf(save_file, "%ld.rlg327", seed);
    }
    if (do_save_image) {
      if (!pgm_file) {
	fprintf(stderr, "No image file was loaded.  Using default.\n");
	do_save_image = 0;
      } else {
	/* Extension of 3 characters longer than image extension + null. */
	save_file = malloc(strlen(pgm_file) + 4);
	strcpy(save_file, pgm_file);
	strcpy(strchr(save_file, '.') + 1, "rlg327");
      }
    }
    write_dungeon(&d, save_file);

    if (do_save_seed || do_save_image) {
      free(save_file);
    }
  }

  /* Print results */
  clear();
  printw("%s\nYou defended your life in the face of %u deadly beasts.\n"
         "You avenged the cruel and untimely murders of %u "
         "peaceful dungeon residents.\n", pc_is_alive(&d) ? victory : tombstone, d.pc.kills[kill_direct], d.pc.kills[kill_avenged]);

  refresh();
  getch();

  /* Deinit the terminal */
  endwin();

  pc_delete(d.pc.pc);

  delete_dungeon(&d);

  return 0;
}
