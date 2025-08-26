#ifndef CENGINE_H
#define CENGINE_H

#include "cengine_platform.h"

struct Game_State
{
  F64 sine_phase;
  F32 volume;

  S32 player_tile_map_x;
  S32 player_tile_map_y;
  F32 player_x;
  F32 player_y;
};

typedef struct Canonical_Position Canonical_Position;
struct Canonical_Position
{
  S32 tile_map_x;
  S32 tile_map_y;
  S32 tile_x;
  S32 tile_y;

  // NOTE: tile relative X and Y
  F32 tile_rel_x;
  F32 tile_rel_y;
};

typedef struct Raw_Position Raw_Position;
struct Raw_Position
{
  S32 tile_map_x;
  S32 tile_map_y;

  // NOTE: tile_map relative X and Y
  F32 x;
  F32 y;
};

typedef struct Tile_Map Tile_Map;
struct Tile_Map
{

  U32* tiles;
};

typedef struct World World;
struct World
{
  S32 tile_count_x;
  S32 tile_count_y;
  F32 upper_left_x;
  F32 upper_left_y;
  F32 tile_width;
  F32 tile_height;

  S32 tile_map_count_x;
  S32 tile_map_count_y;
  Tile_Map* tile_maps;
};

#endif /* CENGINE_H */
