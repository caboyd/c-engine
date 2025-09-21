#ifndef WORLD_H

#define WORLD_H

#define TILES_PER_WIDTH 17
#define TILES_PER_HEIGHT 9

struct World_Position
{

  S32 chunk_x;
  S32 chunk_y;
  S32 chunk_z;

  // NOTE: These are offset from center of chunk
  Vec3 offset_;
};

// TODO: could make this just world_chunk and then allow multiple in same (x,y,z)
struct World_Entity_Block
{
  U32 entity_count;
  U32 low_entity_index[16];
  World_Entity_Block* next;
};

struct World_Chunk
{
  S32 chunk_x;
  S32 chunk_y;
  S32 chunk_z;

  // TODO: profile this and determine if a pointer would be better here!
  World_Entity_Block first_block;

  World_Chunk* next_in_hash;
};

struct World
{
  F32 tile_size_in_meters;
  F32 tile_depth_in_meters;
  Vec3 chunk_dim_in_meters;

  // TODO: maybe should be a pointer of tile_chunks store first block directly
  // as empty chunks waste if first block empty
  // NOTE: must be a power of two
  World_Chunk chunk_hash[4096];

  World_Entity_Block* first_free;
};

#endif // WORLD_H
