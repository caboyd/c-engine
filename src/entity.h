#ifndef ENTITY_H
#define ENTITY_H

#define Invalid_Pos vec3(100000.f, 100000.f, 100000.f);

inline void Set_Flag(Sim_Entity* entity, U32 flag)
{
  entity->flags = (Sim_Entity_Flags)((U32)entity->flags | flag);
}

inline B32 Has_Flag(Sim_Entity* entity, U32 flag)
{
  B32 result = (B32)((U32)entity->flags & flag);
  return result;
}

inline void Clear_Flag(Sim_Entity* entity, U32 flag)
{
  entity->flags = (Sim_Entity_Flags)((U32)entity->flags & ~flag);
}

inline void Make_Entity_Nonspatial(Sim_Entity* entity)
{
  Set_Flag(entity, ENTITY_FLAG_NONSPATIAL);
  entity->pos = Invalid_Pos;
}

inline void Make_Entity_Spatial(Sim_Entity* entity, Vec3 pos, Vec3 vel)
{
  Clear_Flag(entity, ENTITY_FLAG_NONSPATIAL);
  entity->pos = pos;
  entity->vel = vel;
}
inline Vec3 Get_Entity_Ground_Point(Sim_Entity* entity)
{
  Vec3 result = entity->pos + vec3(0, 0, -0.5f * entity->dim.z);

  return result;
}

inline F32 Get_Stair_Ground(Sim_Entity* entity, Vec3 at_ground_point)
{
  ASSERT(entity->type == ENTITY_TYPE_STAIR);

  Rect3 region_rect = Rect_Center_Dim(entity->pos, entity->dim);
  Vec3 bary = Vec_Clamp01(Rect_Get_Barycentric_Coord(region_rect, at_ground_point));
  F32 result = region_rect.min.z + bary.y * entity->walkable_height;

  return result;
}
#endif // ENTITY_H
