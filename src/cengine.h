#ifndef CENGINE_H
#define CENGINE_H

#include "cengine_platform.h" // NOLINT(clang-diagnostic-unused-includes)

struct Game_State
{
  F64 sine_phase;
  F32 volume;

  F32 player_x;
  F32 player_y;
};
typedef struct Tile_Map Tile_Map;
struct Tile_Map
{
  S32 count_x;
  S32 count_y;
  F32 upper_left_x;
  F32 upper_left_y;
  F32 tile_width;
  F32 tile_height;
  U32* tiles;
};

typedef struct World World;
struct World
{
  S32 count_x;
  S32 count_y;
  Tile_Map* tile_maps;
};

#endif /* CENGINE_H */
