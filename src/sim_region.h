#ifndef SIM_REGION_H
#define SIM_REGION_H

struct Move_Spec
{
  B32 normalize_accel;
  F32 speed;
  F32 drag;
};

enum Entity_Type
{
  ENTITY_TYPE_NULL = 0,
  ENTITY_TYPE_PLAYER,
  ENTITY_TYPE_FAMILIAR,
  ENTITY_TYPE_MONSTER,
  ENTITY_TYPE_WALL,
  ENTITY_TYPE_STAIR,
  ENTITY_TYPE_SWORD,
};

enum Sim_Entity_Flags
{
  ENTITY_FLAG_COLLIDES = bit1,
  ENTITY_FLAG_NONSPATIAL = bit2,

  ENTITY_FLAG_SIMMING = bit32
};

struct Sim_Entity;
union Entity_Reference
{
  Sim_Entity* ptr;
  U32 index;
};

struct Sim_Entity
{

  U32 storage_index;
  B32 updatable;

  Entity_Type type;
  Sim_Entity_Flags flags;

  Vec2 pos; // NOTE:Relative to camera
  Vec2 vel;
  S32 chunk_z;
  U32 sprite_index;

  F32 bob_time;

  F32 z;
  F32 vel_z;

  F32 width;
  F32 height;

  // Note: This is for stairs

  S32 delta_abs_tile_z;

  U32 high_entity_index;

  U32 max_health;
  U32 health;
  Entity_Reference sword;
  F32 sword_distance_remaining;
  // TODO: generation index so we know how up to date the entity is
};

struct Sim_Entity_Hash
{
  Sim_Entity* ptr;
  U32 index;
};

struct Sim_Region
{
  World* world;

  World_Position origin;
  Rect2 bounds;
  Rect2 update_bounds;

  U32 max_entity_count;
  U32 entity_count;
  Sim_Entity* entities;

  // TODO: do we want to keep a hash for this
  // NOTE: must be a power of two
  Sim_Entity_Hash hash[4096];
};

#endif // SIM_REGION_H
