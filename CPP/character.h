#ifndef CHARACTER_H
# define CHARACTER_H

# include <stdint.h>
# include <vector>

# include "dims.h"
# include "dice.h"

class dungeon;

typedef enum kill_type {
  kill_direct,
  kill_avenged,
  num_kill_types
} kill_type_t;

class character {
private:
  int32_t hitpoints;
  dice damage;
  std::vector<uint32_t> color;
  uint32_t desc_index;
public:
  char symbol;
  pair_t position;
  int32_t speed;
  uint32_t alive;
  /* Characters use to have a next_turn for the move queue.  Now that it is *
   * an event queue, there's no need for that here.  Instead it's in the    *
   * event.  Similarly, sequence_number was introduced in order to ensure   *
   * that the queue remains stable.  Also no longer necessary here, but in  *
   * this case, we'll keep it, because it provides a bit of interesting     *
   * metadata: locally, how old is this character; and globally, how many   *
   * characters have been created by the game.                              */
  uint32_t sequence_number;
  uint32_t kills[num_kill_types];
  character() : hitpoints(0),  damage(), color(),
		desc_index(0), symbol(), position(),
		speed(0),      alive(0), sequence_number(0),
		kills()
  {
  }
  virtual ~character() = 0;
  void set_killed(dungeon *d);
  inline void set_hitpoints(const int32_t hp) { hitpoints = hp; }
  inline void set_damage(const dice &dmg) { damage = dmg; }
  inline void set_color(const std::vector<uint32_t> &col) { color = col; }
  inline void set_index(const uint32_t i) { desc_index = i; }
  inline const int32_t get_hitpoints() const { return hitpoints; }
  inline const dice &get_damage() const { return damage; }
  inline const std::vector<uint32_t> &get_color() const { return color; }
};

int32_t compare_characters_by_next_turn(const void *character1,
                                        const void *character2);
uint32_t can_see(dungeon *d, pair_t voyeur, pair_t exhibitionist,
                 int is_pc, int learn);
void character_delete(character *c);
int16_t *character_get_pos(character *c);
int16_t character_get_y(const character *c);
int16_t character_set_y(character *c, int16_t y);
int16_t character_get_x(const character *c);
int16_t character_set_x(character *c, int16_t x);
uint32_t character_get_next_turn(const character *c);
void character_die(character *c);
int character_is_alive(const character *c);
void character_next_turn(character *c);
void character_reset_turn(character *c);
char character_get_symbol(const character *c);
uint32_t character_get_speed(const character *c);
uint32_t character_get_dkills(const character *c);
uint32_t character_get_ikills(const character *c);
uint32_t character_increment_dkills(character *c);
uint32_t character_increment_ikills(character *c, uint32_t k);

#endif
