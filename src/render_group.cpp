#include "render_group.h"

internal Render_Group* Allocate_Render_Group(Arena* arena, U32 max_push_buffer_size, F32 meters_to_pixels,
                                             F32 sprite_scale)
{
  Render_Group* result = Push_Struct(arena, Render_Group);
  result->push_buffer_base = (U8*)Push_Size(arena, max_push_buffer_size);
  result->default_basis = Push_Struct(arena, Render_Basis);
  result->default_basis->pos = vec3(0);
  result->meters_to_pixels = meters_to_pixels;
  result->sprite_scale = sprite_scale;
  result->count = 0;

  result->max_push_buffer_size = max_push_buffer_size;
  result->push_buffer_size = 0;
  return result;
}
