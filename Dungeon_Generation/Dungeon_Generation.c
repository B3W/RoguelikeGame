/*
 * Source for creating the dungeon mapping in the Rougelike game.
 * Dungeon fits inside an 80 wide x 21 tall terminal and features
 * rock with rooms set inside it and corridors linking the rooms.
 *
 *   Author: Weston Berg (weberg@iastate.edu)
 *   Date: 09/01/2018
 */

#include <endian.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "Dungeon_Generation.h"


// Graphics Rendering User Changeable Settings
#define ROCK_CHAR ' '
#define ROOM_CHAR '.'
#define CORRIDOR_CHAR '#'
#define PLAYER_CHAR '@'

// Graphics Rendering Non-Changeable Settings
#define MIN_ROOM_COUNT 5
#define MAX_ROOM_COUNT 8
#define MIN_ROOM_X_SIZE 3
#define MIN_ROOM_Y_SIZE 2
#define ROOM_SIZE_RANGE 5
#define ROOM_PADDING 1

// Hardness values macros
#define DUNGEON_BORDER_HARDNESS 255
#define MIN_ROCK_HARDNESS 1
#define ROCK_HARDNESS_RANGE 254
#define CORRIDOR_HARDNESS 0
#define ROOM_HARDNESS 0


// Function prototypes
static uint8_t generate_dungeon(dungeon_t *d);
static uint8_t init_dungeon_arr(dungeon_t *d);
static uint8_t init_rooms(dungeon_t *d);
static uint8_t render_corridors(dungeon_t *d);
static uint8_t read_hardness(dungeon_t *, FILE *);
static uint8_t read_rooms(dungeon_t *, FILE *);
static uint8_t write_hardness(dungeon_t *d, FILE *file);
static uint8_t write_rooms(dungeon_t *d, FILE *file);
static char * get_dungeon_file_path(void);
  

/*
 * Function for initializing a new dungeon.
 *
 * @param dungeon  2D array for representing the entire dungeon with space for status updates
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
 * @param
 */
static uint8_t generate_dungeon(dungeon_t *d)
{
  // Set seed for random numbers as the current time in milliseconds
  int seed = time(NULL);
  srand(seed);
  
  // Determine random room count for this dungeon
  uint8_t rand_num = (rand() % (MAX_ROOM_COUNT - MIN_ROOM_COUNT) + MIN_ROOM_COUNT);


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
  d->player_character.x_pos = d->rooms[0].x_pos;
  d->player_character.y_pos = d->rooms[0].y_pos;
  d->dungeon[d->player_character.y_pos][d->player_character.x_pos] = PLAYER_CHAR;

  
  return 0;
}


/*
 * Function for creating the randomized rooms to be placed into the dungeon
 *
 * @param room_count  number of rooms to create
 * @param p_rooms  pointer to array storing room information
 */
static uint8_t init_rooms(dungeon_t *d)
{
  // Create a number of random rooms equivalent to room_count
  uint8_t valid_room_count = 0;
  uint8_t i, j;
  char invalid_room_flag;

  while (valid_room_count < d->num_rooms) {

    // Generate random values for room
    // Random xpos (in range 1 - 79)
    uint8_t rand_xpos = (rand() % (TERMINAL_WIDTH - 2)) + 1;
    
    // Randome ypos (in range 1 - 20)
    uint8_t rand_ypos = (rand() % (DUNGEON_HEIGHT - 2)) + 1;

    // Random x size
    uint8_t rand_xsize = (rand() % ROOM_SIZE_RANGE) + MIN_ROOM_X_SIZE;

    // Random y size
    uint8_t rand_ysize = (rand() % ROOM_SIZE_RANGE) + MIN_ROOM_Y_SIZE;


    // Validate random values against terminal border
    if ((rand_xpos + rand_xsize) > (TERMINAL_WIDTH - 1)) { // Check xpos out of bounds
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
 * @param room_count  Number of rooms in the dungeon
 * @param p_rooms  pointer to array containing all rooms in the dungeon
 * @param dungeon  2D array representing the dungeon which corridors on being rendered in
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
    uint8_t x0 = origin.x_pos + (rand() % origin.x_size);
    uint8_t y0 = origin.y_pos + (rand() % origin.y_size);

    // Randomize destination door location
    uint8_t x1 = destination.x_pos + (rand() % destination.x_size);
    uint8_t y1 = destination.y_pos + (rand() % destination.y_size);

    // Calculate the incrementing
    int8_t x_increment = abs(x1 - x0)/(x1 - x0);
    int8_t y_increment = abs(y1 - y0)/(y1 - y0);

    
    // Begin carving the corridor into the dungeon
    uint8_t x, y;

    for (x = x0; x != x1; x += x_increment) {

      // If it is not a room draw a corridor symbol and give hardness value for corridor
      if (d->dungeon[y0][x] != ROOM_CHAR) {
	
	d->dungeon[y0][x] = CORRIDOR_CHAR;
	d->material_hardness[y0][x] = CORRIDOR_HARDNESS;
	
      }
      
    }

    for (y = y0; y != y1; y += y_increment) {

      // If it is not a room draw a corridor symbol and give hardness value for corridor
      if (d->dungeon[y][x1] != ROOM_CHAR) {
	
	d->dungeon[y][x1] = CORRIDOR_CHAR;
	d->material_hardness[y][x1] = CORRIDOR_HARDNESS;
	
      }
      
    }
  }

  return 0;
}


/*
 * Function for initializing the dungeon
 * Sets all locations to rock intially and sets up status bar
 *
 * @param dungeon  2D array representing the dungeon
 */
static uint8_t init_dungeon_arr(dungeon_t *d)
{
  uint8_t i, j;

  // Populate dungeon area and hardness values
  for (i = 0; i < DUNGEON_HEIGHT; i++) {
    for (j = 0; j < TERMINAL_WIDTH; j++) {

      // Check if on the outermost walls of the dungeon
      if (i == 0 || i == (DUNGEON_HEIGHT - 1)) {

	// Top/bottom most walls therefore assign hardness value for the dungeon border
	d->dungeon[i][j] = '-';
	d->material_hardness[i][j] = DUNGEON_BORDER_HARDNESS;
	
      } else if (j == 0 || j == (TERMINAL_WIDTH - 1)) {

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

  // Populate status bar
  uint8_t index;
  for (i = DUNGEON_HEIGHT; i < TERMINAL_HEIGHT; i++) {

    index = 0;
    d->dungeon[i][index] = 'S';
    index++;
    d->dungeon[i][index] = 'T';
    index++;
    d->dungeon[i][index] = 'A';
    index++;
    d->dungeon[i][index] = 'T';
    index++;
    d->dungeon[i][index] = 'U';
    index++;
    d->dungeon[i][index] = 'S';
    index++;
    
    // Fill rest with white space
    for (j = index; j < TERMINAL_WIDTH; j++) {

      d->dungeon[i][j] = ' ';

    }
  }

  return 0;
}


/*
 * Function for loading a dungeon from disk
 *
 * @param
 * @param
 * @param
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
  fread(&d->player_character.x_pos, 1, 1, file);
  fread(&d->player_character.y_pos, 1 ,1, file);

  
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
  d->dungeon[d->player_character.y_pos][d->player_character.x_pos] = PLAYER_CHAR;

  return 0;
}


/*
 * Function for reading in hardness matrix and populating location of
 * corridors in the dungeon
 */
static uint8_t read_hardness(dungeon_t *d, FILE *file)
{
  uint8_t i, j;

  for (i = 0; i < DUNGEON_HEIGHT; i++) {
    for (j = 0; j < TERMINAL_WIDTH; j++) {
      // Read in current byte
      fread(&d->material_hardness[i][j], sizeof(d->material_hardness[i][j]), 1, file);

      // Check if the current square is a corridor/room
      if(d->material_hardness[i][j] == DUNGEON_BORDER_HARDNESS) {
	if (i == 0 || i == (DUNGEON_HEIGHT - 1)) {

	  // Top/bottom most walls therefore assign hardness value for the dungeon border
	  d->dungeon[i][j] = '-';

	} else if (j == 0 || j == (TERMINAL_WIDTH - 1)) {

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

  // Populate status bar
  uint8_t index;
  for (i = DUNGEON_HEIGHT; i < TERMINAL_HEIGHT; i++) {

    index = 0;
    d->dungeon[i][index] = 'S';
    index++;
    d->dungeon[i][index] = 'T';
    index++;
    d->dungeon[i][index] = 'A';
    index++;
    d->dungeon[i][index] = 'T';
    index++;
    d->dungeon[i][index] = 'U';
    index++;
    d->dungeon[i][index] = 'S';
    index++;

    // Fill rest with white space
    for (j = index; j < TERMINAL_WIDTH; j++) {

      d->dungeon[i][j] = ' ';

    }
  }  
  return 0;
}


/*
 * Function for reading in room data and populating rooms
 * in the dungeon
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
 * @param
 * @param
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
  fwrite(&d->player_character.x_pos, 1, 1, file);
  fwrite(&d->player_character.y_pos, 1, 1, file);
  
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
 */
static uint8_t write_hardness(dungeon_t *d, FILE *file)
{
  uint8_t i, j;

  for (i = 0; i < DUNGEON_HEIGHT; i++) {
    for (j = 0; j < TERMINAL_WIDTH; j++) {
      fwrite(&d->material_hardness[i][j], sizeof(d->material_hardness[i][j]), 1, file);
    }
  }
  return 0;
}


/*
 * Function for writing room data to disk
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
 * Function to free all memory being used by a dungeon
 */
void del_dungeon(dungeon_t *d)
{
  if(d != NULL) {
    free (d->rooms);
  }
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
 * Function for showing the dungeon in the console
 *
 * @param dungeon  2D array representing the dungeon
 */
void show_dungeon(dungeon_t *d)
{
  uint8_t i, j;

  for (i = 0; i < TERMINAL_HEIGHT; i++) {
    for (j = 0; j < TERMINAL_WIDTH; j++) {

      printf("%c", d->dungeon[i][j]);

    }
    printf("\n");
  }
}
