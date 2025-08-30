#ifndef CENGINE_H

#include "base/base_core.h"
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
  S32 pixel_count;
  U32* pixels;

} Loaded_Bitmap;

typedef struct Game_State Game_State;
struct Game_State
{
  Arena world_arena;

  World* world;
  Tile_Map_Position player_p;
  Loaded_Bitmap player_bmp;
  Loaded_Bitmap* test_bmp;
  Loaded_Bitmap* wall1_bmp;
  Loaded_Bitmap* wall2_bmp;

  F64 sine_phase;
  F32 volume;
};

#define CENGINE_H
#endif /* CENGINE_H */
