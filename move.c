#include "move.h"

#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>

#include "dungeon.h"
#include "heap.h"
#include "move.h"
#include "npc.h"
#include "pc.h"
#include "character.h"
#include "utils.h"
#include "path.h"
#include "event.h"

void do_combat(dungeon_t *d, character_t *atk, character_t *def)
{
  if (def->alive) {
    def->alive = 0;
    if (def != &d->pc) {
      d->num_monsters--;
    }
    atk->kills[kill_direct]++;
    atk->kills[kill_avenged] += (def->kills[kill_direct] +
                                  def->kills[kill_avenged]);
  }
}

void move_character(dungeon_t *d, character_t *c, pair_t next)
{
  /* Check if character in tile being moved to */
  if (charpair(next) &&
      ((next[dim_y] != c->position[dim_y]) ||
       (next[dim_x] != c->position[dim_x]))) {
    do_combat(d, c, charpair(next));
  }
  
  d->character[c->position[dim_y]][c->position[dim_x]] = NULL;
  
  pair_t temp;
  temp[dim_y] = c->position[dim_y];
  temp[dim_x] = c->position[dim_x];

  /* Make sure to offset Y coord by 1 */
  if (mappair(temp) == ter_floor_room) {
    mvaddch(temp[dim_y]+1, temp[dim_x], '.');
  } else {
    mvaddch(temp[dim_y]+1, temp[dim_x], '#');
  }
  
  c->position[dim_y] = next[dim_y];
  c->position[dim_x] = next[dim_x];
  d->character[c->position[dim_y]][c->position[dim_x]] = c;
  
  mvaddch(c->position[dim_y]+1, c->position[dim_x], c->symbol);
}

void do_moves(dungeon_t *d)
{
  pair_t next;
  character_t *c;
  event_t *e;

  /* Remove the PC when it is PC turn.  Replace on next call.  This allows *
   * use to completely uninit the heap when generating a new level without *
   * worrying about deleting the PC.                                       */

  if (pc_is_alive(d)) {
    /* The PC always goes first one a tie, so we don't use new_event().  *
     * We generate one manually so that we can set the PC sequence       *
     * number to zero.                                                   */
    e = malloc(sizeof (*e));
    e->type = event_character_turn;
    /* Hack: New dungeons are marked.  Unmark and ensure PC goes at d->time, *
     * otherwise, monsters get a turn before the PC.                         */
    if (d->is_new) {
      d->is_new = 0;
      e->time = d->time;
    } else {
      e->time = d->time + (1000 / d->pc.speed);
    }
    e->sequence = 0;
    e->c = &d->pc;
    heap_insert(&d->events, e);
  }

  while (pc_is_alive(d) &&
         (e = heap_peek_min(&d->events)) &&
         ((e->type != event_character_turn) || (e->c != &d->pc))) {
    e = heap_remove_min(&d->events);
    d->time = e->time;
    if (e->type == event_character_turn) {
      c = e->c;
    }
    if (!c->alive) {
      if (d->character[c->position[dim_y]][c->position[dim_x]] == c) {
        d->character[c->position[dim_y]][c->position[dim_x]] = NULL;
      }
      if (c != &d->pc) {
        event_delete(e);
      }
      continue;
    }

    npc_next_pos(d, c, next);
    move_character(d, c, next);

    heap_insert(&d->events, update_event(d, e, 1000 / c->speed));
  }

}

uint8_t check_move(dungeon_t *d, int input, pair_t next_move)
{
  if (mvinch(0, 0) != ' ') {
    clear_status();
  }

  pair_t dir;
  dir[dim_y] = dir[dim_x] = 0;

  switch(input)
    {
    // Move up-left (7, y, KEY_HOME)
    case 55:
    case 121:
    case KEY_HOME:
      dir[dim_y] = -1;
      dir[dim_x] = -1;
      break;
      
    // Move up (8, k, KEY_UP)
    case 56:
    case 107:
    case KEY_UP:
      dir[dim_y] = -1;
      break;
      
    // Move up-right (9, u, KEY_PPAGE)
    case 57:
    case 117:
    case KEY_PPAGE:
      dir[dim_y] = -1;
      dir[dim_x] = 1;
      break;
      
    // Move right (6, l, KEY_RIGHT)
    case 54:
    case 108:
    case KEY_RIGHT:
      dir[dim_x] = 1;
      break;
      
    // Move down-right (3, n, KEY_NPAGE)
    case 51:
    case 110:
    case KEY_NPAGE:
      dir[dim_y] = 1;
      dir[dim_x] = 1;
      break;

    // Move down (2, j, KEY_DOWN)
    case 50:
    case 106:
    case KEY_DOWN:
      dir[dim_y] = 1;
      break;
      
    // Move down-left (1, b, KEY_END)
    case 49:
    case 98:
    case KEY_END:
      dir[dim_y] = 1;
      dir[dim_x] = -1;
      break;
      
    // Move left (4, h, KEY_LEFT)
    case 52:
    case 104:
    case KEY_LEFT:
      dir[dim_x] = -1;
      break;

    // Rest for a turn (5 or space)
    case 53:
    case 32:
      break;
      
    // Key not recognized so no-op
    default:
      display_key_error("Unreconized Key");
      return 1;
    }

  next_move[dim_y] = d->pc.position[dim_y] + dir[dim_y];
  next_move[dim_x] = d->pc.position[dim_x] + dir[dim_x];
  
  if (mappair(next_move) < ter_floor) {
    display_key_error("PC bangs head on wall...");
    return 1;
  }

  return 0;
}

void clear_status()
{
  uint16_t i;
  
  for (i = 0; i < DUNGEON_X; i++) {
    mvaddch(0, i, ' ');
  }
  refresh();
}

void display_key_error(char *str)
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

void dir_nearest_wall(dungeon_t *d, character_t *c, pair_t dir)
{
  dir[dim_x] = dir[dim_y] = 0;

  if (c->position[dim_x] != 1 && c->position[dim_x] != DUNGEON_X - 2) {
    dir[dim_x] = (c->position[dim_x] > DUNGEON_X - c->position[dim_x] ? 1 : -1);
  }
  if (c->position[dim_y] != 1 && c->position[dim_y] != DUNGEON_Y - 2) {
    dir[dim_y] = (c->position[dim_y] > DUNGEON_Y - c->position[dim_y] ? 1 : -1);
  }
}

uint32_t against_wall(dungeon_t *d, character_t *c)
{
  return ((mapxy(c->position[dim_x] - 1,
                 c->position[dim_y]    ) == ter_wall_immutable) ||
          (mapxy(c->position[dim_x] + 1,
                 c->position[dim_y]    ) == ter_wall_immutable) ||
          (mapxy(c->position[dim_x]    ,
                 c->position[dim_y] - 1) == ter_wall_immutable) ||
          (mapxy(c->position[dim_x]    ,
                 c->position[dim_y] + 1) == ter_wall_immutable));
}

uint32_t in_corner(dungeon_t *d, character_t *c)
{
  uint32_t num_immutable;

  num_immutable = 0;

  num_immutable += (mapxy(c->position[dim_x] - 1,
                          c->position[dim_y]    ) == ter_wall_immutable);
  num_immutable += (mapxy(c->position[dim_x] + 1,
                          c->position[dim_y]    ) == ter_wall_immutable);
  num_immutable += (mapxy(c->position[dim_x]    ,
                          c->position[dim_y] - 1) == ter_wall_immutable);
  num_immutable += (mapxy(c->position[dim_x]    ,
                          c->position[dim_y] + 1) == ter_wall_immutable);

  return num_immutable > 1;
}

uint32_t move_pc(dungeon_t *d, pair_t loc)
{
  character_t *c;
  event_t *e;

  if (pc_is_alive(d) &&
      (e = heap_peek_min(&d->events)) && e->c == &d->pc) {

    e = heap_remove_min(&d->events);
    
  } else {
    return 0;
  }
  
  c = e->c;
  d->time = e->time;
  /* Kind of kludgey, but because the PC is never in the queue when   *
   * we are outside of this function, the PC event has to get deleted *
   * and recreated every time we leave and re-enter this function.    */
  e->c = NULL;
  event_delete(e);

  //pc_next_pos(d, next);
  //next[dim_x] += c->position[dim_x];
  //next[dim_y] += c->position[dim_y]; 
  move_character(d, c, loc);
  
  dijkstra(d);

  return 0;
}
