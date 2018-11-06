#ifndef OBJECT_H
#define OBJECT_H

#include <stdint.h>
#include <string>

#include "dice.h"

class object {
 private:
  std::string name, description;
  char symbol;
  uint32_t color;
  int32_t hit, dodge, defence, weight, speed, attribute, value;
  dice damage;
  
 public:
  object() : name(),       description(), symbol(),
	     color(0),     hit(0),        dodge(0),
	     defence(0),   weight(0),     speed(0),
	     attribute(0), value(0),      damage()
  {
  }
  void set(const std::string &name,
           const std::string &description,
           const char symbol,
           const uint32_t color,
           const int32_t hit,
           const dice &damage,
           const int32_t dodge,
           const int32_t defence,
           const int32_t weight,
           const int32_t speed,
           const int32_t attribute,
           const int32_t value);
  
  inline const std::string &get_name() const { return name; }
  inline const std::string &get_description() const { return description; }
  inline const char get_symbol() const { return symbol; }
  inline const uint32_t get_color() const { return color; }
  inline const int32_t get_hit() const { return hit; }
  inline const dice &get_damage() const { return damage; }
  inline const int32_t get_dodge() const { return dodge; }
  inline const int32_t get_defence() const { return defence; }
  inline const int32_t get_weight() const { return weight; }
  inline const int32_t get_speed() const { return speed; }
  inline const int32_t get_attribute() const { return attribute; }
  inline const int32_t get_value() const { return value; }
};

#endif
