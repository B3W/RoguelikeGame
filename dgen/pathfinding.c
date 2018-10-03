/*
 * Functions for calculating pathing in a corridor based on Djikstra's Algorithm
 *
 *   Author: Weston Berg (weberg@iastate.edu)
 *   Date: 10/02/2018
 */


/* Preprocessing */
#include <limits.h>
#include <stdio.h>
#include "dgen.h"
#include "../pqueue/heap.h"
#include "pathfinding.h"


/* Data Structures */
/* Represents a single tile in the map for calculating the path distance via Dijkstra's Algorithm */
typedef struct path_node {
  heap_node_t *hn;
  uint8_t x_pos;
  uint8_t y_pos;
  int32_t cost;
} path_node_t;


/* Static function prototypes */
static int32_t path_node_cmp(const void *, const void *);
static uint8_t calculate_ntnl_path(path_node_t *, path_node_t[DUNGEON_HEIGHT][DUNGEON_WIDTH], heap_t *);
static uint8_t calculate_tnl_path(path_node_t *, path_node_t[DUNGEON_HEIGHT][DUNGEON_WIDTH], heap_t *, dungeon_t *);


/*
 * Function for comparing two nodes on the path with each other
 *
 * @param key  node getting compared
 * @param with  node being compared against
 */
static int32_t path_node_cmp(const void *key, const void *with)
{
  return ((path_node_t *) key)->cost - ((path_node_t *) with)->cost;
}


/*
 * Calculates the movement cost of each tile and populates
 * path maps for tunneling and non-tunneling monsters
 *
 * @param d
 */
void calculate_paths(dungeon_t *d)
{
  static path_node_t ntnl_path[DUNGEON_HEIGHT][DUNGEON_WIDTH], *ntnl_p;
  static path_node_t tnl_path[DUNGEON_HEIGHT][DUNGEON_WIDTH], *tnl_p;
  static uint8_t paths_initialized = 0;
  heap_t ntnl_heap, tnl_heap;
  uint8_t x, y;

  // Only need to initialize the x and y coordinates of the path arrays
  // the first time that this function is called
  if (!paths_initialized) {
    for (y = 0; y < DUNGEON_HEIGHT; y++) {
      for (x = 0; x < DUNGEON_WIDTH; x++) {
       
	ntnl_path[y][x].x_pos = x;
	ntnl_path[y][x].y_pos = y;

	tnl_path[y][x].x_pos = x;
	tnl_path[y][x].y_pos = y;
      }
    }
    paths_initialized = 1;
  }

  // Set cost of all nodes to the max integer value (~infinity)
  for (y = 0; y < DUNGEON_HEIGHT; y++) {
    for (x = 0; x < DUNGEON_WIDTH; x++) {

      ntnl_path[y][x].cost = INT_MAX;

      tnl_path[y][x].cost = INT_MAX;
    }
  }

  // Set position of the Player Character to cost of 0
  ntnl_path[d->pc.y_pos][d->pc.x_pos].cost = 0;
  tnl_path[d->pc.y_pos][d->pc.x_pos].cost = 0;

  // Initialize heaps (priority queues)
  heap_init(&ntnl_heap, path_node_cmp, NULL);
  heap_init(&tnl_heap, path_node_cmp, NULL);

  // Insert path nodes into respective heap
  for (y = 0; y < DUNGEON_HEIGHT; y++){
    for (x = 0; x < DUNGEON_WIDTH; x++) {

      // If on the dungeon border then that path node is NULL
      if (d->material_hardness[y][x] != 255) {
	
	// Insert non-tunneling path nodes into non-tunneling heap
	if (d->material_hardness[y][x] == 0) {
	  ntnl_path[y][x].hn = heap_insert(&ntnl_heap, &ntnl_path[y][x]);
	} else {
	  ntnl_path[y][x].hn = NULL;
	}

	// Insert tunneling path nodes into tunneling heap
	tnl_path[y][x].hn = heap_insert(&tnl_heap, &tnl_path[y][x]);

      } else {

	ntnl_path[y][x].hn = NULL;
	tnl_path[y][x].hn = NULL;
      }
    }
  }


  // Calculate non-tunneling path map
  calculate_ntnl_path(ntnl_p, ntnl_path, &ntnl_heap);

  // Calculate tunneling path map
  calculate_tnl_path(tnl_p, tnl_path, &tnl_heap, d);

  
  // Store it into the dungeon struct
  for (y = 0; y < DUNGEON_HEIGHT; y++) {
    for (x = 0; x < DUNGEON_WIDTH; x++) {
 
      d->ntnl_path_map[y][x] = ntnl_path[y][x].cost;
      d->tnl_path_map[y][x] = tnl_path[y][x].cost;
    }
  }
}


/*
 * Calculates increasing path costs eminating from the origin tile
 * Origin tile represents location of PC and has a cost of 0
 * Only considers paths for non_tunneling monsters
 *
 * @param p  pointer to path nodes returned from the heap
 * @param path  2D array containing all path nodes in the dungeon
 * @param h  heap containing all path nodes filtered via priority
 */
static uint8_t calculate_ntnl_path(path_node_t *p, path_node_t path[DUNGEON_HEIGHT][DUNGEON_WIDTH], heap_t *h)
{
  // For every node in the priority queue remove it and calculate
  // the cost for each of it's neighbors along the path
  while ((p = heap_remove_min(h))) {

    // Delete the current node
    p->hn = NULL;

    // Check neighbors for null then change the cost if it is greater the 'current' node
    if ((path[p->y_pos - 1][p->x_pos - 1].hn) &&  // UP LEFT
        (path[p->y_pos - 1][p->x_pos - 1].cost > p->cost)) {

      path[p->y_pos - 1][p->x_pos - 1].cost = p->cost + 1;
      
      heap_decrease_key_no_replace(h, path[p->y_pos - 1][p->x_pos - 1].hn);
    }
    
    if ((path[p->y_pos - 1][p->x_pos].hn) &&  // UP
        (path[p->y_pos - 1][p->x_pos].cost > p->cost)) {

      path[p->y_pos - 1][p->x_pos].cost = p->cost + 1;

      heap_decrease_key_no_replace(h, path[p->y_pos - 1][p->x_pos].hn);
    }

    if ((path[p->y_pos - 1][p->x_pos + 1].hn) &&  // UP RIGHT
        (path[p->y_pos - 1][p->x_pos + 1].cost > p->cost)) {

      path[p->y_pos - 1][p->x_pos + 1].cost = p->cost + 1;
      
      heap_decrease_key_no_replace(h, path[p->y_pos - 1][p->x_pos + 1].hn);
    }
    
    if ((path[p->y_pos][p->x_pos + 1].hn) &&  // RIGHT
        (path[p->y_pos][p->x_pos + 1].cost > p->cost)) {

      path[p->y_pos][p->x_pos + 1].cost = p->cost + 1;
      
      heap_decrease_key_no_replace(h, path[p->y_pos][p->x_pos + 1].hn);
    }

    if ((path[p->y_pos + 1][p->x_pos + 1].hn) &&  // DOWN RIGHT
        (path[p->y_pos + 1][p->x_pos + 1].cost > p->cost)) {

      path[p->y_pos + 1][p->x_pos + 1].cost = p->cost + 1;
      
      heap_decrease_key_no_replace(h, path[p->y_pos + 1][p->x_pos + 1].hn);
    }

    if ((path[p->y_pos + 1][p->x_pos].hn) &&  // DOWN
        (path[p->y_pos + 1][p->x_pos].cost > p->cost)) {

      path[p->y_pos + 1][p->x_pos].cost = p->cost + 1;
      
      heap_decrease_key_no_replace(h, path[p->y_pos + 1][p->x_pos].hn);
    }

    if ((path[p->y_pos + 1][p->x_pos - 1].hn) &&  // DOWN LEFT
        (path[p->y_pos + 1][p->x_pos - 1].cost > p->cost)) {

      path[p->y_pos + 1][p->x_pos - 1].cost = p->cost + 1;
      
      heap_decrease_key_no_replace(h, path[p->y_pos + 1][p->x_pos - 1].hn);
    }

    if ((path[p->y_pos][p->x_pos - 1].hn) &&  // LEFT
        (path[p->y_pos][p->x_pos - 1].cost > p->cost)) {

      path[p->y_pos][p->x_pos - 1].cost = p->cost + 1;
      
      heap_decrease_key_no_replace(h, path[p->y_pos][p->x_pos - 1].hn);
    }

  }
  return 0;
}


/*
 * Calculates increasing path costs eminating from the origin tile
 * Origin tile represents location of PC and has a cost of 0
 * Takes paths through rocks into account
 *
 * @param p  pointer to path nodes returned from the heap
 * @param path  2D array containing all path nodes in the dungeon
 * @param h  heap containing all path nodes filtered via priority
 * @param d  dungeon containing relevant hardness data
 */
static uint8_t calculate_tnl_path(path_node_t *p, path_node_t path[DUNGEON_HEIGHT][DUNGEON_WIDTH], heap_t *h, dungeon_t *d)
{
  // For every node in the priority queue remove it and calculate
  // the cost for each of it's neighbors along the path
  while ((p = heap_remove_min(h))) {

    // Delete the current node
    p->hn = NULL;

    // Check neighbors for null then change the cost if it is greater the 'current' node
    // Cost of going through rock is (rock_hardness / rock_factor)
    if ((path[p->y_pos - 1][p->x_pos - 1].hn) &&  // UP LEFT
        (path[p->y_pos - 1][p->x_pos - 1].cost > p->cost + (d->material_hardness[p->y_pos][p->x_pos] / 85) + 1)) {

      path[p->y_pos - 1][p->x_pos - 1].cost = p->cost + (d->material_hardness[p->y_pos][p->x_pos] / 85) + 1;
      
      heap_decrease_key_no_replace(h, path[p->y_pos - 1][p->x_pos - 1].hn);
    }
    
    if ((path[p->y_pos - 1][p->x_pos].hn) &&  // UP
        (path[p->y_pos - 1][p->x_pos].cost > p->cost + (d->material_hardness[p->y_pos][p->x_pos] / 85) + 1)) {

      path[p->y_pos - 1][p->x_pos].cost = p->cost + (d->material_hardness[p->y_pos][p->x_pos] / 85) + 1;
            
      heap_decrease_key_no_replace(h, path[p->y_pos - 1][p->x_pos].hn);
    }

    if ((path[p->y_pos - 1][p->x_pos + 1].hn) &&  // UP RIGHT
        (path[p->y_pos - 1][p->x_pos + 1].cost > p->cost + (d->material_hardness[p->y_pos][p->x_pos] / 85) + 1)) {

      path[p->y_pos - 1][p->x_pos + 1].cost = p->cost + (d->material_hardness[p->y_pos][p->x_pos] / 85) + 1;
      
      heap_decrease_key_no_replace(h, path[p->y_pos - 1][p->x_pos + 1].hn);
    }
    
    if ((path[p->y_pos][p->x_pos + 1].hn) &&  // RIGHT
        (path[p->y_pos][p->x_pos + 1].cost > p->cost + (d->material_hardness[p->y_pos][p->x_pos] / 85) + 1)) {

      path[p->y_pos][p->x_pos + 1].cost = p->cost + (d->material_hardness[p->y_pos][p->x_pos] / 85) + 1;
      
      heap_decrease_key_no_replace(h, path[p->y_pos][p->x_pos + 1].hn);
    }

    if ((path[p->y_pos + 1][p->x_pos + 1].hn) &&  // DOWN RIGHT
        (path[p->y_pos + 1][p->x_pos + 1].cost > p->cost + (d->material_hardness[p->y_pos][p->x_pos] / 85) + 1)) {

      path[p->y_pos + 1][p->x_pos + 1].cost = p->cost + (d->material_hardness[p->y_pos][p->x_pos] / 85) + 1;
            
      heap_decrease_key_no_replace(h, path[p->y_pos + 1][p->x_pos + 1].hn);
    }

    if ((path[p->y_pos + 1][p->x_pos].hn) &&  // DOWN
        (path[p->y_pos + 1][p->x_pos].cost > p->cost + (d->material_hardness[p->y_pos][p->x_pos] / 85) + 1)) {

      path[p->y_pos + 1][p->x_pos].cost = p->cost + (d->material_hardness[p->y_pos][p->x_pos] / 85) + 1;
      
      heap_decrease_key_no_replace(h, path[p->y_pos + 1][p->x_pos].hn);
    }

    if ((path[p->y_pos + 1][p->x_pos - 1].hn) &&  // DOWN LEFT
        (path[p->y_pos + 1][p->x_pos - 1].cost > p->cost + (d->material_hardness[p->y_pos][p->x_pos] / 85) + 1)) {

      path[p->y_pos + 1][p->x_pos - 1].cost = p->cost + (d->material_hardness[p->y_pos][p->x_pos] / 85) + 1;
            
      heap_decrease_key_no_replace(h, path[p->y_pos + 1][p->x_pos - 1].hn);
    }

    if ((path[p->y_pos][p->x_pos - 1].hn) &&  // LEFT
        (path[p->y_pos][p->x_pos - 1].cost > p->cost + (d->material_hardness[p->y_pos][p->x_pos] / 85) + 1)) {

      path[p->y_pos][p->x_pos - 1].cost = p->cost + (d->material_hardness[p->y_pos][p->x_pos] / 85) + 1;
            
      heap_decrease_key_no_replace(h, path[p->y_pos][p->x_pos - 1].hn);
    }

  }
  return 0;
}



/*
 * Function for displaying the path maps of the dungeon
 */
void show_paths(dungeon_t *d)
{
  uint8_t x, y;

  // Show non-tunneling path map
  for (y = 0; y < DUNGEON_HEIGHT; y++) {
    for (x = 0; x < DUNGEON_WIDTH; x++) {

      // INT_MAX is the value for invalid locations 
      if (d->ntnl_path_map[y][x] == INT_MAX) {

	putchar(' ');

      } else {

	if (d->ntnl_path_map[y][x] == 0) {

	  putchar('@');
	  
	} else {
	  
	  printf("%d", d->ntnl_path_map[y][x] % 10);
	}
      }
    }
    putchar('\n');
  }

  // Show tunneling path map
  for (y = 0; y < DUNGEON_HEIGHT; y++) {
    for (x = 0; x < DUNGEON_WIDTH; x++) {

      // INT_MAX is the value for invalid locations 
      if (d->tnl_path_map[y][x] == INT_MAX) {

	putchar(' ');

      } else {

	if (d->tnl_path_map[y][x] == 0) {

	  putchar('@');
	  
	} else {
	  
	  printf("%d", d->tnl_path_map[y][x] % 10);
	}
      }
    }
    putchar('\n');
  }

}
