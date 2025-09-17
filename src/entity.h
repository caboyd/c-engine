#ifndef ENTITY_H
#define ENTITY_H

#define Invalid_Pos vec2(10000.f, 10000.f);

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

inline void Make_Entity_Spatial(Sim_Entity* entity, Vec2 pos, Vec2 vel)
{
  Clear_Flag(entity, ENTITY_FLAG_NONSPATIAL);
  entity->pos = pos;
  entity->vel = vel;
}
#endif // ENTITY_H
