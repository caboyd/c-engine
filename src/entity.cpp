#ifdef CLANGD
#include "sim_region.cpp"
#endif

inline Move_Spec Default_Move_Spec()
{
  Move_Spec result;

  result.normalize_accel = false;
  result.speed = 1.f;
  result.drag = 0.f;

  return result;
}

internal void Update_Familiar(Sim_Region* sim_region, Sim_Entity* entity, F32 delta_time)
{
  Sim_Entity* closest_player = {};
  F32 closest_player_distsq = Square(10.f); // NOTE: 10m max search

  // TODO: make spatial queries easy for things!
  Sim_Entity* test_entity = sim_region->entities;
  for (U32 entity_index = 0; entity_index < sim_region->entity_count; ++entity_index, ++test_entity)

  {
    if (test_entity->type == ENTITY_TYPE_PLAYER)
    {
      F32 test_distsq = Vec2_Length_Sq(test_entity->pos - entity->pos);
      if (test_distsq < closest_player_distsq)
      {
        closest_player_distsq = test_distsq;
        closest_player = test_entity;
      }
    }
  }

  Vec2 accel = {};
  // NOTE: follow player
  if (closest_player && (closest_player_distsq > Square(1.5f)))
  {
    accel = (closest_player->pos - entity->pos);
  }
  Move_Spec move_spec = Default_Move_Spec();
  move_spec.normalize_accel = true;
  move_spec.drag = 2.f;
  move_spec.speed = 6.f;

  Move_Entity(sim_region, entity, delta_time, &move_spec, accel);
}

internal void Update_Monster(Sim_Region* sim_region, Sim_Entity* entity, F32 delta_time) {}

internal void Update_Sword(Sim_Region* sim_region, Sim_Entity* entity, F32 delta_time)
{
  if (Has_Flag(entity, ENTITY_FLAG_NONSPATIAL))
  {
  }
  else
  {
    Move_Spec move_spec = Default_Move_Spec();

    Vec2 old_pos = entity->pos;
    Move_Entity(sim_region, entity, delta_time, &move_spec, vec2(0, 0));
    F32 distance_travelled = Vec2_Length(entity->pos - old_pos);

    // TODO: Need to handle case where distance remaninng is less than
    // distance travelled causing it to move extra
    entity->sword_distance_remaining -= distance_travelled;
    if (entity->sword_distance_remaining < 0.f)
    {
      Make_Entity_Nonspatial(entity);
    }
  }
}
