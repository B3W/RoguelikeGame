/*
 * Function, struct, and macro declarations for Dungeon_Generation.c
 *
 *   Author: Weston Berg (weberg@iastate.edu)
 *   Date: 09/01/2018
 */

#ifndef DUNGEON_GENERATION_H
#define DUNGEON_GENERATION_H

#include <stdint.h>

#define DUNGEON_WIDTH 80
#define DUNGEON_HEIGHT 21

// Data structure for representing a room in the dungeon
// uint8_t is utilized over int to save on memory size
typedef struct room {
  uint8_t x_pos;
  uint8_t y_pos;
  uint8_t x_size;
  uint8_t y_size;
} room_t;

// Data structure for representing player character on the dungeon map
typedef struct player_character {
  uint8_t x_pos;
  uint8_t y_pos;
} pc_t;

// Data structure containing all information on the dungeon
typedef struct dungeon {
  uint8_t num_rooms;
  room_t *rooms;
  pc_t pc;
  // 2D array representing map visible to player
  unsigned char dungeon[DUNGEON_HEIGHT][DUNGEON_WIDTH];
  // 2D array representing hardness of each square in the dungeon
  uint8_t material_hardness[DUNGEON_HEIGHT][DUNGEON_WIDTH];
  // 2D path map for non-tunneling creatures
  int32_t ntnl_path_map[DUNGEON_HEIGHT][DUNGEON_WIDTH];
  // 2D path map for tunneling creatures
  int32_t tnl_path_map[DUNGEON_HEIGHT][DUNGEON_WIDTH];
} dungeon_t;


uint8_t init_dungeon(dungeon_t *, char, char);
uint8_t load_dungeon(dungeon_t *);
uint8_t save_dungeon(dungeon_t *);
void del_dungeon(dungeon_t *d);
void show_dungeon(dungeon_t *);

void calculate_paths(dungeon_t *);
void show_paths(dungeon_t *);

#endif
