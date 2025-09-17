#ifdef CLANGD
#include "world.cpp"
#endif

internal Sim_Entity* Add_Entity(Game_State* game_state, Sim_Region* sim_region, U32 storage_index, Low_Entity* source,
                                Vec2* sim_pos);

internal Sim_Entity_Hash* Get_Hash_From_Storage_Index(Sim_Region* sim_region, U32 storage_index)

{
  ASSERT(storage_index);
  Sim_Entity_Hash* result = 0;

  U32 hash_value = storage_index;
  for (U32 offset = 0; offset < Array_Count(sim_region->hash); ++offset)
  {
    U32 hash_mask = (Array_Count(sim_region->hash) - 1);
    U32 hash_index = ((hash_value + offset) & hash_mask);

    Sim_Entity_Hash* entry = sim_region->hash + hash_index;
    if ((entry->index == 0) || (entry->index == storage_index))
    {
      result = entry;
      break;
    }
  }

  return result;
}

inline void Load_Entity_Reference(Game_State* game_state, Sim_Region* sim_region, Entity_Reference* ref)
{
  if (ref->index)
  {
    Sim_Entity_Hash* entry = Get_Hash_From_Storage_Index(sim_region, ref->index);
    if (entry->ptr == 0)
    {
      entry->index = ref->index;
      entry->ptr = Add_Entity(game_state, sim_region, ref->index, Get_Low_Entity(game_state, ref->index), 0);
    }
    ref->ptr = entry->ptr;
  }
}

inline void Store_Entity_Reference(Entity_Reference* ref)
{
  if (ref->ptr != 0)
  {
    U32 index = ref->ptr->storage_index;
    ref->index = index;
  }
}

inline Vec2 Get_Sim_Space_Pos(Sim_Region* sim_region, Low_Entity* stored)
{
  // NOTE: Map entity into camera space
  // TODO: Do we want to set this to signalling NAN in debug mode
  // to make sure nobody evers uses the position of a non spatial entity
  Vec2 result = Invalid_Pos;
  if (!Has_Flag(&stored->sim, ENTITY_FLAG_NONSPATIAL))
  {
    World_Diff diff = World_Pos_Subtract(sim_region->world, &stored->pos, &sim_region->origin);
    result = diff.dxy;
  }
  return result;
}

internal Sim_Entity* Add_Entity_Raw(Game_State* game_state, Sim_Region* sim_region, U32 storage_index,
                                    Low_Entity* source)
{
  ASSERT(storage_index);
  Sim_Entity* entity = 0;
  Sim_Entity_Hash* entry = Get_Hash_From_Storage_Index(sim_region, storage_index);
  if (entry->ptr == 0)
  {
    if (sim_region->entity_count < sim_region->max_entity_count)
    {
      entity = sim_region->entities + sim_region->entity_count++;

      entry->index = storage_index;
      entry->ptr = entity;

      if (source)
      {
        // TODO: this should really be a decompression step, not a copy!
        *entity = source->sim;
        entity->chunk_z = source->pos.chunk_z;
        Load_Entity_Reference(game_state, sim_region, &entity->sword);

        ASSERT(!Has_Flag(&source->sim, ENTITY_FLAG_SIMMING));
        Set_Flag(&source->sim, ENTITY_FLAG_SIMMING);
      }
      entity->storage_index = storage_index;
      entity->updatable = false;
    }
    else
    {
      Invalid_Code_Path;
    }
  }
  return entity;
}

internal Sim_Entity* Add_Entity(Game_State* game_state, Sim_Region* sim_region, U32 storage_index, Low_Entity* source,
                                Vec2* sim_pos)
{
  Sim_Entity* dest = Add_Entity_Raw(game_state, sim_region, storage_index, source);
  if (dest)
  {
    if (sim_pos)
    {
      dest->pos = *sim_pos;
    }
    else
    {
      dest->pos = Get_Sim_Space_Pos(sim_region, source);
    }
    dest->updatable = Is_In_Rect(sim_region->update_bounds, dest->pos);
  }
  return dest;
}

internal Sim_Region* Begin_Sim(Arena* sim_arena, Game_State* game_state, World* world, World_Position origin,

                               Rect2 bounds)
{
  // TODO: if entities stored in world we wouldnt need game state here

  Sim_Region* sim_region = Push_Struct(sim_arena, Sim_Region);
  Zero_Struct(sim_region->hash);

  // IMPORTANT: TODO - Calculate this eventually from the maximum of all entities
  // radius + per frame movement amount
  F32 update_safety_margin = 1.f; // one meter

  sim_region->world = world;
  sim_region->origin = origin;
  sim_region->update_bounds = bounds;
  sim_region->bounds = Rect_Add_Radius(bounds, update_safety_margin, update_safety_margin);

  sim_region->max_entity_count = 4096;
  sim_region->entity_count = 0;
  sim_region->entities = Push_Array(sim_arena, sim_region->max_entity_count, Sim_Entity);

  World_Position min_chunk_pos =
      Map_Into_Chunk_Space(sim_region->world, sim_region->origin, Rect_Get_Min_Corner(sim_region->bounds));
  World_Position max_chunk_pos =
      Map_Into_Chunk_Space(sim_region->world, sim_region->origin, Rect_Get_Max_Corner(sim_region->bounds));

  for (S32 chunk_y = min_chunk_pos.chunk_y; chunk_y <= max_chunk_pos.chunk_y; ++chunk_y)
  {
    for (S32 chunk_x = min_chunk_pos.chunk_x; chunk_x <= max_chunk_pos.chunk_x; ++chunk_x)
    {
      World_Chunk* chunk = Get_World_Chunk(sim_region->world, chunk_x, chunk_y, sim_region->origin.chunk_z);
      if (chunk)
      {
        for (World_Entity_Block* block = &chunk->first_block; block; block = block->next)
        {
          for (U32 block_entity_index = 0; block_entity_index < block->entity_count; ++block_entity_index)
          {
            U32 low_entity_index = block->low_entity_index[block_entity_index];
            Low_Entity* low = game_state->low_entities + low_entity_index;
            if (!Has_Flag(&low->sim, ENTITY_FLAG_NONSPATIAL))
            {
              Vec2 entity_pos_in_sim_space = Get_Sim_Space_Pos(sim_region, low);
              if ((low->pos.chunk_z == sim_region->origin.chunk_z) &&
                  (Is_In_Rect(sim_region->bounds, entity_pos_in_sim_space)))
              {
                Add_Entity(game_state, sim_region, low_entity_index, low, &entity_pos_in_sim_space);
              }
            }
          }
        }
      }
    }
  }
  return sim_region;
}
inline Sim_Entity* Get_Entity_By_Storage_Index(Sim_Region* sim_region, U32 storage_index)
{
  Sim_Entity_Hash* entry = Get_Hash_From_Storage_Index(sim_region, storage_index);
  Sim_Entity* result = entry->ptr;
  return result;
}

internal void End_Sim(Sim_Region* region, Game_State* game_state)
{
  // TODO: maybe don't take game_state here, low entities should be stored in the world

  Sim_Entity* entity = region->entities;
  for (U32 entity_index = 0; entity_index < region->entity_count; ++entity_index, ++entity)
  {
    Low_Entity* stored = game_state->low_entities + entity->storage_index;

    ASSERT(Has_Flag(&stored->sim, ENTITY_FLAG_SIMMING));
    stored->sim = *entity;
    ASSERT(!Has_Flag(&stored->sim, ENTITY_FLAG_SIMMING));

    Store_Entity_Reference(&stored->sim.sword);

    // TODO: save state back to store entities once high entities do state decompression

    World_Position new_pos = Null_Position();
    if (!Has_Flag(entity, ENTITY_FLAG_NONSPATIAL))
    {
      new_pos = Map_Into_Chunk_Space(game_state->world, region->origin, entity->pos);
      new_pos.chunk_z = entity->chunk_z;
    }
    Change_Entity_Location(&game_state->world_arena, game_state->world, entity->storage_index, stored, new_pos);

    // NOTE: Update camera position
    // TODO: entity mapping hash table
    if (entity->storage_index == game_state->camera_follow_entity_index)
    {
      World_Position new_camera_pos = game_state->camera_pos;
      Vec2 new_camera_offset = {};

      new_camera_pos.chunk_z = entity->chunk_z;

      F32 tile_size = game_state->world->tile_size_in_meters;
      F32 tile_map_width = (TILES_PER_WIDTH / 2.f) * tile_size;
      F32 tile_map_height = (TILES_PER_HEIGHT / 2.f) * tile_size;

      if (stored->sim.pos.x > tile_map_width)
      {
        new_camera_offset.x += TILES_PER_WIDTH * tile_size;
      }
      else if (stored->sim.pos.x < -tile_map_width)
      {
        new_camera_offset.x -= TILES_PER_WIDTH * tile_size;
      }
      if (stored->sim.pos.y > tile_map_height)
      {
        new_camera_offset.y += TILES_PER_HEIGHT * tile_size;
      }
      else if (stored->sim.pos.y < -tile_map_height)
      {
        new_camera_offset.y -= TILES_PER_HEIGHT * tile_size;
      }

      new_camera_pos = Map_Into_Chunk_Space(game_state->world, new_camera_pos, new_camera_offset);
      game_state->camera_pos = new_camera_pos;
    }
  }
}
internal B32 Test_Wall(F32 wall_x, F32 rel_x, F32 rel_y, F32 player_delta_x, F32 player_delta_y, F32* t_min, F32 min_y,
                       F32 max_y)
{
  B32 hit = false;
  F32 t_epsilon = 0.0001f;
  if (player_delta_x != 0.0f)
  {
    F32 t_result = (wall_x - rel_x) / player_delta_x;
    F32 y = rel_y + t_result * player_delta_y;
    if ((t_result >= 0.0f) && (*t_min > t_result))

    {
      if ((y > min_y) && (y <= max_y))
      {
        *t_min = MAX(0.0f, t_result - t_epsilon);
        hit = true;
      }
    }
  }
  return hit;
}

internal void Move_Entity(Sim_Region* sim_region, Sim_Entity* entity, F32 delta_time, Move_Spec* move_spec, Vec2 accel)
{
  ASSERT(!Has_Flag(entity, ENTITY_FLAG_NONSPATIAL));

  if (move_spec)
  {
    if (move_spec->normalize_accel)
    {
      accel = Vec2_NormalizeSafe(accel);
    }
    accel *= move_spec->speed;
    // friction
    accel += entity->vel * (-move_spec->drag);
  }
  else
  {
    Invalid_Code_Path;
  }

  // NOTE: analytical closed-form kinematic equation
  //  new_pos = (0.5f * accel * dt^2) + vel * dt + old_pos
  Vec2 player_delta = accel * 0.5f * Square(delta_time) + entity->vel * delta_time;
  entity->vel = entity->vel + accel * delta_time;

  F32 gravity = -9.8f;
  entity->z = 0.5f * gravity * Square(delta_time) + entity->vel_z * delta_time + entity->z;
  if (entity->z > 0.f)
  {
    entity->vel_z += gravity * delta_time;
  }
  entity->z = MAX(entity->z, 0.0f);

  for (U32 iteration = 0; (iteration < 4); ++iteration)
  {
    F32 t_min = 1.0f;
    Vec2 wall_normal = {};
    Sim_Entity* hit_entity = 0;
    Vec2 desired_pos = entity->pos + player_delta;

    if (Has_Flag(entity, ENTITY_FLAG_COLLIDES) && !Has_Flag(entity, ENTITY_FLAG_NONSPATIAL))
    {
      // TODO: spatial partition here
      for (U32 test_entity_index = 0; test_entity_index < sim_region->entity_count; ++test_entity_index)
      {
        Sim_Entity* test_entity = sim_region->entities + test_entity_index;

        B32 should_test_collision = test_entity != entity && Has_Flag(test_entity, ENTITY_FLAG_COLLIDES) &&
                                    !Has_Flag(test_entity, ENTITY_FLAG_NONSPATIAL) &&
                                    test_entity->chunk_z == entity->chunk_z;

        if (should_test_collision)
        {
          F32 test_width = test_entity->width + entity->width;
          F32 test_height = test_entity->height + entity->height;
          Vec2 min_corner = -0.5f * Vec2{{test_width, test_height}};
          Vec2 max_corner = 0.5f * Vec2{{test_width, test_height}};

          Vec2 rel = entity->pos - test_entity->pos;
          if (Test_Wall(min_corner.x, rel.x, rel.y, player_delta.x, player_delta.y, &t_min, min_corner.y, max_corner.y))
          {
            hit_entity = test_entity;
            wall_normal = {{-1, 0}};
          }
          if (Test_Wall(max_corner.x, rel.x, rel.y, player_delta.x, player_delta.y, &t_min, min_corner.y, max_corner.y))
          {
            hit_entity = test_entity;
            wall_normal = {{1, 0}};
          }
          if (Test_Wall(min_corner.y, rel.y, rel.x, player_delta.y, player_delta.x, &t_min, min_corner.x, max_corner.x))
          {
            hit_entity = test_entity;
            wall_normal = {{0, -1}};
          }
          if (Test_Wall(max_corner.y, rel.y, rel.x, player_delta.y, player_delta.x, &t_min, min_corner.x, max_corner.x))
          {
            hit_entity = test_entity;
            wall_normal = {{0, 1}};
          }
        }
      }
    }
    entity->pos += t_min * player_delta;
    if (hit_entity)
    {
      entity->vel = Vec2_Slide(entity->vel, wall_normal);
      player_delta = Vec2_Slide(desired_pos - entity->pos, wall_normal);

      entity->chunk_z += (U32)hit_entity->delta_abs_tile_z;
    }
    else
    {
      break;
    }
  }

  // NOTE: Update player sprite direction
  //
  if (Abs_F32(entity->vel.x) > Abs_F32(entity->vel.y))
  {
    if (entity->vel.x > 0)
    {
      entity->sprite_index = E_SPRITE_WALK_RIGHT_1;
    }
    else if (entity->vel.x < 0)
    {
      entity->sprite_index = E_SPRITE_WALK_LEFT_1;
    }
  }
  else if (Abs_F32(entity->vel.x) < Abs_F32(entity->vel.y))
  {
    if (entity->vel.y > 0)
    {
      entity->sprite_index = E_SPRITE_WALK_BACK_1;
    }
    else if (entity->vel.y < 0)
    {
      entity->sprite_index = E_SPRITE_WALK_FRONT_1;
    }
  }
}
