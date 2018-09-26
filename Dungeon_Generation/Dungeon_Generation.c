/*
 * Source for creating the dungeon mapping in the Rougelike game.
 * Dungeon fits inside an 80 wide x 21 tall terminal and features
 * rock with rooms set inside it and corridors linking the rooms.
 *
 *   Author: Weston Berg (weberg@iastate.edu)
 *   Date: 09/01/2018
 */

#include <endian.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "Dungeon_Generation.h"
#include "heap.h"


/* Macro Definitions */
/* Returns random number in range [min, min + range) */
#define rand_range(min, range) ((rand() % range) + min)

/* Graphics Rendering User Changeable Settings */
#define ROCK_CHAR ' '
#define ROOM_CHAR '.'
#define CORRIDOR_CHAR '#'
#define PLAYER_CHAR '@'

/* Graphics Rendering Non-Changeable Settings */
#define MIN_ROOM_COUNT 5
#define ROOM_COUNT_RANGE 3
#define MIN_ROOM_X_SIZE 3
#define MIN_ROOM_Y_SIZE 2
#define ROOM_SIZE_RANGE 5
#define ROOM_PADDING 1

/* Hardness values macros */
#define DUNGEON_BORDER_HARDNESS 255
#define MIN_ROCK_HARDNESS 1
#define ROCK_HARDNESS_RANGE 254
#define CORRIDOR_HARDNESS 0
#define ROOM_HARDNESS 0


/* Data Structures */
/* Represents a single tile in the map for calculating the path distance via Dijkstra's Algorithm */
typedef struct path_node {
  heap_node_t *hn;
  uint8_t x_pos;
  uint8_t y_pos;
  int32_t cost;
} path_node_t;


/* Function Prototypes */
/* Use static identifier as they are only needed within this compilation unit */
static uint8_t generate_dungeon(dungeon_t *d);
static uint8_t init_dungeon_arr(dungeon_t *d);
static uint8_t init_rooms(dungeon_t *d);
static uint8_t render_corridors(dungeon_t *d);
static uint8_t draw_simple_path(dungeon_t *, uint8_t, uint8_t, uint8_t, uint8_t);
static uint8_t read_hardness(dungeon_t *, FILE *);
static uint8_t read_rooms(dungeon_t *, FILE *);
static uint8_t write_hardness(dungeon_t *d, FILE *file);
static uint8_t write_rooms(dungeon_t *d, FILE *file);
static char * get_dungeon_file_path(void);
static int32_t path_node_cmp(const void *, const void *);
static uint8_t calculate_ntnl_path(path_node_t *, path_node_t[DUNGEON_HEIGHT][DUNGEON_WIDTH], heap_t *);
static uint8_t calculate_tnl_path(path_node_t *, path_node_t[DUNGEON_HEIGHT][DUNGEON_WIDTH], heap_t *, dungeon_t *);
  

/*
 * Function for initializing a new dungeon.
 *
 * @param d  dungeon struct to store relevant data on dungeon
 * @param load_flag  flag to determine if dungeon should be loaded from disk
 * @param save_flag  flag to determine if dungeon should be saved to disk
 */
uint8_t init_dungeon(dungeon_t *d, char load_flag, char save_flag)
{  
  // Determine wheter to load in a dungeon or create a new one
  if (load_flag) {

    // Check if dungeon file exists, if not generate a new one
    char *file_path = get_dungeon_file_path();
    FILE *file;
    if ((file = fopen(file_path, "r"))) {
      
      // Dungeon file exists so load existing
      free (file_path);
      fclose(file);

      if(load_dungeon(d)) {
	printf("Failed to load dungeon\n");
	return 1;
      }

    } else {
      
      // Dungeon file doesn't exist so create new dungeon  
      free (file_path);
      if(generate_dungeon(d)) {
	printf("Failed to generate new dungeon\n");
	return 1;
      }
    }
    
  } else {
      
    // Create new dungeon
    if(generate_dungeon(d)) {
      printf("Failed to generate new dungeon\n");
      return 1;
    }
    
  }
  
  // Determine whether to save the dungeon back to disk or not
  if (save_flag) {

    if(save_dungeon(d)) {
      printf("Failed to save dungeon to disk\n");
      return 1;
    }
    
  }

  return 0;
}


/*
 * Function for generating a new dungeon
 *
 * @param d  dungeon struct to store all data relevant to dungeon
 */
static uint8_t generate_dungeon(dungeon_t *d)
{
  // Set seed for random numbers as the current time in milliseconds
  int seed = time(NULL);
  srand(seed);
  
  // Determine random room count for this dungeon
  uint8_t rand_num = rand_range(MIN_ROOM_COUNT, ROOM_COUNT_RANGE);


  // Allocate memory for the dungeon
  if(!(d->rooms = malloc(rand_num * sizeof(*d->rooms)))) {
    printf("FATAL: malloc() unable to allocate memory for rooms.\n");
    return 1;
  }

  
  // Assign number of rooms in dungeon
  d->num_rooms = rand_num;


  // Initialize dungeon arrays
  if(init_dungeon_arr(d)) {
    return 1;
  }


  // Initialize all of the rooms
  if(init_rooms(d)) {
    return 1;
  }


  // Tunnels corridors between rooms
  if(render_corridors(d)) {
    return 1;
  }

  
  // Place character
  d->pc.x_pos = d->rooms[0].x_pos;
  d->pc.y_pos = d->rooms[0].y_pos;
  d->dungeon[d->pc.y_pos][d->pc.x_pos] = PLAYER_CHAR;

  
  return 0;
}


/*
 * Function for creating the randomized rooms to be placed into the dungeon
 *
 * @param d  dungeon struct to store room data in
 */
static uint8_t init_rooms(dungeon_t *d)
{
  // Create a number of random rooms equivalent to room_count
  uint8_t valid_room_count = 0;
  uint8_t i, j;
  char invalid_room_flag;

  while (valid_room_count < d->num_rooms) {

    // Generate random values for room
    // Random xpos (in range 1 - (79 - min_x_size))
    uint8_t rand_xpos = rand_range(1, (DUNGEON_WIDTH - MIN_ROOM_X_SIZE - 2));
    
    // Random ypos (in range 1 - (20 - min_y_size))
    uint8_t rand_ypos = rand_range(1, (DUNGEON_HEIGHT - MIN_ROOM_Y_SIZE - 2));

    // Random x size
    uint8_t rand_xsize = rand_range(MIN_ROOM_X_SIZE, ROOM_SIZE_RANGE);

    // Random y size
    uint8_t rand_ysize = rand_range(MIN_ROOM_Y_SIZE, ROOM_SIZE_RANGE);


    // Validate random values against terminal border
    if ((rand_xpos + rand_xsize) > (DUNGEON_WIDTH - 1)) { // Check xpos out of bounds
      continue;
      
    } else if ((rand_ypos + rand_ysize) > (DUNGEON_HEIGHT - 1)) { // Check ypos out of bounds
      continue;
      
    }

    
    // Validate random values against other rooms (rooms cannot touch or be within one space of each other in any direction)
    invalid_room_flag = 0;

    for (i = 0; i < valid_room_count; i++) {
      
      if(!((rand_xpos > (d->rooms[i].x_pos + d->rooms[i].x_size + ROOM_PADDING)) ||   // New room not to right of valid room
	   ((rand_xpos + rand_xsize + ROOM_PADDING) < d->rooms[i].x_pos) ||          // Valid room not to right of new room
	   (rand_ypos > (d->rooms[i].y_pos + d->rooms[i].y_size + ROOM_PADDING)) ||   // New room not below valid room
	   ((rand_ypos + rand_ysize + ROOM_PADDING) < d->rooms[i].y_pos))) {         // Valid room not below new room 

	invalid_room_flag = 1;
	break;	
      }
      
    }
    
    // If the room was invalid then try again
    if (invalid_room_flag) {
      continue;
    }

    
    // Room is valid
    // Create new room
    room_t room_to_add = { rand_xpos, rand_ypos, rand_xsize, rand_ysize };

    // Add new room to array for tracking
    d->rooms[valid_room_count] = room_to_add;
    
    // Write new room to the dungeon
    for (i = room_to_add.y_pos; i < (room_to_add.y_pos + room_to_add.y_size); i++) {
      for (j = room_to_add.x_pos; j < (room_to_add.x_pos + room_to_add.x_size); j++) {

	// Write room character to selected square and give hardness value for room
	d->dungeon[i][j] = ROOM_CHAR;
	d->material_hardness[i][j] = ROOM_HARDNESS;

      }
    }

    // Increment number of rooms created
    valid_room_count++;
  }

  return 0;
}


/*
 * Creates corridors between rooms
 *
 * @param d  dungeon struct containing map to render corridors in
 */
static uint8_t render_corridors(dungeon_t *d)
{
  uint8_t i;

  for (i = 0; i < d->num_rooms; i++) {

    // Room to begin in
    room_t origin = d->rooms[i];

    // Room to end in
    room_t destination;

    // Check if it is necessary to wrap around to the beginning
    if ((i + 1) >= d->num_rooms) {

      destination = d->rooms[0];
      
    }else { // No wrapping necessary

      destination = d->rooms[i+1];
      
    }

    
    // Randomize origin door location
    uint8_t x0 = origin.x_pos + rand_range(0, origin.x_size);
    uint8_t y0 = origin.y_pos + rand_range(0, origin.y_size);

    // Randomize destination door location
    uint8_t x1 = destination.x_pos + rand_range(0,  destination.x_size);
    uint8_t y1 = destination.y_pos + rand_range(0,  destination.y_size);

    draw_simple_path(d, x0, y0, x1, y1);
  }

  return 0;
}


/*
 * Draws a simple path between the two locations
 *
 * @param d
 * @param x0
 * @param y0
 * @param x1
 * @param y1
 */
static uint8_t draw_simple_path(dungeon_t *d, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
  // Calculate the x incrementing
  int8_t x_increment = abs(x1 - x0)/(x1 - x0);
  uint8_t x;
  
  // Begin carving the corridor into the dungeon
  for (x = x0; x != x1; x += x_increment) {

    // TODO: Check if corridor should turn
    
    // If it is not a room draw a corridor symbol and give hardness value for corridor
    if (d->material_hardness[y0][x] != 0) {
      	
      d->dungeon[y0][x] = CORRIDOR_CHAR;
      d->material_hardness[y0][x] = CORRIDOR_HARDNESS;
	
    } 
  }

  // Calculate y incrementing
  int8_t y_increment = abs(y1 - y0)/(y1 - y0);
  uint8_t y;
  
  for (y = y0; y != y1; y += y_increment) {
    
    // If it is not a room draw a corridor symbol and give hardness value for corridor
    if (d->material_hardness[y][x1] != 0) {
      
      d->dungeon[y][x1] = CORRIDOR_CHAR;
      d->material_hardness[y][x1] = CORRIDOR_HARDNESS;
      
    }  
  }
  return 0;
}


/*
 * Function for initializing the dungeon
 * Sets all locations to rock intially and sets up status bar
 *
 * @param d  dungeon struct containing arrays to initialize
 */
static uint8_t init_dungeon_arr(dungeon_t *d)
{
  uint8_t i, j;

  // Populate dungeon area and hardness values
  for (i = 0; i < DUNGEON_HEIGHT; i++) {
    for (j = 0; j < DUNGEON_WIDTH; j++) {

      // Check if on the outermost walls of the dungeon
      if (i == 0 || i == (DUNGEON_HEIGHT - 1)) {

	// Top/bottom most walls therefore assign hardness value for the dungeon border
	d->dungeon[i][j] = '-';
	d->material_hardness[i][j] = DUNGEON_BORDER_HARDNESS;
	
      } else if (j == 0 || j == (DUNGEON_WIDTH - 1)) {

	// Left/right most walls therefore assign hardness value for dungeon border
	d->dungeon[i][j] = '|';
	d->material_hardness[i][j] = DUNGEON_BORDER_HARDNESS;

      } else {

	// Assign random integer between 1-254 inclusive to all other cells
	d->dungeon[i][j] = ROCK_CHAR;
	d->material_hardness[i][j] = (rand() % ROCK_HARDNESS_RANGE) + MIN_ROCK_HARDNESS; 

      }
    }
  }
  return 0;
}


/*
 * Function for loading a dungeon from disk
 *
* @param d  dungeon struct to load dungeon from disk in to
 */
uint8_t load_dungeon(dungeon_t *d)
{
  // Get path to file
  char *file_path;
  file_path = get_dungeon_file_path();

  // Open file for reading
  FILE *file;
  if (!(file = fopen(file_path, "r"))) {
    printf("FATAL: Failed to open %s in load_dungeon()\n", file_path);
    free (file_path);
    return 1;
  }
  free(file_path);

  
  // Read file type marker
  char *file_type_marker;
  if (!(file_type_marker = malloc(12 + 1))) {
    printf("FATAL: malloc() failed assigning space for file_type_marker\n");
    return 1;
  }
  fread(file_type_marker, 12, 1, file);
  free(file_type_marker);

  
  // Read file version
  uint32_t be_file_version;
  fread(&be_file_version, sizeof(uint32_t), 1, file);

  
  // Read file size
  uint32_t be_file_size;
  fread(&be_file_size, sizeof(uint32_t), 1, file);
  uint32_t file_size = be32toh(be_file_size);
  
  
  // Read Player Character's position
  fread(&d->pc.x_pos, 1, 1, file);
  fread(&d->pc.y_pos, 1 ,1, file);

  
  // Read hardness matrix
  read_hardness(d, file);


  // Calculate number of rooms from size of file   
  d->num_rooms = (file_size - 1702) / 4;
  
  // Allocate memory for room
  if(!(d->rooms = malloc(d->num_rooms * sizeof(*d->rooms)))) {
    printf("FATAL: malloc() unable to allocate memory for rooms.\n");
    return 1;
  }

  // Read in toom data
  read_rooms(d, file);


  // Close file
  fclose(file);
   
  // Place character
  d->dungeon[d->pc.y_pos][d->pc.x_pos] = PLAYER_CHAR;

  return 0;
}


/*
 * Function for reading in hardness matrix and populating location of
 * corridors in the dungeon
 *
 * @param d  dungeon struct to read hardness data in to
 * @param file  file to read hardness data from
 */
static uint8_t read_hardness(dungeon_t *d, FILE *file)
{
  uint8_t i, j;

  for (i = 0; i < DUNGEON_HEIGHT; i++) {
    for (j = 0; j < DUNGEON_WIDTH; j++) {
      // Read in current byte
      fread(&d->material_hardness[i][j], sizeof(d->material_hardness[i][j]), 1, file);

      // Check if the current square is a corridor/room
      if(d->material_hardness[i][j] == DUNGEON_BORDER_HARDNESS) {
	if (i == 0 || i == (DUNGEON_HEIGHT - 1)) {

	  // Top/bottom most walls therefore assign hardness value for the dungeon border
	  d->dungeon[i][j] = '-';

	} else if (j == 0 || j == (DUNGEON_WIDTH - 1)) {

	  // Left/right most walls therefore assign hardness value for dungeon border
	  d->dungeon[i][j] = '|';

	}
	
      } else if(d->material_hardness[i][j] == 0) {

	d->dungeon[i][j] = CORRIDOR_CHAR;
	
      } else {

	d->dungeon[i][j] = ROCK_CHAR;
	
      }
    }
  }  
  return 0;
}


/*
 * Function for reading in room data and populating rooms
 * in the dungeon
 *
 * @param d  dungeon struct to read room data in to
 * @param file  file to read room data from
 */
static uint8_t read_rooms(dungeon_t *d, FILE *file)
{
  uint8_t i, byte;
  uint8_t j, k;

  for (i = 0; i < d->num_rooms; i++) {
    // Read in current room data
    fread(&byte, 1, 1, file);
    d->rooms[i].x_pos = byte;
    fread(&byte, 1, 1, file);
    d->rooms[i].y_pos = byte;
    fread(&byte, 1, 1, file);
    d->rooms[i].x_size = byte;
    fread(&byte, 1, 1, file);
    d->rooms[i].y_size = byte;
    
    // Write room to the dungeon
    for (j = d->rooms[i].y_pos; j < (d->rooms[i].y_pos + d->rooms[i].y_size); j++) {
      for (k = d->rooms[i].x_pos; k < (d->rooms[i].x_pos + d->rooms[i].x_size); k++) {

	d->dungeon[j][k] = ROOM_CHAR;

      }
    }
  }
  return 0;
}


/*
 * Function for saving the dungeon to disk
 *
 * @param d  dungeon struct to save to disk
 */
uint8_t save_dungeon(dungeon_t *d)
{
  // Get path to file
  char *file_path;
  file_path = get_dungeon_file_path();

  // Open file for writing
  FILE *file;
  if (!(file = fopen(file_path, "w"))) {
    free (file_path);
    printf("FATAL: Failed to open %s in save_dungeon()\n", file_path);
    return 1;
  }

  // Free memory allocated to file_path
  free(file_path);

  
  // Write file type marker
  char file_type_marker[] = "RLG327-F2018";
  fwrite(file_type_marker, sizeof(file_type_marker) - 1, 1, file);

  
  // Write file version to file in Big Endian byte ordering
  uint32_t file_version = 0;
  uint32_t be_file_version = htobe32(file_version);
  fwrite(&be_file_version, sizeof(uint32_t), 1, file);

  
  // Write file size to file in Big Endian byte ordering
  uint32_t file_size = 1702 + (d->num_rooms * 4);
  uint32_t be_file_size = htobe32(file_size);  
  fwrite(&be_file_size, sizeof(uint32_t), 1, file);


  // Write player character position
  fwrite(&d->pc.x_pos, 1, 1, file);
  fwrite(&d->pc.y_pos, 1, 1, file);
  
  // Write hardness matrix
  write_hardness(d, file);
  
  // Write room data
  write_rooms(d, file);
	 
  
  // Close file
  fclose(file);

  return 0;
}


/*
 * Function for writing the hardness matrix to disk
 *
 * @param d  dungeon struct containing hardness data
 * @param file  file to write hardness data to
 */
static uint8_t write_hardness(dungeon_t *d, FILE *file)
{
  uint8_t i, j;

  for (i = 0; i < DUNGEON_HEIGHT; i++) {
    for (j = 0; j < DUNGEON_WIDTH; j++) {
      fwrite(&d->material_hardness[i][j], sizeof(d->material_hardness[i][j]), 1, file);
    }
  }
  return 0;
}


/*
 * Function for writing room data to disk
 *
 * @param d  dungeon struct containing room data
 * @param file  file to write room data to
 */
static uint8_t write_rooms(dungeon_t *d, FILE *file)
{
  uint8_t i;

  for (i = 0; i < d->num_rooms; i++) {
    fwrite(&d->rooms[i].x_pos, 1, 1, file);
    fwrite(&d->rooms[i].y_pos, 1, 1, file);
    fwrite(&d->rooms[i].x_size, 1, 1, file);
    fwrite(&d->rooms[i].y_size, 1, 1, file);
  }

  return 0;
}


/*
 * Takes in pointer and assigns it the path to the dungeon file
 *
 * @return  Address to location dungeon file path is stored
 */
static char * get_dungeon_file_path(void)
{
  // Path to dungeon from home
  char path_dungeon[] = "/.rlg327/dungeon";

  // Allocate memory for combined path
  char *path_buffer;
  if (!(path_buffer = malloc(strlen(getenv("HOME")) + strlen(path_dungeon) + 1))) {
    printf("malloc() failed in get_dungeon_file_path()");
    exit(-1);
  }

  // Combine the paths
  strcpy(path_buffer, getenv("HOME"));
  strcat(path_buffer, path_dungeon);

  return path_buffer;
}


/*
 * Function to free all memory being used by a dungeon
 *
 * @param d  dungeon struct to deallocate from memory
 */
void del_dungeon(dungeon_t *d)
{
  if(d != NULL) {
    free (d->rooms);
  }
}

  
/*
 * Function for showing the dungeon in the console
 *
 * @param d  dungeon struct with dungeon map for displaying
 */
void show_dungeon(dungeon_t *d)
{
  uint8_t i, j;

  for (i = 0; i < DUNGEON_HEIGHT; i++) {
    for (j = 0; j < DUNGEON_WIDTH; j++) {

      putchar(d->dungeon[i][j]);

    }
    putchar('\n');
  }
}


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
