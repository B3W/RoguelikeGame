/*
 * Source for creating the dungeon mapping in the Rougelike game.
 * Dungeon fits inside an 80 wide x 21 tall terminal and features
 * rock with rooms set inside it and corridors linking the rooms.
 *
 *   Author: Weston Berg (weberg@iastate.edu)
 *   Date: 09/01/2018
 */

#include <endian.h>
#include <stdint.h>
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

// Hardness values macros
#define DUNGEON_BORDER_HARDNESS 255
#define MIN_ROCK_HARDNESS 1
#define ROCK_HARDNESS_RANGE 254
#define CORRIDOR_HARDNESS 0
#define ROOM_HARDNESS 0


// Data structure for representing a room in the dungeon
// Use char instead of int to save on memory size
typedef struct room {
  uint8_t x_pos;
  uint8_t y_pos;
  uint8_t x_size;
  uint8_t y_size;
} room_t;

// Data structure for representing the player character on the dungeon map
typedef struct pc {
  uint8_t x_pos;
  uint8_t y_pos;
} pc_t;

// Global variable for keeping track of how many rooms there are in the dungeon
char num_rooms;
// Global variable for keeping track of the player character
pc_t player_character;


/*
 * Entry point for running dungeon creation code
 * Not yet setup for handling command line input
 *
 * @param argc
 * @param argv
 */
int main(int argc, char *argv[])
{
  // 2D array for representing the entire dungeon with space for status updates
  char dungeon[TERMINAL_HEIGHT][TERMINAL_WIDTH];

  // 2D array representing hardness of each square in the dungeon
  unsigned char material_hardness[DUNGEON_HEIGHT][TERMINAL_WIDTH];

  // Check for command line arguments
  char load_flag = 0;
  char save_flag = 0;

  if (argc > 1) {

    int i;
    for (i = 1; i < argc; i++) {
      // Check which switches were included
      if (argv[i][1] == '-') {
	if (strcmp(argv[i], "--load") == 0) {
	  load_flag = 1;

	} else if (strcmp(argv[i], "--save") == 0) {
	  save_flag = 1;

	}
      } else {
	printf ("%s contains invalid format.\nMake sure switches are preceded by \'--\'.\nDungeon generation  exiting...\n", argv[i]);
	exit(-1);

      }
    }
  }
  
  // Initialize a new dungeon and show in terminal
  init_dungeon(dungeon, material_hardness, load_flag, save_flag);
  
  return 0;

}


/*
 * Function for initializing a new dungeon.
 *
 * @param dungeon  2D array for representing the entire dungeon with space for status updates
 */
void init_dungeon(char dungeon[TERMINAL_HEIGHT][TERMINAL_WIDTH], unsigned char material_hardness[DUNGEON_HEIGHT][TERMINAL_WIDTH], char load_flag, char save_flag)
{
  // Declare array for keeping track of rooms within the dungeon
  room_t *rooms;

  
  // Determine wheter to load in a dungeon or create a new one
  if (load_flag) {

    // Check if dungeon file exists, if not generate a new one
    char *file_path = get_dungeon_file_path();
    FILE *file;
    if ((file = fopen(file_path, "r"))) {

      // Dungeon file exists so load existing
      fclose(file);
      rooms = load_dungeon(dungeon, material_hardness);

    } else {

      // Dungeon file doesn't exist so create new dungeon
      rooms = generate_dungeon(dungeon, material_hardness);

    }
    
  } else {

    rooms = generate_dungeon(dungeon, material_hardness);
    
  }

  
  // Display the dungeon
  show_dungeon(dungeon);

  
  // Determine whether to save the dungeon back to disk or not
  if (save_flag) {

    save_dungeon(material_hardness, rooms);

  }

  
  // Free memory allocated for rooms array
  free(rooms);
}


/*
 * Function for generating a new dungeon
 *
 * @param
 * @param
 * @param
 */
room_t * generate_dungeon(char dungeon[TERMINAL_HEIGHT][TERMINAL_WIDTH], unsigned char material_hardness[DUNGEON_HEIGHT][TERMINAL_WIDTH])
{
  // Initialize dungeon array
  init_dungeon_arr(dungeon, material_hardness);

  
  // Set seed for random numbers as the current time in milliseconds
  int seed = time(NULL);
  srand(seed);
  
  // Determine random room count for this dungeon
  num_rooms = (rand() % (MAX_ROOM_COUNT - MIN_ROOM_COUNT) + MIN_ROOM_COUNT);

  
  // Declare memory for room array
  room_t *rooms;
  if (!(rooms = malloc(num_rooms * sizeof(*rooms)))) {
    printf("malloc() failed\n");
    exit(-1);
  }

  
  // Initialize all of the rooms
  init_rooms(num_rooms, rooms, dungeon, material_hardness);

  
  // Tunnels corridors between rooms
  render_corridors(num_rooms, rooms, dungeon, material_hardness);

  
  // Place character
  player_character.x_pos = rooms[0].x_pos;
  player_character.y_pos = rooms[0].y_pos;
  dungeon[player_character.y_pos][player_character.x_pos] = PLAYER_CHAR;

  
  return rooms;
}


/*
 * Function for creating the randomized rooms to be placed into the dungeon
 *
 * @param room_count  number of rooms to create
 * @param p_rooms  pointer to array storing room information
 */
void init_rooms(char req_room_count, room_t *p_rooms, char dungeon[TERMINAL_HEIGHT][TERMINAL_WIDTH], unsigned char material_hardness[DUNGEON_HEIGHT][TERMINAL_WIDTH])
{
  // Create a number of random rooms equivalent to room_count
  unsigned char valid_room_count = 0;
  while (valid_room_count < req_room_count) {

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
    int i, j;
    char break_flag = 0;
    
    for (i = rand_ypos; i < (rand_ypos + rand_ysize); i++) {
      for (j = rand_xpos; j < (rand_xpos + rand_xsize); j++) {

	if (dungeon[i-1][j-1] == ROOM_CHAR) { // Upper Left
	  break_flag = 1;
	}else if (dungeon[i-1][j] == ROOM_CHAR) { // Up
	  break_flag = 1;
	}else if (dungeon[i-1][j+1] == ROOM_CHAR) { // Up Right
	  break_flag = 1;
	}else if (dungeon[i][j+1] == ROOM_CHAR) { // Right
	  break_flag = 1;
	}else if (dungeon[i+1][j+1] == ROOM_CHAR) { // Down Right
	  break_flag = 1;
	}else if (dungeon[i+1][j] == ROOM_CHAR) { // Down
	  break_flag = 1;
	}else if (dungeon[i+1][j-1] == ROOM_CHAR) { // Down Left
	  break_flag = 1;
	}else if (dungeon[i][j-1] == ROOM_CHAR) { // Left
	  break_flag = 1;
	}

	// Exit inner loop if invalid
	// *** Find a better way to do this ***
	if (break_flag) {
	  break;
	}
	
      }

      // Break out of outer loop if invalid
      if (break_flag) {
	break;
      }
      
    }
  
    // If the room was invalid then try again
    if (break_flag) {
      continue;
    }


    // Room is valid
    // Create new room
    room_t room_to_add = { rand_xpos, rand_ypos, rand_xsize, rand_ysize };

    // Add new room to array for tracking
    p_rooms[valid_room_count] = room_to_add;

    // Write new room to the dungeon
    render_room(&room_to_add, dungeon, material_hardness);
    

    // Increment number of rooms created
    valid_room_count++;
  }  
}


/*
 * Renders the room onto the dungeon map
 *
 * @param room_inst  pointer to instance of room struct which holds information on room being rendered
 * @param dungeon  2D array representation of dungeon which room is being rendered in
 */
void render_room(room_t *room_inst, char dungeon[TERMINAL_HEIGHT][TERMINAL_WIDTH], unsigned char material_hardness[DUNGEON_HEIGHT][TERMINAL_WIDTH])
{
  uint8_t i, j;

  for (i = room_inst->y_pos; i < (room_inst->y_pos + room_inst->y_size); i++) {
    for (j = room_inst->x_pos; j < (room_inst->x_pos + room_inst->x_size); j++) {

      // Write room character to selected square and give hardness value for room
      dungeon[i][j] = ROOM_CHAR;
      material_hardness[i][j] = ROOM_HARDNESS;
      
    }
  }
}


/*
 * Creates corridors between rooms
 *
 * @param room_count  Number of rooms in the dungeon
 * @param p_rooms  pointer to array containing all rooms in the dungeon
 * @param dungeon  2D array representing the dungeon which corridors on being rendered in
 */
void render_corridors(char room_count, room_t *p_rooms, char dungeon[TERMINAL_HEIGHT][TERMINAL_WIDTH], unsigned char material_hardness[DUNGEON_HEIGHT][TERMINAL_WIDTH])
{
  int i;

  for (i = 0; i < room_count; i++) {

    // Room to begin in
    room_t origin = p_rooms[i];

    // Room to end in
    room_t destination;
    if ((i + 1) >= room_count) { // Check if it is necessary to wrap around to the beginning
      destination = p_rooms[0];
      
    }else { // No wrapping necessary
      destination = p_rooms[i+1];
      
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
      if (dungeon[y0][x] != ROOM_CHAR) {
	
	dungeon[y0][x] = CORRIDOR_CHAR;
	material_hardness[y0][x] = CORRIDOR_HARDNESS;
	
      }
      
    }

    for (y = y0; y != y1; y += y_increment) {

      // If it is not a room draw a corridor symbol and give hardness value for corridor
      if (dungeon[y][x1] != ROOM_CHAR) {
	
	dungeon[y][x1] = CORRIDOR_CHAR;
	material_hardness[y][x1] = CORRIDOR_HARDNESS;
	
      }
      
    }
  }
}


/*
 * Function for initializing the dungeon
 * Sets all locations to rock intially and sets up status bar
 *
 * @param dungeon  2D array representing the dungeon
 */
void init_dungeon_arr(char dungeon[TERMINAL_HEIGHT][TERMINAL_WIDTH], unsigned char material_hardness[DUNGEON_HEIGHT][TERMINAL_WIDTH])
{
  uint8_t i, j;

  // Populate dungeon area and hardness values
  for (i = 0; i < DUNGEON_HEIGHT; i++) {
    for (j = 0; j < TERMINAL_WIDTH; j++) {

      // Check if on the outermost walls of the dungeon
      if (i == 0 || i == (DUNGEON_HEIGHT - 1)) {

	// Top/bottom most walls therefore assign hardness value for the dungeon border
	dungeon[i][j] = '-';
	material_hardness[i][j] = DUNGEON_BORDER_HARDNESS;
	
      } else if (j == 0 || j == (TERMINAL_WIDTH - 1)) {

	// Left/right most walls therefore assign hardness value for dungeon border
	dungeon[i][j] = '|';
	material_hardness[i][j] = DUNGEON_BORDER_HARDNESS;

      } else {

	// Assign random integer between 1-254 inclusive to all other cells
	dungeon[i][j] = ROCK_CHAR;
	material_hardness[i][j] = (rand() % ROCK_HARDNESS_RANGE) + MIN_ROCK_HARDNESS; 

      }
    }
  }

  // Populate status bar
  uint8_t index;
  for (i = DUNGEON_HEIGHT; i < TERMINAL_HEIGHT; i++) {

    index = 0;
    dungeon[i][index] = 'S';
    index++;
    dungeon[i][index] = 'T';
    index++;
    dungeon[i][index] = 'A';
    index++;
    dungeon[i][index] = 'T';
    index++;
    dungeon[i][index] = 'U';
    index++;
    dungeon[i][index] = 'S';
    index++;
    
    // Fill rest with white space
    for (j = index; j < TERMINAL_WIDTH; j++) {

      dungeon[i][j] = ' ';

    }
  }
}


/*
 * Function for loading a dungeon from disk
 *
 * @param
 * @param
 * @param
 */
room_t * load_dungeon(char dungeon[TERMINAL_HEIGHT][TERMINAL_WIDTH], unsigned char material_hardness[DUNGEON_HEIGHT][TERMINAL_WIDTH])
{
  // Get path to file
  char *file_path;
  file_path = get_dungeon_file_path();

  // Open file for reading
  FILE *file;
  if (!(file = fopen(file_path, "r"))) {
    printf("Failed to open %s in save_dungeon()", file_path);
    exit(-1);
  }

  // Free memory allocated to file_path
  free(file_path);

  
  // Read file type marker
  char *file_type_marker;
  if (!(file_type_marker = malloc(12 + 1))) {
    printf("malloc() failed assigning space for file_type_marker");
    exit(-1);
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
  fread(&player_character, sizeof(pc_t), 1, file);

  
  // Read hardness matrix
  fread(material_hardness, sizeof(unsigned char[DUNGEON_HEIGHT][TERMINAL_WIDTH]), 1, file);
  
  
  // Read in room data
  // Calculate number of rooms from size of file
  num_rooms = (file_size - 1702) / 4;
  // Allocate appropriate amount of memory
  room_t *room_ptr;
  if (!(room_ptr  = malloc(num_rooms * sizeof(*room_ptr)))) {
    printf ("malloc() error in load_dungeon()");
    exit(-1);
  }
  fread(room_ptr, sizeof(*room_ptr), num_rooms, file);


  // Configure dungeon from hardness matrix
  uint8_t i, j;
  for (i = 0; i < DUNGEON_HEIGHT; i++) {
    for (j = 0; j < TERMINAL_WIDTH; j++) {

      // Check if on the outermost walls of the dungeon
      if (i == 0 || i == (DUNGEON_HEIGHT - 1)) {

        // Top/bottom most walls therefore assign hardness value for the dungeon border
        dungeon[i][j] = '-';

      } else if (j == 0 || j == (TERMINAL_WIDTH - 1)) {

        // Left/right most walls therefore assign hardness value for dungeon border
        dungeon[i][j] = '|';

      } else {

	// Check if corridor or room
	if (material_hardness[i][j] == 0) {
	  dungeon[i][j] = CORRIDOR_CHAR;

	} else {
	  dungeon[i][j] = ROCK_CHAR;

	}
      }
    }
  }

  // Fill in rooms
  uint8_t k;
  for (i = 0; i < num_rooms; i++) {
    room_t temp_room = room_ptr[i];

    for (j = temp_room.y_pos; j < (temp_room.y_pos + temp_room.y_size); j++) {
      for (k = temp_room.x_pos; k < (temp_room.x_pos + temp_room.x_size); k++) {
	dungeon[j][k] = ROOM_CHAR;

      }
    }
  }

  // Populate status bar
  uint8_t index;
  for (i = DUNGEON_HEIGHT; i < TERMINAL_HEIGHT; i++) {

    index = 0;
    dungeon[i][index] = 'S';
    index++;
    dungeon[i][index] = 'T';
    index++;
    dungeon[i][index] = 'A';
    index++;
    dungeon[i][index] = 'T';
    index++;
    dungeon[i][index] = 'U';
    index++;
    dungeon[i][index] = 'S';
    index++;

    // Fill rest with white space
    for (j = index; j < TERMINAL_WIDTH; j++) {

      dungeon[i][j] = ' ';

    }
  }

  // Place character
  dungeon[player_character.y_pos][player_character.x_pos] = PLAYER_CHAR;
  

  return room_ptr;
}


/*
 * Function for saving the dungeon to disk
 *
 * @param
 * @param
 */
void save_dungeon(unsigned char material_hardness[DUNGEON_HEIGHT][TERMINAL_WIDTH], room_t *p_rooms)
{
  // Get path to file
  char *file_path;
  file_path = get_dungeon_file_path();

  // Open file for writing
  FILE *file;
  if (!(file = fopen(file_path, "w"))) {
    printf("Failed to open %s in save_dungeon()", file_path);
    exit(-1);
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
  uint32_t file_size = 1702 + (num_rooms * 4);
  uint32_t be_file_size = htobe32(file_size);  
  fwrite(&be_file_size, sizeof(uint32_t), 1, file);


  // Write player character position
  fwrite(&player_character, sizeof(pc_t), 1, file);
  
  
  // Write hardness matrix
  fwrite(material_hardness, sizeof(unsigned char[DUNGEON_HEIGHT][TERMINAL_WIDTH]), 1, file);
  
  // TODO: Write room data
  fwrite(p_rooms, sizeof(*p_rooms), num_rooms, file);
	 
  
  // Close file
  fclose(file);
}


/*
 * Takes in pointer and assigns it the path to the dungeon file
 *
 * @return  Address to location dungeon file path is stored
 */
char * get_dungeon_file_path(void)
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
void show_dungeon(char dungeon[TERMINAL_HEIGHT][TERMINAL_WIDTH])
{
  uint8_t i, j;

  for (i = 0; i < TERMINAL_HEIGHT; i++) {
    for (j = 0; j < TERMINAL_WIDTH; j++) {

      printf("%c", dungeon[i][j]);

    }
    printf("\n");
  }
}
