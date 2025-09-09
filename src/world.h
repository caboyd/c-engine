#ifndef WORLD_H

#define WORLD_H

#define TILES_PER_WIDTH 17
#define TILES_PER_HEIGHT 9

struct World_Diff
{
  Vec2 dxy;
  F32 dz;
};

struct World_Position
{
  // NOTE: these are fixed point tile locations
  //  The high bits are the tile chunk index, the low bits are the tile index in the chunk
  S32 abs_tile_x;
  S32 abs_tile_y;
  S32 abs_tile_z;

  // NOTE: These are offset from center of tile
  Vec2 offset_;
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

  World_Entity_Block first_block;

  World_Chunk* next_in_hash;
};

struct World
{
  F32 tile_size_in_meters;

  U32 chunk_shift;
  U32 chunk_mask;
  U32 chunk_dim;

  // TODO: maybe should be a pointer of tile_chunks store first block directly
  // as empty chunks waste if first block empty
  // NOTE: must be a power of two
  World_Chunk chunk_hash[4096];
};

#endif // WORLD_H
