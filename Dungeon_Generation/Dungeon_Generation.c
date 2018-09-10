/*
 * Source for creating the dungeon mapping in the Rougelike game.
 * Dungeon fits inside an 80 wide x 21 tall terminal and features
 * rock with rooms set inside it and corridors linking the rooms.
 *
 *   Author: Weston Berg (weberg@iastate.edu)
 *   Date: 09/01/2018
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "Dungeon_Generation.h"


// Graphics Rendering User Changeable Settings
#define ROCK_CHAR ' '
#define ROOM_CHAR '.'
#define CORRIDOR_CHAR '#'

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
struct room {
  unsigned char x_pos;
  unsigned char y_pos;
  unsigned char x_size;
  unsigned char y_size;
};


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
  
  // Initialize a new dungeon and show in terminal
  init_dungeon(dungeon, material_hardness);
  
  return 0;

}


/*
 * Function for initializing a new dungeon.
 *
 * @param dungeon  2D array for representing the entire dungeon with space for status updates
 */
void init_dungeon(char dungeon[TERMINAL_HEIGHT][TERMINAL_WIDTH], unsigned char material_hardness[DUNGEON_HEIGHT][TERMINAL_WIDTH])
{
  // Initialize dungeon array
  init_dungeon_arr(dungeon, material_hardness);

  // Set seed for random numbers as the current time in milliseconds
  int seed = time(NULL);
  srand(seed);

  // Determine random room count for this dungeon
  char num_rooms = (rand() % (MAX_ROOM_COUNT - MIN_ROOM_COUNT) + MIN_ROOM_COUNT);

  // Declare array for keeping track of rooms within the dungeon
  struct room rooms[num_rooms];

  // Initialize all of the rooms
  init_rooms(num_rooms, rooms, dungeon, material_hardness);

  // Tunnels corridors between rooms
  render_corridors(num_rooms, rooms, dungeon, material_hardness);


  // Display the dungeon
  show_dungeon(dungeon);

}


/*
 * Function for creating the randomized rooms to be placed into the dungeon
 *
 * @param room_count  number of rooms to create
 * @param p_rooms  pointer to array storing room information
 */
void init_rooms(char req_room_count, struct room *p_rooms, char dungeon[TERMINAL_HEIGHT][TERMINAL_WIDTH], unsigned char material_hardness[DUNGEON_HEIGHT][TERMINAL_WIDTH])
{
  // Create a number of random rooms equivalent to room_count
  unsigned char valid_room_count = 0;
  while (valid_room_count < req_room_count) {

    // Generate random values for room
    // Random xpos (in range 1 - 79)
    char rand_xpos = (rand() % (TERMINAL_WIDTH - 2)) + 1;
    
    // Randome ypos (in range 1 - 20)
    char rand_ypos = (rand() % (DUNGEON_HEIGHT - 2)) + 1;

    // Random x size
    char rand_xsize = (rand() % ROOM_SIZE_RANGE) + MIN_ROOM_X_SIZE;

    // Random y size
    char rand_ysize = (rand() % ROOM_SIZE_RANGE) + MIN_ROOM_Y_SIZE;


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
    struct room room_to_add;
    init_room(&room_to_add, rand_xpos, rand_ypos, rand_xsize, rand_ysize);

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
void render_room(struct room *room_inst, char dungeon[TERMINAL_HEIGHT][TERMINAL_WIDTH], unsigned char material_hardness[DUNGEON_HEIGHT][TERMINAL_WIDTH])
{
  int i, j;

  for (i = room_inst->y_pos; i < (room_inst->y_pos + room_inst->y_size); i++) {
    for (j = room_inst->x_pos; j < (room_inst->x_pos + room_inst->x_size); j++) {

      // Write room character to selected square and give hardness value for room
      dungeon[i][j] = ROOM_CHAR;
      material_hardness[i][j] = ROOM_HARDNESS;
      
    }
  }
}


/*
 * Initializes room struct with given parameters as field values
 *
 * @param room_inst  pointer to instance of room struct which is being initialized
 * @param x_pos  X position for upper left corner of the room
 * @param y_pos  Y position for upper left corner of the room
 * @param x_size  Room's size in the X direction 
 * @param y_size  Room's size in the Y direction
 */
void init_room(struct room *room_inst, char x_pos, char y_pos, char x_size, char y_size)
{  
    room_inst->x_pos = x_pos;
    room_inst->y_pos = y_pos;
    room_inst->x_size = x_size;
    room_inst->y_size = y_size;
    
}


/*
 * Creates corridors between rooms
 *
 * @param room_count  Number of rooms in the dungeon
 * @param p_rooms  pointer to array containing all rooms in the dungeon
 * @param dungeon  2D array representing the dungeon which corridors on being rendered in
 */
void render_corridors(char room_count, struct room *p_rooms, char dungeon[TERMINAL_HEIGHT][TERMINAL_WIDTH], unsigned char material_hardness[DUNGEON_HEIGHT][TERMINAL_WIDTH])
{
  int i;

  for (i = 0; i < room_count; i++) {

    // Room to begin in
    struct room origin = p_rooms[i];

    // Room to end in
    struct room destination;
    if ((i + 1) >= room_count) { // Check if it is necessary to wrap around to the beginning
      destination = p_rooms[0];
      
    }else { // No wrapping necessary
      destination = p_rooms[i+1];
      
    }

    // Randomize origin door location
    unsigned char x0 = origin.x_pos + (rand() % origin.x_size);
    unsigned char y0 = origin.y_pos + (rand() % origin.y_size);

    // Randomize destination door location
    unsigned char x1 = destination.x_pos + (rand() % destination.x_size);
    unsigned char y1 = destination.y_pos + (rand() % destination.y_size);

    // Calculate the incrementing
    int x_increment = abs(x1 - x0)/(x1 - x0);
    int y_increment = abs(y1 - y0)/(y1 - y0);

    // Begin carving the corridor into the dungeon
    int x, y;

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
  int i, j;

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
  for (i = DUNGEON_HEIGHT; i < TERMINAL_HEIGHT; i++) {

    int index = 0;
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
 * Function for showing the dungeon in the console
 *
 * @param dungeon  2D array representing the dungeon
 */
void show_dungeon(char dungeon[TERMINAL_HEIGHT][TERMINAL_WIDTH])
{
  int i, j;

  for (i = 0; i < TERMINAL_HEIGHT; i++) {
    for (j = 0; j < TERMINAL_WIDTH; j++) {

      printf("%c", dungeon[i][j]);

    }
    printf("\n");
  }
}
