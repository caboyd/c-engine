#ifndef CENGINE_H

#include "base/base_core.h"
#include "base/base_intrinsics.h"
#include "base/base_math.h"
#include "base/base_color.h"
#include "cengine_platform.h"
#include "tile.h"

typedef struct World World;
struct World
{
  Tile_Map* tile_map;
};

typedef struct Loaded_Bitmap
{
  S32 width;
  S32 height;

  U32* pixels;

} Loaded_Bitmap;

typedef struct Game_State Game_State;
struct Game_State
{
  // IMPORTANT: tracks bytes used of memory,
  // required for sparse storage of playback files
  // IMPORTANT Do not move from first element in struct
  // platform layer with cast void* memory block to read this
  // TODO: may need to change this approach when more memory is used
  // from transient storage
  // IDEA: have this arena be a ptr to the arena with the highest
  // memory address in the case i have multiple arenas
  Arena permananent_arena;

  World* world;
  Tile_Map_Position player_p;
  Loaded_Bitmap player_bmp;

  Loaded_Bitmap test_bmp;
  Loaded_Bitmap wall1_bmp;
  Loaded_Bitmap wall2_bmp;

  F64 sine_phase;
  F32 volume;
};

#define CENGINE_H
#endif /* CENGINE_H */
