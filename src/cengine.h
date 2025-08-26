#ifndef CENGINE_H

#include "cengine_platform.h"

typedef struct Canonical_Position Canonical_Position;
struct Canonical_Position
{
  /*TODO:
   *
   *  Take the tile map x and y and tile x and y
   *
   *  and pack them into a single 32 bit value for x and y
   *  where there is some low bits for the tile index
   *  and high bits are the tile "page" (like 16x16)
   *
   */
  S32 tile_map_x;
  S32 tile_map_y;
  S32 tile_x;
  S32 tile_y;

  /* TODO:
   *
   * Convert these to math friendly (Y is up)
   */
  F32 tile_rel_x;
  F32 tile_rel_y;
};



typedef struct Tile_Map Tile_Map;
struct Tile_Map
{

  U32* tiles;
};

typedef struct World World;
struct World
{
  F32 tile_size_in_meters;
  S32 tile_size_in_pixels;
  F32 meters_to_pixels;

  // NOTE: number of tiles in a tile map
  S32 tile_count_x;
  S32 tile_count_y;

  S32 upper_left_x;
  S32 upper_left_y;

  // NOTE: number of tile maps in world
  S32 tile_map_count_x;
  S32 tile_map_count_y;
  Tile_Map* tile_maps;
};

struct Game_State
{
  F64 sine_phase;
  F32 volume;

  Canonical_Position player_p;
};

#define CENGINE_H
#endif /* CENGINE_H */
