/*
 * Header file for pathfinding file pathfinding.c
 *
 *   Author: Weston Berg (weberg@iastate.edu)
 *   Date: 10/02/2018
 */

#ifndef PATHFINDING_H
#define PATHFINDING_H

// Dungeon data structure signature
typedef struct room room_t;
typedef struct dungeon dungeon_t;

// Function prototypes
void calculate_paths(dungeon_t *);
void show_paths(dungeon_t *);

#endif
