#ifndef CENGINE_H
#define CENGINE_H

#include "base/base_core.h"
#include "base/base_intrinsics.h"
#include "base/base_math.h"
#include "base/base_color.h"
#include "cengine_platform.h"
#include "rand.h"
#include "math/vec.h"
#include "math/rect.h"
#include "world.h"

struct Loaded_Bitmap
{
  S32 width;
  S32 height;

  U32* pixels;
};

#pragma pack(push, 1)
struct Bitmap_Header
{
  U16 Type;
  U32 SizeFile;
  U16 Reserved1;
  U16 Reserved2;
  U32 OffBits;
  U32 SizeHeader2;
  S32 Width;
  S32 Height;
  U16 Planes;
  U16 BitCount;
  U32 Compression;
  U32 SizeImage;
  S32 XPelsPerMeter;
  S32 YPelsPerMeter;
  U32 ClrUsed;
  U32 ClrImportant;
  U32 RedMask;
  U32 GreenMask;
  U32 BlueMask;
  U32 AlphaMask;
};
#pragma pack(pop)

enum E_Sprite_Sheet_Character
{
  E_CHAR_WALK_FRONT_1 = 0,
  E_CHAR_WALK_BACK_1,
  E_CHAR_WALK_LEFT_1,
  E_CHAR_WALK_RIGHT_1,
  E_CHAR_WALK_FRONT_2,
  E_CHAR_WALK_BACK_2,
  E_CHAR_WALK_LEFT_2,
  E_CHAR_WALK_RIGHT_2,
  E_CHAR_WALK_FRONT_3,
  E_CHAR_WALK_BACK_3,
  E_CHAR_WALK_LEFT_3,
  E_CHAR_WALK_RIGHT_3,
  E_CHAR_WALK_FRONT_4,
  E_CHAR_WALK_BACK_4,
  E_CHAR_WALK_LEFT_4,
  E_CHAR_WALK_RIGHT_4,
  E_CHAR_ATTACK_FRONT,
  E_CHAR_ATTACK_BACK,
  E_CHAR_ATTACK_LEFT,
  E_CHAR_ATTACK_RIGHT,
  E_CHAR_JUMP_FRONT,
  E_CHAR_JUMP_BACK,
  E_CHAR_JUMP_LEFT,
  E_CHAR_JUMP_RIGHT,
  E_CHAR_DEAD,
  E_CHAR_ITEM_PICKUP,
  E_CHAR_SPECIAL_1,
  E_CHAR_SPECIAL_2,

};

struct Sprite
{
  S32 x;
  S32 y;
  S32 width;
  S32 height;
};

struct Sprite_Sheet
{
  Loaded_Bitmap bitmap;
  S32 sprite_width;
  S32 sprite_height;
  Sprite* sprites;
  S32 sprite_count;
};

struct Entity_Sprite
{
  S32 align_x;
  S32 align_y;
  E_Sprite_Sheet_Character sprite_index;
  Sprite_Sheet* sprite_sheet;
  Loaded_Bitmap* bitmap;
};

struct High_Entity
{
  B32 exists;
  Vec2 pos; // NOTE:Relative to camera
  Vec2 vel;
  F32 z;
  F32 vel_z;
  S32 chunk_z;

  U32 sprite_index;

  U32 low_entity_index;
};

enum E_Entity_Type
{
  E_ENTITY_TYPE_NULL = 0,
  E_ENTITY_TYPE_PLAYER,
  E_ENTITY_TYPE_WALL,
  E_ENTITY_TYPE_STAIR,
};

struct Low_Entity
{
  E_Entity_Type type;
  World_Position pos;
  F32 width;
  F32 height;

  // Note: This is for stairs
  B32 collides;
  S32 delta_abs_tile_z;

  U32 high_entity_index;
};

enum E_Entity_Residence
{
  E_ENTITY_RESIDENCE_NONEXISTENT,
  E_ENTITY_RESIDENCE_DORMANT,
  E_ENTITY_RESIDENCE_LOW,
  E_ENTITY_RESIDENCE_HIGH,
};

struct Entity
{
  U32 low_index;
  High_Entity* high;
  Low_Entity* low;
};

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

  U32 camera_follow_entity_index;
  World_Position camera_p;

  U32 player_index_for_controller[Array_Count(((Game_Input*)0)->controllers)];
  F64 sine_phase;
  F32 volume;

  Sprite_Sheet knight_sprite_sheet;
  Sprite_Sheet grass_sprite_sheet;

  Loaded_Bitmap test_bmp;
  Loaded_Bitmap shadow_bmp;
  Loaded_Bitmap stone_floor_bmp;
  Loaded_Bitmap stair_up_bmp;
  Loaded_Bitmap stair_down_bmp;
  Loaded_Bitmap wall1_bmp;
  Loaded_Bitmap wall2_bmp;
  Loaded_Bitmap pillar_bmp;

  U32 high_entity_count;
  High_Entity high_entities[256];
  U32 low_entity_count;
  Low_Entity low_entities[100000];
};

#endif /* CENGINE_H */
