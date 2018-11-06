#include "object.h"

void object::set(const std::string &name,
		 const std::string &description,
		 const char symbol,
		 const uint32_t color,
		 const int32_t hit,
		 const dice &damage,
		 const int32_t dodge,
		 const int32_t defence,
		 const int32_t weight,
		 const int32_t speed,
		 const int32_t attrubute,
		 const int32_t value)
{
  this->name = name;
  this->description = description;
  this->symbol = symbol;
  this->color = color;
  this->hit = hit;
  this->damage = damage;
  this->dodge = dodge;
  this->defence = defence;
  this->weight = weight;
  this->speed = speed;
  this->attribute = attrubute;
  this->value = value;
}
