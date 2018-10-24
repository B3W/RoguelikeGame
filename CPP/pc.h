#ifndef PC_H
# define PC_H

# include <stdint.h>

# include "dims.h"
# include "dungeon.h"

class character;

class player : public character {
public:
  terrain_type_t player_map[DUNGEON_Y][DUNGEON_X];
};

void pc_delete(player *pc);
uint32_t pc_is_alive(dungeon *d);
void update_player_map(dungeon *d);
void config_player_map(dungeon *d);
void config_pc(dungeon *d);
uint32_t pc_next_pos(dungeon *d, pair_t dir);
void place_pc(dungeon *d);
uint32_t pc_in_room(dungeon *d, uint32_t room);

#endif
