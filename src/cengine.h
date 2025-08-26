#ifndef CENGINE_H

#include "cengine_platform.h"

typedef struct World_Position World_Position;
struct World_Position
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
  U32 abs_tile_x;
  U32 abs_tile_y;

  F32 tile_rel_x;
  F32 tile_rel_y;
};

typedef struct Tile_Chunk Tile_Chunk;
struct Tile_Chunk
{

  U32* tiles;
};

typedef struct Tile_Chunk_Position
{
  U32 tile_chunk_x;
  U32 tile_chunk_y;

  U32 chunk_rel_tile_x;
  U32 chunk_rel_tile_y;
} Tile_Chunk_Position;

typedef struct World World;
struct World
{
  U32 chunk_shift;
  U32 chunk_mask;
  U32 chunk_dim;

  F32 tile_size_in_meters;
  S32 tile_size_in_pixels;
  F32 meters_to_pixels;



  // NOTE: number of tile maps in world
  U32 tile_chunk_count_x;
  U32 tile_chunk_count_y;

  Tile_Chunk* tile_chunks;
};

struct Game_State
{
  F64 sine_phase;
  F32 volume;

  World_Position player_p;
};

#define CENGINE_H
#endif /* CENGINE_H */
