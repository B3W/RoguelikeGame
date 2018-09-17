/*
 * Function declarations for Dungeon_Generation.c
 *
 *   Author: Weston Berg (weberg@iastate.edu)
 *   Date: 09/01/2018
 */

#ifndef DUNGEON_GENERATION_H
#define DUNGEON_GENERATION_H

#include <stdint.h>

#define TERMINAL_WIDTH 80
#define TERMINAL_HEIGHT 24
#define DUNGEON_HEIGHT 21

typedef struct room room_t;

void init_dungeon(char[TERMINAL_HEIGHT][TERMINAL_WIDTH], uint8_t[DUNGEON_HEIGHT][TERMINAL_WIDTH], char, char);

room_t * generate_dungeon(char[TERMINAL_HEIGHT][TERMINAL_WIDTH], uint8_t[DUNGEON_HEIGHT][TERMINAL_WIDTH]);

void init_rooms(uint8_t, room_t *, char[TERMINAL_HEIGHT][TERMINAL_WIDTH], uint8_t[DUNGEON_HEIGHT][TERMINAL_WIDTH]);

void render_corridors(uint8_t, room_t *, char[TERMINAL_HEIGHT][TERMINAL_WIDTH], uint8_t[DUNGEON_HEIGHT][TERMINAL_WIDTH]);

void init_dungeon_arr(char[TERMINAL_HEIGHT][TERMINAL_WIDTH], uint8_t[DUNGEON_HEIGHT][TERMINAL_WIDTH]);

room_t * load_dungeon(char[TERMINAL_HEIGHT][TERMINAL_WIDTH], uint8_t[DUNGEON_HEIGHT][TERMINAL_WIDTH]);

void save_dungeon(uint8_t[DUNGEON_HEIGHT][TERMINAL_WIDTH], room_t *);

char * get_dungeon_file_path(void);

void show_dungeon(char[TERMINAL_HEIGHT][TERMINAL_WIDTH]);

#endif
