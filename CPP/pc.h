#ifndef PC_H
# define PC_H

# include <array>
# include <vector>
# include <stdint.h>

# include "dims.h"
# include "character.h"
# include "dungeon.h"

typedef enum equip_position {eqslot_INVALID,
			     eqslot_WEAPON,
			     eqslot_OFFHAND,
			     eqslot_RANGED,
			     eqslot_ARMOR,
			     eqslot_HELMET,
			     eqslot_CLOAK,
			     eqslot_GLOVES,
			     eqslot_BOOTS,
			     eqslot_AMULET,
			     eqslot_LIGHT,
			     eqslot_RING
}equip_position_t;

class object;
typedef enum object_type object_type_t;

class pc : public character {
 public:
  ~pc() {}
  terrain_type known_terrain[DUNGEON_Y][DUNGEON_X];
  uint8_t visible[DUNGEON_Y][DUNGEON_X];
  std::vector<object *> inventory;
  std::array<object *, 12> equipment;
};

equip_position_t get_epos(int32_t type);
void pc_delete(pc *pc);
uint32_t pc_is_alive(dungeon *d);
void config_pc(dungeon *d);
uint32_t pc_next_pos(dungeon *d, pair_t dir);
void place_pc(dungeon *d);
uint32_t pc_in_room(dungeon *d, uint32_t room);
void pc_learn_terrain(pc *p, pair_t pos, terrain_type ter);
terrain_type pc_learned_terrain(pc *p, int16_t y, int16_t x);
void pc_init_known_terrain(pc *p);
void pc_observe_terrain(pc *p, dungeon *d);
int32_t is_illuminated(pc *p, int16_t y, int16_t x);
void pc_reset_visibility(pc *p);

#endif
