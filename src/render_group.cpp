#include "render_group.h"
#ifdef CLANGD
#include "hot.cpp"
#endif

internal void Draw_BMP_Subset(Loaded_Bitmap* buffer, Loaded_Bitmap* bitmap, F32 x, F32 y, S32 bmp_x_dim, S32 bmp_y_dim,
                              F32 scale, B32 alpha_blend, F32 c_alpha)
{
#if 1
  Draw_BMP_Subset_Hot(buffer, bitmap, x, y, bmp_x_dim, bmp_y_dim, scale, alpha_blend, c_alpha);
#else
  c_alpha = CLAMP(c_alpha, 0.f, 1.f);
  if (!bitmap || !bitmap->memory)
  {
    // TODO: Maybe draw pink checkerboard if no texture
    return;
  };
  ASSERT(scale > 0);

  S32 min_x = Round_F32_S32(x);
  S32 min_y = Round_F32_S32(y);
  S32 max_x = Round_F32_S32(x + (F32)bmp_x_dim * scale);
  S32 max_y = Round_F32_S32(y + (F32)bmp_y_dim * scale);
  S32 x_draw_offset = Round_F32_S32((F32)bmp_x_offset * scale);
  if (min_x < 0)
  {
    x_draw_offset += (-min_x) % Round_F32_S32(scale * (F32)bmp_x_dim);
  }
  S32 y_draw_offset = Round_F32_S32((F32)bmp_y_offset * scale);
  if (min_y < 0)
  {
    y_draw_offset += (-min_y) % Round_F32_S32(scale * (F32)bmp_y_dim);
  }
  min_x = CLAMP(min_x, 0, buffer->width);
  max_x = CLAMP(max_x, 0, buffer->width);

  min_y = CLAMP(min_y, 0, buffer->height);
  max_y = CLAMP(max_y, 0, buffer->height);

  U8* dest_row_in_bytes = (U8*)buffer->memory + (min_y * buffer->pitch_in_bytes) + (min_x * BITMAP_BYTES_PER_PIXEL);
  for (S32 y_index = min_y; y_index < max_y; y_index++)
  {
    U8* pixel = dest_row_in_bytes;
    S32 y_src_offset = Trunc_F32_S32((F32)(y_index - min_y + y_draw_offset) / scale);
    // NOTE: flip the bmp to render into buffer top to bottom
    S32 y_src = y_src_offset;
    // y_src = CLAMP(y_src, (bitmap->height - 1) - bmp_y_dim - bmp_y_offset, (bitmap->height - 1) - bmp_y_offset);
    y_src = CLAMP(y_src, bmp_y_offset, bmp_y_offset + bmp_y_dim - 1);
    ASSERT(y_src < bitmap->height);

    for (S32 x_index = min_x; x_index < max_x; x_index++)
    {
      S32 x_src = Trunc_F32_S32((F32)(x_index - min_x + x_draw_offset) / scale);
      x_src = CLAMP(x_src, bmp_x_offset, bmp_x_offset + bmp_x_dim - 1);
      ASSERT(x_src < bitmap->width);

      U8* src = (U8*)(void*)((U32*)bitmap->memory + y_src * (bitmap->pitch_in_bytes / BITMAP_BYTES_PER_PIXEL) + x_src);
      Color4 out = *(Color4*)(void*)src;

      if (alpha_blend)
      {
        Color4F dest = Color4_SRGB_To_Color4F_Linear(*(Color4*)(void*)pixel);
        Color4F texel = Color4_SRGB_To_Color4F_Linear(*(Color4*)(void*)src);
        texel *= c_alpha;

        Color4F result = (1.f - texel.a) * dest + texel;
        out = Color4F_Linear_To_Color4_SRGB(result);
        // out = Color4F_Linear_To_Color4_SRGB(Color4F_Blend_Normal(dest, texel, vec4(1, 1, 1, c_alpha)));
      }

      // Note: BMP may not be 4 byte aligned so assign byte by byte
      *pixel++ = out.argb.b; // B
      *pixel++ = out.argb.g; // G
      *pixel++ = out.argb.r; // R
      *pixel++ = out.argb.a; // A
    }
    dest_row_in_bytes += buffer->pitch_in_bytes;
  }

#endif
}

internal void Draw_Sprite_Sheet_Sprite(Loaded_Bitmap* buffer, Sprite_Sheet* sprite_sheet, U32 sprite_index, F32 x,
                                       F32 y, F32 scale, B32 alpha_blend = true, F32 c_alpha = 1.0f)
{
  Loaded_Bitmap sprite = sprite_sheet->sprite_bitmaps[sprite_index];

  Draw_BMP_Subset(buffer, &sprite, x, y, sprite.width, sprite.height, scale, alpha_blend, c_alpha);
}

internal void Draw_BMP(Loaded_Bitmap* buffer, Loaded_Bitmap* bitmap, F32 x, F32 y, F32 scale = 1.f,
                       B32 alpha_blend = true, F32 c_alpha = 1.0f)
{
  Draw_BMP_Subset(buffer, bitmap, x, y, bitmap->width, bitmap->height, scale, alpha_blend, c_alpha);
}

internal void Draw_Rect_Slowly(Loaded_Bitmap* buffer, Vec2 origin, Vec2 x_axis, Vec2 y_axis, Vec4 color,
                               Loaded_Bitmap* texture, Loaded_Bitmap* normal_map, Environment_Map* top_env_map,
                               Environment_Map* middle_env_map, Environment_Map* bottom_env_map, F32 pixels_to_meters)
{
#if 1
  Draw_Rect_Slowly_Hot(buffer, origin, x_axis, y_axis, color, texture, normal_map, top_env_map, middle_env_map,
                       bottom_env_map, pixels_to_meters);
#else

#endif
}

internal void Draw_Rectf(Loaded_Bitmap* buffer, F32 fmin_x, F32 fmin_y, F32 fmax_x, F32 fmax_y, Vec4 color)
{
#if 1
  Draw_Rectf_Hot(buffer, fmin_x, fmin_y, fmax_x, fmax_y, color);
#else
  fmin_x = CLAMP(fmin_x, 0, (F32)buffer->width);
  fmin_y = CLAMP(fmin_y, 0, (F32)buffer->width);
  fmax_x = CLAMP(fmax_x, 0, (F32)buffer->width);
  fmax_y = CLAMP(fmax_y, 0, (F32)buffer->width);

  S32 min_x = Round_F32_S32(fmin_x);
  S32 min_y = Round_F32_S32(fmin_y);
  S32 max_x = Round_F32_S32(fmax_x);
  S32 max_y = Round_F32_S32(fmax_y);

  min_x = CLAMP(min_x, 0, buffer->width);
  max_x = CLAMP(max_x, 0, buffer->width);

  min_y = CLAMP(min_y, 0, buffer->height);
  max_y = CLAMP(max_y, 0, buffer->height);

  // NOTE: premultiply alpha
  color = Color4F_SRGB_Premult(color);
  U32 out_color = Color4F_To_Color4(color).u32;

  U8* row_in_bytes = (U8*)buffer->memory + (min_y * buffer->pitch_in_bytes) + (min_x * BITMAP_BYTES_PER_PIXEL);

  for (S32 y = min_y; y < max_y; y++)
  {
    U32* pixel = (U32*)(void*)row_in_bytes;
    for (S32 x = min_x; x < max_x; x++)
    {
      *pixel++ = out_color;
    }
    row_in_bytes += buffer->pitch_in_bytes;
  }
#endif
}

internal void Draw_Rect(Loaded_Bitmap* buffer, Vec2 min, Vec2 max, Vec4 color)

{
  Draw_Rectf(buffer, min.x, min.y, max.x, max.y, color);
}

internal void Draw_Rect(Loaded_Bitmap* buffer, Rect2 rect, Vec4 color)

{
  Draw_Rectf(buffer, rect.min.x, rect.min.y, rect.max.x, rect.max.y, color);
}

internal void Draw_Rect_Outline(Loaded_Bitmap* buffer, Rect2 rect, Vec4 color, F32 pixel_thickness = 1.f)
{
  // HACK: to fix float rounding when straddling 0.5px boundary
  F32 epsilon = 0.0001f;
  rect.min += vec2(epsilon);
  rect.max += vec2(epsilon);
  F32 t = 0.5f * pixel_thickness;
  // TOP/BOTTOM
  Draw_Rectf(buffer, rect.min.x - t, rect.min.y - t, rect.max.x + t, rect.min.y + t, color);
  Draw_Rectf(buffer, rect.min.x - t, rect.max.y - t, rect.max.x + t, rect.max.y + t, color);

  // LEFT/RIGHT
  Draw_Rectf(buffer, rect.min.x - t, rect.min.y - t, rect.min.x + t, rect.max.y + t, color);
  Draw_Rectf(buffer, rect.max.x - t, rect.min.y - t, rect.max.x + t, rect.max.y + t, color);
}

internal Render_Group* Allocate_Render_Group(Arena* arena, U32 max_push_buffer_size, F32 meters_to_pixels,
                                             F32 sprite_scale)
{
  Render_Group* result = Push_Struct(arena, Render_Group);
  result->push_buffer_base = (U8*)Push_Size(arena, max_push_buffer_size);
  result->default_basis = Push_Struct(arena, Render_Basis);
  result->default_basis->pos = vec3(0);
  result->meters_to_pixels = meters_to_pixels;
  result->global_alpha = 1.f;

  result->max_push_buffer_size = max_push_buffer_size;
  result->push_buffer_size = 0;
  return result;
}

struct Entity_Basis_Result
{
  B32 valid;
  Vec2 pos;
  F32 scale;
};

internal Entity_Basis_Result Get_Render_Entity_Basis(Render_Group* render_group, Render_Group_Entry_Base* base,
                                                     Vec2 screen_center)
{
  // TODO: ZHANDLING
  Entity_Basis_Result result = {};

  F32 meters_to_pixels = render_group->meters_to_pixels;
  Vec3 entity_base_pos = meters_to_pixels * base->basis->pos;

  // F32 monitor_width = 1920.f;
  F32 focal_length = 14.f * meters_to_pixels;
  F32 camera_distance_above_ground = 14.f * meters_to_pixels;
  F32 distance_to_entity_z = (camera_distance_above_ground - entity_base_pos.z);
  F32 near_clip_plane = 0.2f * meters_to_pixels;

  Vec3 raw_xy = vec3(entity_base_pos.xy + base->offset.xy * base->scale, 1.f);

  if (distance_to_entity_z > near_clip_plane)
  {
    Vec3 projected_xy = (1.f / distance_to_entity_z) * focal_length * raw_xy;

    result.pos = screen_center + projected_xy.xy;

    // result.fudged_alpha = base->color.a * alpha_fudge;
    result.scale = base->scale * projected_xy.z;
    result.valid = true;
  }

  return result;
}

internal void Render_Group_To_Output(Render_Group* render_group, Loaded_Bitmap* draw_buffer)
{

  Vec2 screen_center = Vec2{{0.5f * (F32)draw_buffer->width, 0.5f * (F32)draw_buffer->height}};
  F32 pixels_to_meters = 1.f / render_group->meters_to_pixels;

  for (U32 base_address = 0; base_address < render_group->push_buffer_size;)
  {
    Render_Group_Entry_Header* header =
        (Render_Group_Entry_Header*)(void*)(render_group->push_buffer_base + base_address);
    base_address += sizeof(*header);
    void* untyped_entry = (U8*)header + sizeof(*header);
    switch (header->type)
    {
      case E_RENDER_GROUP_ENTRY_Render_Entry_Clear:
      {
        Render_Entry_Clear* entry = (Render_Entry_Clear*)(void*)untyped_entry;
        base_address += sizeof(*entry);

        Draw_Rectf(draw_buffer, 0, 0, (F32)draw_buffer->width, (F32)draw_buffer->height, entry->color);
      }
      break;
      case E_RENDER_GROUP_ENTRY_Render_Entry_Bitmap:
      {
        Render_Entry_Bitmap* entry = (Render_Entry_Bitmap*)(void*)untyped_entry;
        base_address += sizeof(*entry);
        Render_Group_Entry_Base* base = &entry->base;
        Entity_Basis_Result basis = Get_Render_Entity_Basis(render_group, base, screen_center);

        ASSERT(entry->bitmap);

        // Draw_BMP_Subset(draw_buffer, entry->bitmap, basis.pos.x, basis.pos.y, (S32)entry->bitmap->width,
        //                 (S32)entry->bitmap->height, basis.scale, true, basis.fudged_alpha);
        Vec2 x_axis = basis.scale * vec2i(entry->bitmap->width, 0);
        Vec2 y_axis = basis.scale * vec2i(0, entry->bitmap->height);
        Draw_Rect_Slowly(draw_buffer, basis.pos, x_axis, y_axis, base->color, entry->bitmap, NULL, NULL, NULL, NULL,
                         pixels_to_meters);
      }
      break;
      case E_RENDER_GROUP_ENTRY_Render_Entry_Rectangle:
      {
        Render_Entry_Rectangle* entry = (Render_Entry_Rectangle*)(void*)untyped_entry;
        base_address += sizeof(*entry);
        Render_Group_Entry_Base* base = &entry->base;
        Entity_Basis_Result basis = Get_Render_Entity_Basis(render_group, base, screen_center);

        Rect2 rect = Rect_Center_Dim(basis.pos, base->dim * basis.scale);
        Draw_Rect(draw_buffer, rect, base->color);
      }
      break;
      case E_RENDER_GROUP_ENTRY_Render_Entry_Rectangle_Outline:
      {
        Render_Entry_Rectangle_Outline* entry = (Render_Entry_Rectangle_Outline*)(void*)untyped_entry;
        base_address += sizeof(*entry);
        Render_Group_Entry_Base* base = &entry->base;
        Entity_Basis_Result basis = Get_Render_Entity_Basis(render_group, base, screen_center);

        Rect2 rect = Rect_Center_Dim(basis.pos, base->dim * basis.scale);
        Draw_Rect_Outline(draw_buffer, rect, base->color, entry->outline_pixel_thickness);
      }
      break;
      case E_RENDER_GROUP_ENTRY_Render_Entry_Coordinate_System:
      {
        Render_Entry_Coordinate_System* entry = (Render_Entry_Coordinate_System*)(void*)untyped_entry;
        base_address += sizeof(*entry);

        Draw_Rect_Slowly(draw_buffer, entry->origin, entry->x_axis, entry->y_axis, entry->color, entry->texture,
                         entry->normal_map, entry->top_env_map, entry->middle_env_map, entry->bottom_env_map,
                         pixels_to_meters);

        Vec2 dim = vec2(4);
        Vec2 pos = entry->origin;
        Vec4 color = vec4(1, 1, 0, 1);
        Draw_Rect(draw_buffer, pos, pos + dim, color);
        pos = entry->origin + entry->x_axis;
        Draw_Rect(draw_buffer, pos, pos + dim, color);
        pos = entry->origin + entry->y_axis;
        Draw_Rect(draw_buffer, pos, pos + dim, color);
        pos = entry->origin + entry->y_axis + entry->x_axis;
        Draw_Rect(draw_buffer, pos, pos + dim, color);
        // for (U32 point_index = 0; point_index < Array_Count(entry->points); ++point_index)
        // {
        //   Vec2 point = entry->points[point_index];
        //   pos = entry->origin + point.x * entry->x_axis + point.y * entry->y_axis;
        //   Draw_Rect(draw_buffer, pos, pos + dim, entry->color);
        // }
      }
      break;
        Invalid_Default_Case;
    }
  }
}

#define Push_Render_Element(group, type) (type*)Push_Render_Element_(group, sizeof(type), E_RENDER_GROUP_ENTRY_##type)

internal void* Push_Render_Element_(Render_Group* group, U32 size, E_Render_Group_Entry_Type type)
{
  void* result = 0;
  size += sizeof(Render_Group_Entry_Header);
  if ((group->push_buffer_size + size) < group->max_push_buffer_size)
  {
    Render_Group_Entry_Header* header =
        (Render_Group_Entry_Header*)(void*)(group->push_buffer_base + group->push_buffer_size);
    header->type = type;
    // NOTE: the bytes starting after the header
    result = (U8*)header + sizeof(*header);
    group->push_buffer_size += size;
  }
  else
  {
    Invalid_Code_Path;
  }

  return result;
}

internal inline void Push_Render_Clear(Render_Group* group, Vec4 color)
{
  Render_Entry_Clear* entry = Push_Render_Element(group, Render_Entry_Clear);
  if (entry)
  {
    entry->color = color;
  }
}

internal inline void Fill_Render_Base(Render_Group* group, Render_Group_Entry_Base* base, Vec2 dim, Vec3 offset,
                                      Vec2 align, F32 scale, Vec4 color)
{
  base->basis = group->default_basis;
  base->dim = dim * group->meters_to_pixels;
  base->offset = offset * group->meters_to_pixels - vec3(align, 0);

  base->scale = scale;
  color.a *= group->global_alpha;
  base->color = color;
}

internal inline void Push_Render_Rectangle(Render_Group* group, Vec2 dim, Vec3 offset, F32 scale, Vec4 color)

{
  Render_Entry_Rectangle* entry = Push_Render_Element(group, Render_Entry_Rectangle);
  if (entry)
  {
    Fill_Render_Base(group, &entry->base, dim, offset, vec2(0), scale, color);
  }
}

internal inline void Push_Render_Rectangle_Pixel_Outline(Render_Group* group, Vec2 dim, Vec3 offset, F32 scale,
                                                         Vec4 color, F32 outline_pixel_thickness = 1.f)

{
  Render_Entry_Rectangle_Outline* entry = Push_Render_Element(group, Render_Entry_Rectangle_Outline);
  if (entry)
  {
    Fill_Render_Base(group, &entry->base, dim, offset, vec2(0), scale, color);
    entry->outline_pixel_thickness = outline_pixel_thickness;
  }
}

internal inline void Push_Render_Bitmap(Render_Group* group, Loaded_Bitmap* bitmap, Vec3 offset, F32 scale,
                                        Vec4 color = vec4(1))

{
  Render_Entry_Bitmap* entry = Push_Render_Element(group, Render_Entry_Bitmap);
  if (entry)
  {

    Fill_Render_Base(group, &entry->base, vec2i(bitmap->width, bitmap->height), offset, bitmap->align, scale, color);
    entry->bitmap = bitmap;
  }
}

internal inline void Push_Render_Bitmap_Centered(Render_Group* group, Loaded_Bitmap* bitmap, Vec3 offset_meters,
                                                 F32 scale, F32 alpha = 1.f)
{
  F32 pixels_to_meters = 1.f / group->meters_to_pixels;
  offset_meters.xy += pixels_to_meters * 0.5f * vec2i(-bitmap->width, bitmap->height);
  Push_Render_Bitmap(group, bitmap, offset_meters, scale, vec4(1, 1, 1, alpha));
}

internal inline Render_Entry_Coordinate_System*
Render_Coordinate_System(Render_Group* group, Vec2 origin, Vec2 x_axis, Vec2 y_axis, Vec4 color, Loaded_Bitmap* texture,
                         Loaded_Bitmap* normal_map, Environment_Map* top_env_map, Environment_Map* middle_env_map,
                         Environment_Map* bottom_env_map)
{
  Render_Entry_Coordinate_System* entry = Push_Render_Element(group, Render_Entry_Coordinate_System);
  if (entry)
  {
    entry->origin = origin;
    entry->x_axis = x_axis;
    entry->y_axis = y_axis;
    entry->color = color;
    entry->texture = texture;
    entry->normal_map = normal_map;
    entry->top_env_map = top_env_map;
    entry->middle_env_map = middle_env_map;
    entry->bottom_env_map = bottom_env_map;
  }
  return entry;
}

internal inline void Push_Render_Rectangle_Outline(Render_Group* group, Vec2 dim, Vec3 offset, F32 scale, Vec4 color,
                                                   F32 thickness_in_meters = 0.1f)
{
  // HACK: to fix float rounding when straddling 0.5px boundary
  F32 epsilon = 0.0001f;
  Vec2 half_dim_y = vec2(0.f, .5f * dim.y);
  Vec2 half_dim_x = vec2(0.5f * dim.x, 0.f);

  Vec2 dim_horizontal = vec2(dim.x + thickness_in_meters, thickness_in_meters) + vec2(epsilon);
  Vec2 dim_vertical = vec2(thickness_in_meters, dim.y + thickness_in_meters) + vec2(epsilon);

  // TOP/BOTTOM
  Push_Render_Rectangle(group, dim_horizontal, offset + vec3(half_dim_y, 0), scale, color);
  Push_Render_Rectangle(group, dim_horizontal, offset - vec3(half_dim_y, 0), scale, color);

  // LEFT/RIGHT
  Push_Render_Rectangle(group, dim_vertical, offset - vec3(half_dim_x, 0), scale, color);
  Push_Render_Rectangle(group, dim_vertical, offset + vec3(half_dim_x, 0), scale, color);
}

internal void Render_Inputs(Render_Group* render_group, Game_Input* input)
{

  F32 size = 0.24f;
  Vec2 dim = vec2(size);
  for (S32 i = 0; i < (S32)Array_Count(input->mouse_buttons); i++)
  {
    if (input->mouse_buttons[i].ended_down)
    {
      Vec3 offset = vec3((F32)i * (2.f * size) + size, -size, 0);
      Push_Render_Rectangle(render_group, dim, offset, 1.f, vec4(1, 0, 0, 1));
    }
  }
  if (input->mouse_z > 0)
  {
    Vec3 pixel_align = size * vec3(1.f, -3.f, 0);
    Push_Render_Rectangle(render_group, dim, pixel_align, 1.f, vec4(1, 0, 0, 1));
  }
  else if (input->mouse_z < 0)
  {
    Vec3 pixel_align = size * vec3(3.f, -3.f, 0);
    Push_Render_Rectangle(render_group, dim, pixel_align, 1.f, vec4(1, 0, 0, 1));
  }

  Vec2 mouse_dim = vec2(0.1f);
  Vec3 mouse_pixel_alignment = vec3i(input->mouse_x, -input->mouse_y, 0) + 0.5f * vec3(mouse_dim.x, -mouse_dim.y, 0);
  Push_Render_Rectangle_Pixel_Outline(render_group, mouse_dim, mouse_pixel_alignment, 1.f, vec4(1, 1, 0, 1));
}
