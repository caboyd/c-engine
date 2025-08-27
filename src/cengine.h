#ifndef CENGINE_H

#include "base/base_core.h"
#include "base/base_math.h"
#include "cengine_platform.h"
#include "tile.h"



typedef struct World World;
struct World
{
  Tile_Map* tile_map;
};

struct Game_State
{
  Arena world_arena;

  World* world;
  Tile_Map_Position player_p;

  F64 sine_phase;
  F32 volume;
};

#define CENGINE_H
#endif /* CENGINE_H */
