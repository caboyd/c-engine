#ifndef TILE_H
#define TILE_H

#define TILES_PER_WIDTH 17
#define TILES_PER_HEIGHT 9

typedef struct Tile_Map_Diff
{
  Vec2 dxy;
  F32 dz;
} Tile_Map_Diff;

typedef struct Tile_Map_Position Tile_Map_Position;
struct Tile_Map_Position
{
  // NOTE: these are fixed point tile locations
  //  The high bits are the tile chunk index, the low bits are the tile index in the chunk
  U32 abs_tile_x;
  U32 abs_tile_y;
  U32 abs_tile_z;

  // NOTE: These are offset from center of tile
  Vec2 offset_;
};

typedef struct Tile_Chunk Tile_Chunk;
struct Tile_Chunk
{

  U32* tiles;
};

typedef struct Tile_Chunk_Position Tile_Chunk_Position;
struct Tile_Chunk_Position
{
  U32 tile_chunk_x;
  U32 tile_chunk_y;
  U32 tile_chunk_z;

  U32 chunk_rel_tile_x;
  U32 chunk_rel_tile_y;
};

typedef struct Tile_Map Tile_Map;
struct Tile_Map
{
  U32 chunk_shift;
  U32 chunk_mask;
  U32 chunk_dim;

  F32 tile_size_in_meters;

  // NOTE: number of tile maps in world
  U32 tile_chunk_count_x;
  U32 tile_chunk_count_y;
  U32 tile_chunk_count_z;

  Tile_Chunk* tile_chunks;
};

#endif // TILE_H
