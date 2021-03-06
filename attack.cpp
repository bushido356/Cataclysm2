#include "field.h"
#include "attack.h"
#include "rng.h"
#include "stringfunc.h"
#include "window.h"
#include "entity.h" // For Stats
#include "item.h"
#include "globals.h" // For FIELDS
#include "game.h"
#include <sstream>

bool load_verbs(std::istream &data, std::string &verb_second,
                std::string &verb_third, std::string owner_name);

Attack::Attack()
{
  verb_second = "hit";
  verb_third = "hits";
  weight =  10;
  speed  = 100;
  to_hit =   0;
  for (int i = 0; i < DAMAGE_MAX; i++) {
    damage[i] = 0;
  }
}

Attack::~Attack()
{
}

bool Attack::load_data(std::istream &data, std::string owner_name)
{
  std::string ident, junk;
  while (ident != "done" && !data.eof()) {

    if ( ! (data >> ident) ) {
      return false;
    }

    ident = no_caps(ident);

    if (!ident.empty() && ident[0] == '#') { // I'ts a comment
      std::getline(data, junk);

    } else if (ident == "verb_second:") {
      std::getline(data, verb_second);
      verb_second = trim(verb_second);

    } else if (ident == "verb_third:") {
      std::getline(data, verb_third);
      verb_third = trim(verb_third);

    } else if (ident == "verb:") {
      if (!load_verbs(data, verb_second, verb_third, owner_name)) {
        return false;
      }

    } else if (ident == "weight:") {
      data >> weight;
      std::getline(data, junk);

    } else if (ident == "speed:") {
      data >> speed;
      std::getline(data, junk);

    } else if (ident == "to_hit:") {
      data >> to_hit;
      std::getline(data, junk);

    } else if (ident != "done") {
// Check if it's a damage type; TODO: make this less ugly.
// Damage_set::load_data() maybe?
      std::string damage_name = ident;
      size_t colon = damage_name.find(':');
      if (colon != std::string::npos) {
        damage_name = damage_name.substr(0, colon);
      }
      Damage_type type = lookup_damage_type(damage_name);
      if (type == DAMAGE_NULL) {
        debugmsg("Unknown Attack property '%s' (%s)",
                 ident.c_str(), owner_name.c_str());
        return false;
      } else {
        data >> damage[type];
      }
    }

  }
  return true;
}

void Attack::use_weapon(Item weapon, Stats stats)
{
  speed  += weapon.get_base_attack_speed(stats);
  to_hit += weapon.get_to_hit();
  for (int i = 0; i < DAMAGE_MAX; i++) {
    damage[i] += weapon.get_damage( Damage_type(i) );
  }
}

Damage_set Attack::roll_damage()
{
  Damage_set ret;
  for (int i = 0; i < DAMAGE_MAX; i++) {
    ret.set_damage( Damage_type(i), rng(0, damage[i]) );
  }
  return ret;
}

Ranged_attack::Ranged_attack()
{
  verb_second = "shoot";
  verb_third = "shoots";
  weight = 10;
  speed = 100;
  charge_time = 0;
  range = 0;
  for (int i = 0; i < DAMAGE_MAX; i++) {
    damage[i] = 0;
    armor_divisor[i] = 10;
  }
}

Ranged_attack::~Ranged_attack()
{
}

bool Ranged_attack::load_data(std::istream &data, std::string owner_name)
{
  std::string ident, junk;
  while (ident != "done" && !data.eof()) {

    if ( ! (data >> ident) ) {
      return false;
    }

    ident = no_caps(ident);

    if (!ident.empty() && ident[0] == '#') { // I'ts a comment
      std::getline(data, junk);

    } else if (ident == "verb_second:") {
      std::getline(data, verb_second);
      verb_second = trim(verb_second);

    } else if (ident == "verb_third:") {
      std::getline(data, verb_third);
      verb_third = trim(verb_third);

    } else if (ident == "verb:" ) {
      if (!load_verbs(data, verb_second, verb_third, owner_name)) {
        return false;
      }

    } else if (ident == "weight:") {
      data >> weight;
      std::getline(data, junk);

    } else if (ident == "speed:") {
      data >> speed;
      std::getline(data, junk);

    } else if (ident == "charge:" || ident == "charge_time:") {
      data >> charge_time;
      std::getline(data, junk);

    } else if (ident == "range:") {
      data >> range;
      std::getline(data, junk);

    } else if (ident == "variance:") {
      if (!variance.load_data(data, owner_name)) {
        return false;
      }

    } else if (ident == "armor_pierce:") {
      std::string damage_name;
      data >> damage_name;
      Damage_type damtype = lookup_damage_type(damage_name);
      if (damtype == DAMAGE_NULL) {
        debugmsg("Unknown damage type for Ranged_attack pierce: '%s' (%s)",
                 damage_name.c_str(), owner_name.c_str());
        return false;
      }
      data >> armor_divisor[damtype];
      if (armor_divisor[damtype] == 0) {
        armor_divisor[damtype] = 10;
      }

    } else if (ident == "wake_field:") {
      if (!wake_field.load_data(data, owner_name + " " + verb_third)) {
        debugmsg("Failed to load wake_field (%s)", owner_name.c_str());
        return false;
      }

    } else if (ident == "target_field:") {
      if (!target_field.load_data(data, owner_name + " " + verb_third)) {
        debugmsg("Failed to load target_field (%s)", owner_name.c_str());
        return false;
      }

    } else if (ident != "done") {
      std::string damage_name = ident;
      size_t colon = ident.find(':');
      if (colon != std::string::npos) {
        damage_name = ident.substr(0, colon);
      }
      Damage_type type = lookup_damage_type(damage_name);
      if (type == DAMAGE_NULL) {
        debugmsg("Unknown Attack property '%s' (%s)",
                 ident.c_str(), owner_name.c_str());
        return false;
      } else {
        data >> damage[type];
      }
    }

  }
  return true;
}

int Ranged_attack::roll_variance()
{
  return variance.roll();
}

Damage_set Ranged_attack::roll_damage()
{
  Damage_set ret;
  for (int i = 0; i < DAMAGE_MAX; i++) {
    ret.set_damage( Damage_type(i), rng(0, damage[i]) );
  }
  return ret;
}


Body_part random_body_part_to_hit()
{
  int pick = rng(1, 38);
  if (pick <= 4) {
    return BODY_PART_HEAD;
  }
  if (pick <= 16) {
    return BODY_PART_TORSO;
  }
  if (pick == 17) {
    return BODY_PART_LEFT_HAND;
  }
  if (pick == 18) {
    return BODY_PART_RIGHT_HAND;
  }
  if (pick <= 22) {
    return BODY_PART_LEFT_ARM;
  }
  if (pick <= 26) {
    return BODY_PART_RIGHT_ARM;
  }
  if (pick == 27) {
    return BODY_PART_LEFT_FOOT;
  }
  if (pick == 28) {
    return BODY_PART_RIGHT_FOOT;
  }
  if (pick <= 33) {
    return BODY_PART_LEFT_LEG;
  }
  return BODY_PART_RIGHT_LEG;
}

bool load_verbs(std::istream &data, std::string &verb_second,
                std::string &verb_third, std::string owner_name)
{
  std::string verb_line;
  std::getline(data, verb_line);
  std::istringstream verb_data(verb_line);

  std::string tmpword;
  std::string tmpverb;
  bool reading_second = true;

  while (verb_data >> tmpword) {
    if (tmpword == "/") {
      if (!reading_second) {
        debugmsg("Too many / in verb definition (%s)", owner_name.c_str());
        return false;
      }
      reading_second = false;
      verb_second = tmpverb;
      tmpverb = "";
    } else {
      if (!tmpverb.empty()) {
        tmpverb += " ";
      }
      tmpverb += tmpword;
    }
  }

  if (reading_second) {
    debugmsg("Verb definition has 2nd person, but no 3rd", owner_name.c_str());
    return false;
  }
  verb_third = tmpverb;

  return true;
}
