#include "sim_region.h"
#include "cengine.h"
#ifdef CLANGD
#include "world.cpp"
#endif

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

internal Sim_Entity* Add_Entity(Game_State* game_state, Sim_Region* sim_region, U32 storage_index, Low_Entity* source,
                                Vec3* sim_pos);
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

inline Vec3 Get_Sim_Space_Pos(Sim_Region* sim_region, Low_Entity* stored)
{
  // NOTE: Map entity into camera space
  // TODO: Do we want to set this to signalling NAN in debug mode
  // to make sure nobody evers uses the position of a non spatial entity
  Vec3 result = Invalid_Pos;
  if (!Has_Flag(&stored->sim, ENTITY_FLAG_NONSPATIAL))
  {
    result = World_Pos_Subtract(sim_region->world, &stored->pos, &sim_region->origin);
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
        // NOTE: for stairs
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
inline B32 Entity_Overlaps_Rect(Rect3 rect, Vec3 pos, Sim_Entity_Collision_Volume volume)
{
  Rect3 rect_sum = Rect_Add_Radius(rect, 0.5f * volume.dim);
  B32 result = Is_In_Rect(rect_sum, pos + volume.offset_pos);
  return result;
}

internal Sim_Entity* Add_Entity(Game_State* game_state, Sim_Region* sim_region, U32 storage_index, Low_Entity* source,
                                Vec3* sim_pos)
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
    dest->updatable = Entity_Overlaps_Rect(sim_region->update_bounds, dest->pos, dest->collision->total_volume);
  }
  return dest;
}

internal Sim_Region* Begin_Sim(Arena* sim_arena, Game_State* game_state, World* world, World_Position origin,
                               Rect3 bounds, F32 delta_time)
{
  // TODO: if entities stored in world we wouldnt need game state here

  Sim_Region* sim_region = Push_Struct(sim_arena, Sim_Region);
  Zero_Struct(sim_region->hash);

  // IMPORTANT: TODO - Calculate this eventually from the maximum of all entities
  // radius + per frame movement amount
  sim_region->max_entity_radius = 5.f;
  sim_region->max_entity_vel = 30.f;
  F32 update_safety_margin = sim_region->max_entity_radius + (delta_time * sim_region->max_entity_vel);
  F32 update_safety_margin_z = 1.f; // what should this be?
  //

  sim_region->world = world;
  sim_region->origin = origin;
  sim_region->update_bounds = Rect_Add_Radius(bounds, vec3(sim_region->max_entity_radius));
  sim_region->bounds = Rect_Add_Radius(sim_region->update_bounds,
                                       vec3(update_safety_margin, update_safety_margin, update_safety_margin_z));

  sim_region->max_entity_count = 4096;
  sim_region->entity_count = 0;
  sim_region->entities = Push_Array(sim_arena, sim_region->max_entity_count, Sim_Entity);

  World_Position min_chunk_pos =
      Map_Into_Chunk_Space(sim_region->world, sim_region->origin, Rect_Get_Min_Corner(sim_region->bounds));
  World_Position max_chunk_pos =
      Map_Into_Chunk_Space(sim_region->world, sim_region->origin, Rect_Get_Max_Corner(sim_region->bounds));

  for (S32 chunk_z = min_chunk_pos.chunk_z; chunk_z <= max_chunk_pos.chunk_z; ++chunk_z)
  {
    for (S32 chunk_y = min_chunk_pos.chunk_y; chunk_y <= max_chunk_pos.chunk_y; ++chunk_y)
    {
      for (S32 chunk_x = min_chunk_pos.chunk_x; chunk_x <= max_chunk_pos.chunk_x; ++chunk_x)
      {
        World_Chunk* chunk = Get_World_Chunk(sim_region->world, chunk_x, chunk_y, chunk_z);

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
                // if (sim_region->origin.chunk_z == 1)
                // {
                //   ASSERT(true);
                // }
                if (low->sim.type == ENTITY_TYPE_SPACE)
                {
                  ASSERT(true);
                }
                else if (low->sim.type == ENTITY_TYPE_MONSTER && sim_region->origin.chunk_z == 1)
                {
                  ASSERT(true);
                }
                Vec3 entity_pos_in_sim_space = Get_Sim_Space_Pos(sim_region, low);
                if ((Entity_Overlaps_Rect(sim_region->bounds, entity_pos_in_sim_space,
                                          low->sim.collision->total_volume)))
                {
                  Add_Entity(game_state, sim_region, low_entity_index, low, &entity_pos_in_sim_space);
                }
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

    if (entity->type == ENTITY_TYPE_SPACE)
    {
      ASSERT(true);
    }
    else if (entity->type == ENTITY_TYPE_MONSTER)
    {
      ASSERT(true);
    }
    ASSERT(Has_Flag(&stored->sim, ENTITY_FLAG_SIMMING));
    stored->sim = *entity;
    ASSERT(!Has_Flag(&stored->sim, ENTITY_FLAG_SIMMING));

    Store_Entity_Reference(&stored->sim.sword);

    // TODO: save state back to store entities once high entities do state decompression
    World_Position new_pos = Null_Position();
    if (!Has_Flag(entity, ENTITY_FLAG_NONSPATIAL))
    {

      new_pos = Map_Into_Chunk_Space(game_state->world, region->origin, entity->pos);
      if (entity->type == ENTITY_TYPE_MONSTER && new_pos.chunk_z != entity->chunk_z)
      {
        new_pos = Map_Into_Chunk_Space(game_state->world, region->origin, entity->pos);
        ASSERT(true);
      }
    }

    Change_Entity_Location(&game_state->world_arena, game_state->world, entity->storage_index, stored, new_pos);

    // NOTE: Update camera position
    if (entity->storage_index == game_state->camera_follow_entity_index)
    {
      World_Position new_camera_pos = game_state->camera_pos;
      Vec3 new_camera_offset = {};

      new_camera_pos.chunk_z = stored->pos.chunk_z;

      F32 tile_size = 3.f;
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

      // new_camera_pos = Map_Into_Chunk_Space(game_state->world, new_camera_pos, new_camera_offset);
      new_camera_pos.offset_ += new_camera_offset;
      game_state->camera_pos = new_camera_pos;

      game_state->camera_pos = stored->pos;
      game_state->camera_pos.offset_.z = 0.f;
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

internal B32 Should_Collide(Game_State* game_state, Sim_Entity* a, Sim_Entity* b)
{
  B32 result = false;

  if (a != b)
  {
    if (Has_Flag(a, ENTITY_FLAG_COLLIDES && Has_Flag(b, ENTITY_FLAG_COLLIDES)))
    {
      if (a->storage_index > b->storage_index)
      {
        Sim_Entity* temp = a;
        a = b;
        b = temp;
      }

      if (!Has_Flag(a, ENTITY_FLAG_NONSPATIAL) && !Has_Flag(b, ENTITY_FLAG_NONSPATIAL))
      {
        result = true;
      }
      if ((a->type == ENTITY_TYPE_FAMILIAR) || (b->type == ENTITY_TYPE_FAMILIAR))
      {
        result = false;
      }
      if ((a->type == ENTITY_TYPE_FAMILIAR) && (b->type == ENTITY_TYPE_FAMILIAR))
      {
        result = true;
      }

      // TODO: BETTER HASH FUNCTION
      // NOTE: a bucket is a hash slot with external chaining
      U32 hash_bucket = a->storage_index & (Array_Count(game_state->collision_rule_hash) - 1);
      for (Pairwise_Collision_Rule* rule = game_state->collision_rule_hash[hash_bucket]; rule;
           rule = rule->next_in_hash)
      {
        if ((rule->storage_index_a == a->storage_index) && (rule->storage_index_b == b->storage_index))
        {
          result = rule->should_collide;
          break;
        }
      }
    }
  }
  return result;
}

internal B32 Handle_Collision(Game_State* game_state, Sim_Entity* a, Sim_Entity* b)

{
  B32 stops_on_collision = false;

  if (a->type == ENTITY_TYPE_SWORD)
  {
    Add_Collision_Rule(game_state, a->storage_index, b->storage_index, false);
    stops_on_collision = false;
  }
  else
  {
    stops_on_collision = true;
  }

  if (a->type == ENTITY_TYPE_PLAYER && b->type == ENTITY_TYPE_FAMILIAR)
  {
    stops_on_collision = false;
  }
  if (a->type > b->type)
  {
    Sim_Entity* temp = a;
    a = b;
    b = temp;
  }

  if (a->type == ENTITY_TYPE_FAMILIAR && b->type == ENTITY_TYPE_FAMILIAR)
  {
    stops_on_collision = false;
  }
  if (((a->type == ENTITY_TYPE_MONSTER) && (b->type == ENTITY_TYPE_SWORD)) ||
      ((a->type == ENTITY_TYPE_FAMILIAR) && (b->type == ENTITY_TYPE_SWORD)))
  {
    a->health -= 3;
    if (a->health <= 0)
    {
      Make_Entity_Nonspatial(a);
      Clear_Collision_Rules_For(game_state, a->storage_index);
    }
    Make_Entity_Nonspatial(b);
    Clear_Collision_Rules_For(game_state, b->storage_index);
    stops_on_collision = true;
  }

  return stops_on_collision;
}
internal B32 Should_Handle_Overlap(Game_State* game_state, Sim_Entity* mover, Sim_Entity* region)
{
  B32 result = false;
  if (mover != region)
  {
    if (region->type == ENTITY_TYPE_STAIR)
    {
      result = true;
    }
  }
  return result;
}
internal B32 Speculative_Collide(Sim_Entity* mover, Sim_Entity* region, Vec3 test_pos)
{
  B32 result = true;
  if (region->type == ENTITY_TYPE_STAIR)
  {
    Vec3 mover_ground_point = Get_Entity_Ground_Point(mover, test_pos);
    F32 ground = Get_Stair_Ground(region, mover_ground_point);
    F32 step_height = 0.04f;
    result = (Abs_F32(mover_ground_point.z - ground) > step_height);
  }
  return result;
}
internal void Handle_Overlap(Game_State* game_state, Sim_Entity* mover, Sim_Entity* region, F32 delta_time, F32* ground)
{
  if (region->type == ENTITY_TYPE_STAIR)
  {
    *ground = Get_Stair_Ground(region, Get_Entity_Ground_Point(mover));
  }
}

internal B32 Entities_Overlap(Sim_Entity* entity, Sim_Entity* test_entity, Vec3 epsilon = vec3(0))
{
  B32 result = false;
  for (U32 entity_volume_index = 0; !result && (entity_volume_index < entity->collision->volume_count);
       ++entity_volume_index)
  {
    Sim_Entity_Collision_Volume* entity_volume = entity->collision->volumes + entity_volume_index;

    Rect3 entity_rect = Rect_Center_Dim(entity->pos + entity_volume->offset_pos, entity_volume->dim + epsilon);

    for (U32 test_volume_index = 0; !result && (test_volume_index < test_entity->collision->volume_count);
         ++test_volume_index)
    {
      Sim_Entity_Collision_Volume* test_volume = test_entity->collision->volumes + test_volume_index;

      Rect3 test_entity_rect = Rect_Center_Dim(test_entity->pos + test_volume->offset_pos, test_volume->dim);
      result = Rect_Intersect_Rect(entity_rect, test_entity_rect);
    }
  }
  return result;
}

struct Test_Wall_t
{
  F32 x;
  F32 rel_x;
  F32 rel_y;
  F32 delta_x;
  F32 delta_y;
  F32 min_y;
  F32 max_y;
  Vec3 normal;
};

internal void Move_Entity(Game_State* game_state, Sim_Region* sim_region, Sim_Entity* entity, F32 delta_time,
                          Move_Spec* move_spec, Vec3 accel)
{
  ASSERT(!Has_Flag(entity, ENTITY_FLAG_NONSPATIAL));
  ASSERT(move_spec);

  if (move_spec->normalize_accel)
  {
    accel = Vec_NormalizeSafe(accel);
  }
  accel *= move_spec->speed;
  // friction
  Vec3 drag = (-move_spec->drag) * entity->vel;
  drag.z = 0.f;
  accel += drag;
  if (!Has_Flag(entity, ENTITY_FLAG_Z_SUPPORTED))
  {
    accel += vec3(0, 0, -9.8f); // Gravity
  }

  // NOTE: analytical closed-form kinematic equation
  //  new_pos = (0.5f * accel * dt^2) + vel * dt + old_pos
  Vec3 player_delta = accel * 0.5f * Square(delta_time) + entity->vel * delta_time;
  entity->vel = entity->vel + accel * delta_time;

  // NOTE: player is allowed to exceed max velocity because it never at edge of sim region
  ASSERT(Vec_Length_Sq(entity->vel) < Square(sim_region->max_entity_vel));

  F32 distance_remaining = entity->distance_limit;
  // NOTE: zero means no limit
  if (distance_remaining == 0.f)
  {
    distance_remaining = 10000.0f;
  }

  for (U32 iteration = 0; (iteration < 4); ++iteration)
  {
    F32 t_min = 1.0f;
    F32 t_max = 0.f;
    F32 player_delta_length = Vec_Length(player_delta);

    // TODO: what do we want to do for epsilons here
    if (player_delta_length <= 0)
    {
      break;
    }

    if (player_delta_length > distance_remaining)
    {
      t_min = (distance_remaining / player_delta_length);
    }

    Sim_Entity* hit_entity_max = 0;
    Sim_Entity* hit_entity_min = 0;
    Vec3 wall_normal_min = vec3(0);
    Vec3 wall_normal_max = vec3(0);

    Vec3 desired_pos = entity->pos + player_delta;

    if (!Has_Flag(entity, ENTITY_FLAG_NONSPATIAL))
    {
      // TODO: spatial partition here
      for (U32 test_entity_index = 0; test_entity_index < sim_region->entity_count; ++test_entity_index)
      {
        Sim_Entity* test_entity = sim_region->entities + test_entity_index;
        F32 overlaps_epsilon = 0.001f;
        if (((Has_Flag(test_entity, ENTITY_FLAG_TRAVERSABLE)) &&
             Entities_Overlap(entity, test_entity, vec3(overlaps_epsilon))) ||
            Should_Collide(game_state, entity, test_entity))
        {
          for (U32 entity_volume_index = 0; entity_volume_index < entity->collision->volume_count;
               ++entity_volume_index)
          {
            Sim_Entity_Collision_Volume* entity_volume = entity->collision->volumes + entity_volume_index;

            for (U32 test_volume_index = 0; test_volume_index < test_entity->collision->volume_count;
                 ++test_volume_index)
            {
              Sim_Entity_Collision_Volume* test_volume = test_entity->collision->volumes + test_volume_index;
              Vec3 minkowski_sum_dim =
                  vec3(test_volume->dim.x + entity_volume->dim.x, test_volume->dim.y + entity_volume->dim.y,
                       test_volume->dim.z + entity_volume->dim.z);

              Vec3 min_corner = -0.5f * minkowski_sum_dim;
              Vec3 max_corner = 0.5f * minkowski_sum_dim;

              Vec3 rel = (entity->pos + entity_volume->offset_pos) - (test_entity->pos + test_volume->offset_pos);

              if ((rel.z >= min_corner.z) && (rel.z <= max_corner.z))
              {
                B32 hit_this = false;

                Test_Wall_t walls[] = {{min_corner.x, rel.x, rel.y, player_delta.x, player_delta.y, min_corner.y,
                                        max_corner.y, vec3(-1, 0, 0)},
                                       {max_corner.x, rel.x, rel.y, player_delta.x, player_delta.y, min_corner.y,
                                        max_corner.y, vec3(1, 0, 0)},
                                       {min_corner.y, rel.y, rel.x, player_delta.y, player_delta.x, min_corner.x,
                                        max_corner.x, vec3(0, -1, 0)},
                                       {max_corner.y, rel.y, rel.x, player_delta.y, player_delta.x, min_corner.x,
                                        max_corner.x, vec3(0, 1, 0)}};
                if (Has_Flag(test_entity, ENTITY_FLAG_TRAVERSABLE))
                {

                  F32 test_t_max = t_max;
                  Vec3 test_wall_normal = {};
                  for (U32 wall_index = 0; wall_index < Array_Count(walls); ++wall_index)
                  {
                    Test_Wall_t* wall = walls + wall_index;
                    F32 t_epsilon = 0.0001f;

                    if (wall->delta_x != 0.0f)
                    {
                      F32 t_result = (wall->x - wall->rel_x) / wall->delta_x;
                      F32 y = wall->rel_y + t_result * wall->delta_y;
                      if ((t_result >= 0.0f) && (test_t_max < t_result))
                      {
                        if ((y >= wall->min_y) && (y <= wall->max_y))
                        {
                          test_t_max = MAX(0.f, t_result - t_epsilon);
                          test_wall_normal = wall->normal;
                          hit_this = true;
                        }
                      }
                    }
                  }
                  if (hit_this)
                  {
                    t_max = test_t_max;
                    wall_normal_max = test_wall_normal;
                    hit_entity_max = test_entity;
                  }
                }
                else
                {
                  F32 test_t_min = t_min;
                  Vec3 test_wall_normal = {};
                  // NOTE: Test 4 wall outer collisions
                  for (U32 wall_index = 0; wall_index < Array_Count(walls); ++wall_index)
                  {
                    Test_Wall_t* wall = walls + wall_index;

                    F32 t_epsilon = 0.0001f;
                    if (wall->delta_x != 0.0f)
                    {
                      F32 t_result = (wall->x - wall->rel_x) / wall->delta_x;
                      F32 y = wall->rel_y + t_result * wall->delta_y;
                      if ((t_result >= 0.0f) && (test_t_min > t_result))
                      {
                        if ((y >= wall->min_y) && (y <= wall->max_y))
                        {
                          test_t_min = MAX(0.0f, t_result - t_epsilon);
                          test_wall_normal = wall->normal;
                          hit_this = true;
                        }
                      }
                    }
                  }
                  if (hit_this)
                  {
                    Vec3 test_pos = entity->pos + test_t_min * player_delta;
                    if (Speculative_Collide(entity, test_entity, test_pos))
                    {
                      t_min = test_t_min;
                      wall_normal_min = test_wall_normal;
                      hit_entity_min = test_entity;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
    F32 t_stop = 1.f;
    Sim_Entity* hit_entity = 0;
    Vec3 wall_normal = {};

    if (t_min < t_max)
    {
      t_stop = t_min;
      hit_entity = hit_entity_min;
      wall_normal = wall_normal_min;
    }
    else if (t_max != 0.f)
    {
      wall_normal = wall_normal_max;
      hit_entity = hit_entity_max;
      t_stop = t_max;
    }

    entity->pos += t_stop * player_delta;
    distance_remaining -= t_stop * player_delta_length;
    if (hit_entity)
    {
      player_delta = desired_pos - entity->pos;

      B32 stops_on_collision = Handle_Collision(game_state, entity, hit_entity);
      if (stops_on_collision)
      {
        ;
        entity->vel = Vec_Slide(entity->vel, wall_normal);
        player_delta = Vec_Slide(player_delta, wall_normal);
      }
    }
    else
    {
      break;
    }
  }

  F32 ground = 0;

  // NOTE: Handle events based on overlapping entities

  // TODO: Spatial partition here
  for (U32 test_entity_index = 0; test_entity_index < sim_region->entity_count; ++test_entity_index)
  {
    Sim_Entity* test_entity = sim_region->entities + test_entity_index;
    if (Should_Handle_Overlap(game_state, entity, test_entity))
    {
      if (Entities_Overlap(entity, test_entity))
      {
        Handle_Overlap(game_state, entity, test_entity, delta_time, &ground);
      }
    }
  }

  // NOTE: snaps to every multiple of the chunk floor -3, 0, 3, 6, etc.
  F32 chunk_floor = game_state->typical_floor_height * (F32)(entity->chunk_z - sim_region->origin.chunk_z);
  ground += chunk_floor;

  // NOTE: kill velocity when falling through ground
  if ((entity->pos.z <= ground) || (Has_Flag(entity, ENTITY_FLAG_Z_SUPPORTED) && entity->vel.z == 0.f))
  {
    entity->vel.z = 0.f;
    entity->pos.z = ground;
    Set_Flag(entity, ENTITY_FLAG_Z_SUPPORTED);
  }
  else
  {
    Clear_Flag(entity, ENTITY_FLAG_Z_SUPPORTED);
  }

  // NOTE: prevent falling through ground and jumping through ceiling

  if (entity->distance_limit != 0.f)
  {
    entity->distance_limit = distance_remaining;
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
