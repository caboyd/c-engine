

#define TILE_CHUNK_SAFE_MARGIN (INT32_MAX / 64)
#define TILE_CHUNK_UNINITIALIZED INT32_MAX
#define TILE_SIZE_IN_METERS 1.4f
#define TILES_PER_CHUNK 16
#define TILE_SIZE_IN_PIXELS 32

inline World_Position Null_Position()
{
  World_Position result = {};
  result.chunk_x = TILE_CHUNK_UNINITIALIZED;

  return result;
}

inline B32 Is_Valid(World_Position pos)
{
  B32 result = (pos.chunk_x != TILE_CHUNK_UNINITIALIZED);
  return result;
}

inline B32 Is_Canonical(World* world, F32 tile_rel)
{
  B32 result = ((tile_rel >= -0.5f * world->chunk_size_in_meters) && (tile_rel <= 0.5f * world->chunk_size_in_meters));

  return result;
}

inline B32 Is_Canonical(World* world, Vec2 offset)
{
  B32 result = (Is_Canonical(world, offset.x) && Is_Canonical(world, offset.y));

  return result;
}

internal inline World_Chunk* Get_World_Chunk(World* world, S32 tile_chunk_x, S32 tile_chunk_y, S32 tile_chunk_z,
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
  world->tile_size_in_meters = tile_size_in_meters;
  world->chunk_size_in_meters = 16.0f * tile_size_in_meters;
  world->first_free = 0;
  for (U32 chunk_index = 0; chunk_index < Array_Count(world->chunk_hash); ++chunk_index)
  {
    world->chunk_hash[chunk_index].chunk_x = TILE_CHUNK_UNINITIALIZED;
    world->chunk_hash[chunk_index].first_block.entity_count = 0;
  }
}

internal void Recanonicalize_Coord(World* world, S32* tile, F32* tile_rel)
{

  // NOTE: at the moment
  //  tile  0,0,0 is tile_rel (0.f,0.f) of chunk(0,0,0)
  //  tile -7,0,0 is tile_rel (-7 * 1.4, 0.f) of chunk(0,0,0)
  //  tile  8,0,0 is tile_rel (8 * 1.4, 0.f) of chunk(0,0,0)
  // NOTE:Wrapping is not allowed, all coords must be within safe margin
  // TODO: Assert we are not near edge of world

  // NOTE: fix rounding for very small tile_rel floats caused by float precision.
  S32 offset = (S32)Floor_F32_S32((*tile_rel + 0.5f * world->chunk_size_in_meters) / world->chunk_size_in_meters);
  *tile += offset;
  *tile_rel -= (F32)offset * (F32)world->chunk_size_in_meters;

  ASSERT(Is_Canonical(world, *tile_rel));
}

internal World_Position Map_Into_Chunk_Space(World* world, World_Position base_pos, Vec2 offset)
{
  World_Position result = base_pos;
  result.offset_ += offset;

  // ASSERT(Is_Canonical(world, base_pos.offset_));
  Recanonicalize_Coord(world, &result.chunk_x, &result.offset_.x);
  Recanonicalize_Coord(world, &result.chunk_y, &result.offset_.y);

  return result;
}

internal B32 Are_In_Same_Chunk(World* world, World_Position* a, World_Position* b)

{
  ASSERT(Is_Canonical(world, a->offset_));
  ASSERT(Is_Canonical(world, b->offset_));

  B32 result;
  result = ((a->chunk_x == b->chunk_x) && (a->chunk_y == b->chunk_y) && (a->chunk_z == b->chunk_z));

  return result;
}
internal World_Position Chunk_Position_From_Tile_Position(World* world, S32 abs_tile_x, S32 abs_tile_y, S32 abs_tile_z)

{
  World_Position result = {};
  result.chunk_x = abs_tile_x / TILES_PER_CHUNK;
  result.chunk_y = abs_tile_y / TILES_PER_CHUNK;
  result.chunk_z = abs_tile_z;
  if (abs_tile_x < 0)
  {
    result.chunk_x--;
  }
  if (abs_tile_y < 0)
  {
    result.chunk_y--;
  }

  result.offset_.x =
      (F32)((abs_tile_x - TILES_PER_CHUNK / 2) - (result.chunk_x * TILES_PER_CHUNK)) * world->tile_size_in_meters;

  result.offset_.y =
      (F32)((abs_tile_y - TILES_PER_CHUNK / 2) - (result.chunk_y * TILES_PER_CHUNK)) * world->tile_size_in_meters;

  ASSERT(Is_Canonical(world, result.offset_));

  return result;
}

internal World_Diff World_Pos_Subtract(World* world, World_Position* a, World_Position* b)
{
  World_Diff result;
  Vec2 dtile_xy = {{(F32)a->chunk_x - (F32)b->chunk_x, (F32)a->chunk_y - (F32)b->chunk_y}};
  F32 dtile_z = (F32)a->chunk_z - (F32)b->chunk_z;

  result.dxy = (a->offset_ - b->offset_) + dtile_xy * world->chunk_size_in_meters;

  result.dz = world->chunk_size_in_meters * dtile_z;

  return result;
}
internal World_Position Centered_Chunk_Point(S32 chunk_x, S32 chunk_y, S32 chunk_z)

{
  World_Position result = {};
  result.chunk_x = chunk_x;
  result.chunk_y = chunk_y;
  result.chunk_z = chunk_z;
  return result;
}

inline void Change_Entity_Location_Raw(Arena* arena, World* world, U32 low_entity_index, World_Position* old_pos,
                                       World_Position* new_pos)
{
  ASSERT(!old_pos || Is_Valid(*old_pos));
  ASSERT(!new_pos || Is_Valid(*new_pos));

  if (old_pos && new_pos && Are_In_Same_Chunk(world, old_pos, new_pos))
  {
    // leave entity where it is
  }
  else
  {
    if (old_pos)
    {
      // pull entity out of its old block
      World_Chunk* chunk = Get_World_Chunk(world, old_pos->chunk_x, old_pos->chunk_y, old_pos->chunk_z);
      ASSERT(chunk);

      if (chunk)
      {
        B32 not_found = true;
        World_Entity_Block* first_block = &chunk->first_block;
        for (World_Entity_Block* block = first_block; block && not_found; block = block->next)
        {
          for (U32 index = 0; (index < block->entity_count) && not_found; ++index)
          {
            if (block->low_entity_index[index] == low_entity_index)
            {
              ASSERT(first_block->entity_count > 0);
              block->low_entity_index[index] = first_block->low_entity_index[--first_block->entity_count];
              if (first_block->entity_count == 0)
              {
                if (first_block->next)
                {
                  World_Entity_Block* next_block = first_block->next;
                  *first_block = *next_block;

                  // NOTE: adding the unused block to the free list
                  next_block->next = world->first_free;
                  world->first_free = next_block;
                }
              }
              not_found = false;
            }
          }
        }
      }
    }
    if (new_pos)
    {
      // insert entity into its new block
      World_Chunk* chunk = Get_World_Chunk(world, new_pos->chunk_x, new_pos->chunk_y, new_pos->chunk_z, arena);
      ASSERT(chunk);
      World_Entity_Block* block = &chunk->first_block;
      if (chunk->first_block.entity_count == Array_Count(chunk->first_block.low_entity_index))
      {
        // NOTE: we're out of room get a new block from free list or make a new one
        World_Entity_Block* next_block = world->first_free;
        if (next_block)
        {
          world->first_free = next_block->next;
        }
        else
        {
          next_block = Push_Struct(arena, World_Entity_Block);
        }
        // NOTE: Put next_block at the front of the daisy chain after block
        *next_block = *block;
        block->next = next_block;
        // NOTE: clear the block so it can be used
        block->entity_count = 0;
      }

      ASSERT(block->entity_count < Array_Count(block->low_entity_index));
      block->low_entity_index[block->entity_count++] = low_entity_index;
    }
  }
}
internal void Change_Entity_Location(Arena* arena, World* world, U32 low_entity_index, Low_Entity* low_entity,
                                     World_Position* old_pos, World_Position* new_pos)

{
  Change_Entity_Location_Raw(arena, world, low_entity_index, old_pos, new_pos);
  if (new_pos)
  {
    low_entity->pos = *new_pos;
  }
  else
  {
    low_entity->pos = Null_Position();
  }
}
