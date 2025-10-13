#ifndef CENGINE_H
#define CENGINE_H

#include "base/base_core.h"
#include "base/base_intrinsics.h"
#include "base/base_math.h"
#include "math/vec.h"
#include "cengine_platform.h"
#include "color.h"
#include "rand.h"
#include "math/rect.h"
#include "world.h"
#include "sim_region.h"
#include "entity.h"

#include "render_group.h"

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

enum E_Sprite_Sheet_Walk
{
  E_SPRITE_WALK_FRONT_1 = 0,
  E_SPRITE_WALK_BACK_1,
  E_SPRITE_WALK_LEFT_1,
  E_SPRITE_WALK_RIGHT_1,

  E_SPRITE_WALK_FRONT_2,
  E_SPRITE_WALK_BACK_2,
  E_SPRITE_WALK_LEFT_2,
  E_SPRITE_WALK_RIGHT_2,

  E_SPRITE_WALK_FRONT_3,
  E_SPRITE_WALK_BACK_3,
  E_SPRITE_WALK_LEFT_3,
  E_SPRITE_WALK_RIGHT_3,

  E_SPRITE_WALK_FRONT_4,
  E_SPRITE_WALK_BACK_4,
  E_SPRITE_WALK_LEFT_4,
  E_SPRITE_WALK_RIGHT_4,

  E_SPRITE_WALK_COUNT // 16
};

enum E_Sprite_Sheet_Character
{
  E_SPRITE_ATTACK_FRONT = E_SPRITE_WALK_COUNT,
  E_SPRITE_ATTACK_BACK,
  E_SPRITE_ATTACK_LEFT,
  E_SPRITE_ATTACK_RIGHT,

  E_SPRITE_JUMP_FRONT,
  E_SPRITE_JUMP_BACK,
  E_SPRITE_JUMP_LEFT,
  E_SPRITE_JUMP_RIGHT,

  E_SPRITE_DEAD,
  E_SPRITE_ITEM_PICKUP,
  E_SPRITE_SPECIAL_1,
  E_SPRITE_SPECIAL_2,

  E_SPRITE_CHAR_COUNT,
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
  Vec2 offset;
  U32 sprite_index;
  Sprite_Sheet* sprite_sheet;
  Loaded_Bitmap* bitmap;
};

struct High_Entity
{
  Vec2 pos; // NOTE:Relative to camera
  Vec2 vel;
  S32 chunk_z;
  U32 sprite_index;

  F32 bob_time;

  F32 z;
  F32 vel_z;

  U32 low_entity_index;
};

struct Low_Entity
{
  World_Position pos;
  Sim_Entity sim;
};

enum E_Entity_Residence
{
  E_ENTITY_RESIDENCE_NONEXISTENT,
  E_ENTITY_RESIDENCE_DORMANT,
  E_ENTITY_RESIDENCE_LOW,
  E_ENTITY_RESIDENCE_HIGH,
};

struct Controlled_Player
{
  U32 entity_index;
  Vec2 move_dir;
  B32 sprint;
  Vec2 attack_dir;
  F32 jump_vel;
};

struct Pairwise_Collision_Rule
{
  B32 should_collide;
  U32 storage_index_a;
  U32 storage_index_b;
  Pairwise_Collision_Rule* next_in_hash;
};

struct Game_State;

internal void Clear_Collision_Rules_For(Game_State* game_state, U32 storage_index);
internal B32 Add_Collision_Rule(Game_State* game_state, U32 storage_index_a, U32 storage_index_b, B32 should_collide);

struct Ground_Buffer
{
  // NOTE: Invalid pos means ground buffer has not been filled
  World_Position pos;
  Loaded_Bitmap bitmap;
};

struct Game_State
{
  // IMPORTANT: tracks bytes used of memory,
  // required for sparse storage of playback files
  // IMPORTANT Do not move from first element in struct
  // platform layer with cast void* memory block to read this
  // TODO: may need to change this approach when more memory is used
  // from transient storage
  // IDEA: have this a ptr to the arena with the highest
  // memory address in the case i have multiple arenas
  Arena world_arena;

  World* world;

  F32 typical_floor_height;

  F32 meters_to_pixels;
  F32 pixels_to_meters;
  F32 sprite_scale;
  F32 draw_scale;

  U32 camera_follow_entity_index;
  World_Position camera_pos;

  Controlled_Player controlled_players[Array_Count(((Game_Input*)0)->controllers)];
  F64 sine_phase;
  F32 volume;
  F32 time;

  Sprite_Sheet knight_sprite_sheet;
  Sprite_Sheet grass_sprite_sheet;
  Sprite_Sheet slime_sprite_sheet;
  Sprite_Sheet eye_sprite_sheet;

  Loaded_Bitmap test_bmp;
  Loaded_Bitmap test_tree_bmp;
  Loaded_Bitmap test_tree_normal;
  Loaded_Bitmap test_diffuse;
  Loaded_Bitmap test_normal;
  Loaded_Bitmap shadow_bmp;
  Loaded_Bitmap stone_floor_bmp;
  Loaded_Bitmap stair_up_bmp;
  Loaded_Bitmap stair_down_bmp;
  Loaded_Bitmap wall1_bmp;
  Loaded_Bitmap wall2_bmp;
  Loaded_Bitmap pillar_bmp;
  Loaded_Bitmap shuriken_bmp;
  Loaded_Bitmap tuft[3];
  Loaded_Bitmap ground[4];
  Loaded_Bitmap grass[2];

  U32 high_entity_count;
  High_Entity high_entities[256];
  U32 low_entity_count;
  Low_Entity low_entities[100000];

  // NOTE: must be power of two
  Pairwise_Collision_Rule* collision_rule_hash[256];
  Pairwise_Collision_Rule* first_free_collision_rule;
  U32 collision_rule_count;

  Sim_Entity_Collision_Volume_Group* null_collision;
  Sim_Entity_Collision_Volume_Group* player_collision;
  Sim_Entity_Collision_Volume_Group* familiar_collision;
  Sim_Entity_Collision_Volume_Group* sword_collision;
  Sim_Entity_Collision_Volume_Group* wall_collision;
  Sim_Entity_Collision_Volume_Group* stair_collision;
  Sim_Entity_Collision_Volume_Group* monster_collision;
  Sim_Entity_Collision_Volume_Group* standard_room_collision;
};

struct Transient_State
{
  B32 is_initialized;
  Arena transient_arena;
  Loaded_Bitmap ground_bitmap_template;
  U32 ground_buffer_count;
  Ground_Buffer* ground_buffers;

  U32 env_map_width;
  U32 env_map_height;
  // NOTE: 0 is bottom, 1 is middle, 2 is top
  Environment_Map env_maps[3];
};

internal Low_Entity* Get_Low_Entity(Game_State* game_state, U32 index)
{
  Low_Entity* result = 0;

  if ((index) > 0 && (index < game_state->low_entity_count))
  {
    result = game_state->low_entities + index;
  }

  return result;
}

#endif /* CENGINE_H */
