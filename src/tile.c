#include "tile.h"

Tile_Chunk_Position Get_Chunk_Position(Tile_Map* tile_map, U32 abs_tile_x, U32 abs_tile_y, U32 tile_z)
{
  Tile_Chunk_Position result;
  result.tile_chunk_x = abs_tile_x >> tile_map->chunk_shift;
  result.tile_chunk_y = abs_tile_y >> tile_map->chunk_shift;
  result.tile_chunk_z = tile_z;
  result.chunk_rel_tile_x = abs_tile_x & tile_map->chunk_mask;
  result.chunk_rel_tile_y = abs_tile_y & tile_map->chunk_mask;

  return result;
}
internal inline U32 Get_Tile_Chunk_Value_Unchecked(Tile_Map* tile_map, Tile_Chunk* tile_chunk, U32 tile_x, U32 tile_y)

{
  ASSERT(tile_chunk);
  ASSERT(tile_x < tile_map->chunk_dim);
  ASSERT(tile_y < tile_map->chunk_dim);

  U32 tile_chunk_value = tile_chunk->tiles[(tile_y * tile_map->chunk_dim) + tile_x];
  return tile_chunk_value;
}
internal inline Tile_Chunk* Tile_Map_Get_Tile_Chunk(Tile_Map* tile_map, U32 tile_chunk_x, U32 tile_chunk_y,
                                                    U32 tile_chunk_z)
{
  Tile_Chunk* tile_chunk = 0;
  if ((tile_chunk_x >= 0 && tile_chunk_x < tile_map->tile_chunk_count_x) &&
      (tile_chunk_y >= 0 && tile_chunk_y < tile_map->tile_chunk_count_x) &&
      (tile_chunk_z >= 0 && tile_chunk_z < tile_map->tile_chunk_count_z))
  {
    tile_chunk = &tile_map->tile_chunks[(tile_chunk_z * tile_map->tile_chunk_count_y * tile_map->tile_chunk_count_x) +
                                        (tile_chunk_y * tile_map->tile_chunk_count_x) + tile_chunk_x];
  }
  return tile_chunk;
}

internal U32 Get_Tile_Chunk_Value(Tile_Map* tile_map, Tile_Chunk* tile_chunk, U32 test_tile_x, U32 test_tile_y)
{
  U32 tile_value = 0;
  if (tile_chunk && tile_chunk->tiles)
  {
    tile_value = Get_Tile_Chunk_Value_Unchecked(tile_map, tile_chunk, test_tile_x, test_tile_y);
  }
  return tile_value;
}
internal U32 Get_Tile_Value(Tile_Map* tile_map, U32 abs_tile_x, U32 abs_tile_y, U32 abs_tile_z)
{
  Tile_Chunk_Position chunk_pos = Get_Chunk_Position(tile_map, abs_tile_x, abs_tile_y, abs_tile_z);
  Tile_Chunk* tile_chunk =
      Tile_Map_Get_Tile_Chunk(tile_map, chunk_pos.tile_chunk_x, chunk_pos.tile_chunk_y, chunk_pos.tile_chunk_z);

  U32 tile_value = Get_Tile_Chunk_Value(tile_map, tile_chunk, chunk_pos.chunk_rel_tile_x, chunk_pos.chunk_rel_tile_y);
  return tile_value;
}

internal inline void Set_Tile_Chunk_Value_Unchecked(Tile_Map* tile_map, Tile_Chunk* tile_chunk, U32 tile_x, U32 tile_y,
                                                    U32 tile_value)
{
  ASSERT(tile_chunk);
  ASSERT(tile_x < tile_map->chunk_dim);
  ASSERT(tile_y < tile_map->chunk_dim);

  tile_chunk->tiles[(tile_y * tile_map->chunk_dim) + tile_x] = tile_value;
}

internal void Set_Tile_Chunk_Value(Tile_Map* tile_map, Tile_Chunk* tile_chunk, U32 test_tile_x, U32 test_tile_y,
                                   U32 tile_value)
{
  if (tile_chunk && tile_chunk->tiles)
  {
    Set_Tile_Chunk_Value_Unchecked(tile_map, tile_chunk, test_tile_x, test_tile_y, tile_value);
  }
}
internal void Set_Tile_Value(Arena* arena, Tile_Map* tile_map, U32 abs_tile_x, U32 abs_tile_y, U32 abs_tile_z,
                             U32 tile_value)
{
  Tile_Chunk_Position chunk_pos = Get_Chunk_Position(tile_map, abs_tile_x, abs_tile_y, abs_tile_z);
  Tile_Chunk* tile_chunk =
      Tile_Map_Get_Tile_Chunk(tile_map, chunk_pos.tile_chunk_x, chunk_pos.tile_chunk_y, chunk_pos.tile_chunk_z);

  ASSERT(tile_chunk);
  if (!tile_chunk->tiles)
  {
    U32 tile_count = tile_map->chunk_dim * tile_map->chunk_dim;
    tile_chunk->tiles = Push_Array(arena, tile_count, U32);
    for (U32 tile_index = 0; tile_index < tile_count; ++tile_index)
    {
      tile_chunk->tiles[tile_index] = 1;
    }
  }
  Set_Tile_Chunk_Value(tile_map, tile_chunk, chunk_pos.chunk_rel_tile_x, chunk_pos.chunk_rel_tile_y, tile_value);
}

internal B32 Is_Tile_Map_Tile_Empty(Tile_Map* tile_map, Tile_Map_Position tile_map_pos)
{
  U32 tile_value = Get_Tile_Value(tile_map, tile_map_pos.abs_tile_x, tile_map_pos.abs_tile_y, tile_map_pos.abs_tile_z);
  B32 is_empty = (tile_value == 1);
  return is_empty;
}
