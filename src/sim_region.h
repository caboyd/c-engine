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

  ENTITY_TYPE_SPACE,

  ENTITY_TYPE_PLAYER,
  ENTITY_TYPE_FAMILIAR,
  ENTITY_TYPE_MONSTER,
  ENTITY_TYPE_WALL,
  ENTITY_TYPE_STAIR,
  ENTITY_TYPE_SWORD,
};

enum Sim_Entity_Flags
{
  // NOTE: collides and z_supported can probably be removed soon
  ENTITY_FLAG_COLLIDES = bit1,
  ENTITY_FLAG_NONSPATIAL = bit2,
  ENTITY_FLAG_MOVEABLE = bit3,
  ENTITY_FLAG_Z_SUPPORTED = bit4,
  ENTITY_FLAG_TRAVERSABLE = bit5,

  ENTITY_FLAG_SIMMING = bit32
};

struct Sim_Entity;

union Entity_Reference
{
  Sim_Entity* ptr;
  U32 index;
};

struct Sim_Entity_Collision_Volume
{
  Vec3 offset_pos;
  Vec3 dim;
};

struct Sim_Entity_Collision_Volume_Group
{
  Sim_Entity_Collision_Volume total_volume;
  U32 volume_count;
  Sim_Entity_Collision_Volume* volumes;
};

struct Sim_Entity
{

  U32 storage_index;
  B32 updatable;

  Entity_Type type;
  Sim_Entity_Flags flags;
  U32 sprite_index;

  Vec3 pos; // NOTE:Relative to camera
  Vec3 vel;
  F32 bob_time;

  S32 chunk_z;

  F32 distance_limit;

  Sim_Entity_Collision_Volume_Group* collision;

  // Note: This is for stairs

  U32 high_entity_index;

  S32 delta_abs_tile_z;
  S32 max_health;
  S32 health;
  Entity_Reference sword;

  // NOTE: for stairs
  F32 walkable_height;
  Vec2 walkable_dim;

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
  F32 max_entity_radius;
  F32 max_entity_vel;

  World_Position origin;
  Rect3 bounds;
  Rect3 update_bounds;

  U32 max_entity_count;
  U32 entity_count;
  Sim_Entity* entities;

  // TODO: do we want to keep a hash for this
  // NOTE: must be a power of two
  Sim_Entity_Hash hash[4096];
};

#endif // SIM_REGION_H
