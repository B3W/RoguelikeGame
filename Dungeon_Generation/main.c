#include <stdio.h>
#include <string.h>
#include "Dungeon_Generation.h"

int main(int argc, char *argv[])
{
  dungeon_t dungeon;

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
        return -1;

      }
    }
  }

  // Initialize a new dungeon
  if(init_dungeon(&dungeon, load_flag, save_flag)) {
    printf("init_dungeon function call contained errors\n");
  }

  // Display the dungeon
  show_dungeon(&dungeon);

  // Calculate the dungeon paths
  calculate_paths(&dungeon);

  // Show the paths
  show_paths(&dungeon);
  
  // Free memory allocated
  del_dungeon(&dungeon);


  return 0;
}
