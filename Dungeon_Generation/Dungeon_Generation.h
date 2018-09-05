/*
 * Function declarations for Dungeon_Generation.c
 *
 *   Author: Weston Berg (weberg@iastate.edu)
 *   Date: 09/01/2018
 */

#ifndef DUNGEON_GENERATION_H
#define DUNGEON_GENERATION_H

#define TERMINAL_WIDTH 80
#define TERMINAL_HEIGHT 24

struct room;

void init_rooms(char, struct room *, char[TERMINAL_HEIGHT][TERMINAL_WIDTH]);

void render_room(struct room *, char[TERMINAL_HEIGHT][TERMINAL_WIDTH]);

void init_room(struct room *, char, char, char, char, char *); 

void init_dungeon_arr(char[TERMINAL_HEIGHT][TERMINAL_WIDTH]);

void show_dungeon(char[TERMINAL_HEIGHT][TERMINAL_WIDTH]);

#endif
