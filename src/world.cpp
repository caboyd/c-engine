

#define TILE_CHUNK_SAFE_MARGIN (INT32_MAX / 64)
#define TILE_CHUNK_UNINITIALIZED INT32_MAX
#define TILE_SIZE_IN_METERS 1.4f

internal inline World_Chunk* Get_Tile_Chunk(World* world, S32 tile_chunk_x, S32 tile_chunk_y, S32 tile_chunk_z,
                                            Arena* arena = 0)
{
  ASSERT(tile_chunk_x > -TILE_CHUNK_SAFE_MARGIN);
  ASSERT(tile_chunk_y > -TILE_CHUNK_SAFE_MARGIN);
  ASSERT(tile_chunk_z > -TILE_CHUNK_SAFE_MARGIN);
  ASSERT(tile_chunk_x < TILE_CHUNK_SAFE_MARGIN);
  ASSERT(tile_chunk_y < TILE_CHUNK_SAFE_MARGIN);
  ASSERT(tile_chunk_z < TILE_CHUNK_SAFE_MARGIN);

  // TODO: Better hash function
  U32 hash_value = (U32)(19 * tile_chunk_x + 7 * tile_chunk_y + 3 * tile_chunk_z);
  U32 hash_slot = hash_value & (Array_Count(world->chunk_hash) - 1);
  ASSERT(hash_slot < Array_Count(world->chunk_hash));

  World_Chunk* chunk = world->chunk_hash + hash_slot;
  do
  {
    if ((tile_chunk_x == chunk->chunk_x) && (tile_chunk_y == chunk->chunk_y) && (tile_chunk_z == chunk->chunk_z))
    {
      break;
    }

    if (arena && (chunk->chunk_x != TILE_CHUNK_UNINITIALIZED) && (!chunk->next_in_hash))
    {
      chunk->next_in_hash = Push_Struct(arena, World_Chunk);
      chunk = chunk->next_in_hash;
      chunk->chunk_x = TILE_CHUNK_UNINITIALIZED;
    }

    if (arena && (chunk->chunk_x == TILE_CHUNK_UNINITIALIZED))
    {

      chunk->chunk_x = tile_chunk_x;
      chunk->chunk_y = tile_chunk_y;
      chunk->chunk_z = tile_chunk_z;
      chunk->next_in_hash = 0;

      break;
    }
    chunk = chunk->next_in_hash;
  } while (chunk);

  return chunk;
}

internal void Initialize_World(World* world, F32 tile_size_in_meters)
{
  world->chunk_shift = 4;
  world->chunk_mask = (1u << world->chunk_shift) - 1;
  world->chunk_dim = (1u << world->chunk_shift);
  world->tile_size_in_meters = 1.4f;
  for (U32 tile_chunk_index = 0; tile_chunk_index < Array_Count(world->chunk_hash); ++tile_chunk_index)
  {
    world->chunk_hash[tile_chunk_index].chunk_x = TILE_CHUNK_UNINITIALIZED;
  }
}

internal void Recanonicalize_Coord(World* world, S32* tile, F32* tile_rel)
{
  // TODO: Shoud this function be in a different file about positioning
  // TODO: Need to fix rounding for very small tile_rel floats caused by float precision.
  //  Ex near -0.00000001 tile_rel + 60 to result in 60 wrapping to next tile
  //  Don't use divide multiple method
  //

  S32 tile_offset = Round_F32_S32(*tile_rel / (F32)world->tile_size_in_meters);

  *tile += tile_offset;

  *tile_rel -= (F32)tile_offset * (F32)world->tile_size_in_meters;

  ASSERT(*tile_rel >= -0.5f * world->tile_size_in_meters);
  ASSERT(*tile_rel <= 0.5f * world->tile_size_in_meters);
}

internal World_Position Map_Into_Tile_Space(World* world, World_Position base_pos, Vec2 offset)
{
  World_Position result = base_pos;
  base_pos.offset_ += offset;

  Recanonicalize_Coord(world, &result.abs_tile_x, &result.offset_.x);
  Recanonicalize_Coord(world, &result.abs_tile_y, &result.offset_.y);

  return result;
}

internal B32 Is_On_Same_Tile(World_Position old_pos, World_Position new_pos)
{
  B32 result;
  result = (old_pos.abs_tile_x == new_pos.abs_tile_x && old_pos.abs_tile_y == new_pos.abs_tile_y &&
            old_pos.abs_tile_z == new_pos.abs_tile_z);
  return result;
}

internal World_Diff World_Pos_Subtract(World* world, World_Position* a, World_Position* b)
{
  World_Diff result;
  Vec2 dtile_xy = {.x = (F32)a->abs_tile_x - (F32)b->abs_tile_x, .y = (F32)a->abs_tile_y - (F32)b->abs_tile_y};
  F32 dtile_z = (F32)a->abs_tile_z - (F32)b->abs_tile_z;

  result.dxy = (a->offset_ - b->offset_) + dtile_xy * world->tile_size_in_meters;

  result.dz = world->tile_size_in_meters * dtile_z;

  return result;
}
internal World_Position Centered_Tile_Point(S32 abs_tile_x, S32 abs_tile_y, S32 abs_tile_z)

{
  World_Position result = {};
  result.abs_tile_x = abs_tile_x;
  result.abs_tile_y = abs_tile_y;
  result.abs_tile_z = abs_tile_z;
  return result;
}
// internal World_Chunk_Poisition Get_Chunk_Position(World* world, S32 abs_tile_x, S32 abs_tile_y, S32 abs_tile_z)
// {
//   World_Chunk_Position result;
//   result.chunk_x = abs_tile_x >> world->chunk_shift;
//   result.chunk_y = abs_tile_y >> world->chunk_shift;
//   result.chunk_z = abs_tile_z;
//   result.chunk_rel_tile_x = (U32)abs_tile_x & world->chunk_mask;
//   result.chunk_rel_tile_y = (U32)abs_tile_y & world->chunk_mask;
//
//   return result;
// }
// internal inline U32 Get_Tile_Chunk_Value_Unchecked(Tile_Map* tile_map, Tile_Chunk* tile_chunk, U32 tile_x, U32
// tile_y)
//
// {
//   ASSERT(tile_chunk);
//   ASSERT(tile_x < tile_map->chunk_dim);
//   ASSERT(tile_y < tile_map->chunk_dim);
//
//   U32 tile_chunk_value = tile_chunk->tiles[(tile_y * tile_map->chunk_dim) + tile_x];
//   return tile_chunk_value;
// }
//
// internal U32 Get_Tile_Value(Tile_Map* tile_map, Tile_Chunk* tile_chunk, U32 test_tile_x, U32 test_tile_y)
// {
//   U32 tile_value = 0;
//   if (tile_chunk && tile_chunk->tiles)
//   {
//     tile_value = Get_Tile_Chunk_Value_Unchecked(tile_map, tile_chunk, test_tile_x, test_tile_y);
//   }
//   return tile_value;
// }
//
// internal U32 Get_Tile_Value(Tile_Map* tile_map, S32 abs_tile_x, S32 abs_tile_y, S32 abs_tile_z)
// {
//   Tile_Chunk_Position chunk_pos = Get_Chunk_Position(tile_map, abs_tile_x, abs_tile_y, abs_tile_z);
//   Tile_Chunk* tile_chunk =
//       Get_Tile_Chunk(tile_map, chunk_pos.tile_chunk_x, chunk_pos.tile_chunk_y, chunk_pos.tile_chunk_z);
//
//   U32 tile_value = Get_Tile_Value(tile_map, tile_chunk, chunk_pos.chunk_rel_tile_x, chunk_pos.chunk_rel_tile_y);
//   return tile_value;
// }
//
// internal U32 Get_Tile_Value(Tile_Map* tile_map, Tile_Map_Position tile_map_pos)
// {
//   U32 tile_value = Get_Tile_Value(tile_map, tile_map_pos.abs_tile_x, tile_map_pos.abs_tile_y,
//   tile_map_pos.abs_tile_z); return tile_value;
// }
// internal B32 Is_Tile_Value_Empty(U32 tile_value)
// {
//   B32 is_empty = (tile_value == 1 || tile_value == 3 || tile_value == 4);
//   return is_empty;
// }

// internal B32 Is_Tile_Map_Position_Empty(Tile_Map* tile_map, Tile_Map_Position tile_map_pos)
// {
//   U32 tile_value = Get_Tile_Value(tile_map, tile_map_pos.abs_tile_x, tile_map_pos.abs_tile_y,
//   tile_map_pos.abs_tile_z); B32 is_empty = Is_Tile_Value_Empty(tile_value); return is_empty;
// }

// internal inline void Set_Tile_Chunk_Value_Unchecked(Tile_Map* tile_map, Tile_Chunk* tile_chunk, U32 tile_x, U32
// tile_y,
//                                                     U32 tile_value)
// {
//   ASSERT(tile_chunk);
//   ASSERT(tile_x < tile_map->chunk_dim);
//   ASSERT(tile_y < tile_map->chunk_dim);
//
//   tile_chunk->tiles[(tile_y * tile_map->chunk_dim) + tile_x] = tile_value;
// }
//
// internal void Set_Tile_Value(Tile_Map* tile_map, Tile_Chunk* tile_chunk, U32 test_tile_x, U32 test_tile_y,
//                              U32 tile_value)
// {
//   if (tile_chunk && tile_chunk->tiles)
//   {
//     Set_Tile_Chunk_Value_Unchecked(tile_map, tile_chunk, test_tile_x, test_tile_y, tile_value);
//   }
// }
//
// internal void Set_Tile_Value(Arena* arena, Tile_Map* tile_map, S32 abs_tile_x, S32 abs_tile_y, S32 abs_tile_z,
//                              U32 tile_value)
// {
//   Tile_Chunk_Position chunk_pos = Get_Chunk_Position(tile_map, abs_tile_x, abs_tile_y, abs_tile_z);
//   Tile_Chunk* tile_chunk =
//       Get_Tile_Chunk(tile_map, chunk_pos.tile_chunk_x, chunk_pos.tile_chunk_y, chunk_pos.tile_chunk_z, arena);
//
//   ASSERT(tile_chunk);
//   if (!tile_chunk->tiles)
//   {
//     U32 tile_count = tile_map->chunk_dim * tile_map->chunk_dim;
//     tile_chunk->tiles = Push_Array(arena, tile_count, U32);
//     for (U32 tile_index = 0; tile_index < tile_count; ++tile_index)
//     {
//       tile_chunk->tiles[tile_index] = 1;
//     }
//   }
//   Set_Tile_Value(tile_map, tile_chunk, chunk_pos.chunk_rel_tile_x, chunk_pos.chunk_rel_tile_y, tile_value);
// }
