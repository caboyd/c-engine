
#ifdef CLANGD
#include "render_group.h"
#include "hot.h"
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

internal void Draw_Rectf(Loaded_Bitmap* buffer, F32 fmin_x, F32 fmin_y, F32 fmax_x, F32 fmax_y, Vec4 color,
                         Rect2i clip_rect)
{
#if 1
  Draw_Rectf_Hot(buffer, fmin_x, fmin_y, fmax_x, fmax_y, color, clip_rect);
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

internal void Draw_Rect(Loaded_Bitmap* buffer, Vec2 min, Vec2 max, Vec4 color, Rect2i clip_rect)

{
  Draw_Rectf(buffer, min.x, min.y, max.x, max.y, color, clip_rect);
}

internal void Draw_Rect(Loaded_Bitmap* buffer, Rect2 rect, Vec4 color, Rect2i clip_rect)

{
  Draw_Rectf(buffer, rect.min.x, rect.min.y, rect.max.x, rect.max.y, color, clip_rect);
}

internal void Draw_Rect_Outline(Loaded_Bitmap* buffer, Rect2 rect, Vec4 color, Rect2i clip_rect,
                                F32 pixel_thickness = 1.f)
{
  // HACK: to fix float rounding when straddling 0.5px boundary
  F32 epsilon = 0.0001f;
  rect.min += vec2(epsilon);
  rect.max += vec2(epsilon);
  F32 t = 0.5f * pixel_thickness;
  // TOP/BOTTOM
  Draw_Rectf(buffer, rect.min.x - t, rect.min.y - t, rect.max.x + t, rect.min.y + t, color, clip_rect);
  Draw_Rectf(buffer, rect.min.x - t, rect.max.y - t, rect.max.x + t, rect.max.y + t, color, clip_rect);

  // LEFT/RIGHT
  Draw_Rectf(buffer, rect.min.x - t, rect.min.y - t, rect.min.x + t, rect.max.y + t, color, clip_rect);
  Draw_Rectf(buffer, rect.max.x - t, rect.min.y - t, rect.max.x + t, rect.max.y + t, color, clip_rect);
}

internal Render_Group* Allocate_Render_Group(Arena* arena, U32 max_push_buffer_size, F32 resolution_pixels_x,
                                             F32 resolution_pixels_y)
{
  Render_Group* result = Push_Struct(arena, Render_Group);
  result->push_buffer_base = (U8*)Push_Size(arena, max_push_buffer_size);

  result->max_push_buffer_size = max_push_buffer_size;
  result->push_buffer_size = 0;

  result->global_alpha = 1.f;

  F32 monitor_width_meters = 0.635f; // 25 inches
  F32 meters_to_pixels = resolution_pixels_x * monitor_width_meters;
  F32 pixels_to_meters = Safe_Ratio1(1.f, meters_to_pixels);
  Vec2 screen_center = 0.5f * vec2(resolution_pixels_x, resolution_pixels_y);
  result->monitor_half_dim_meters = pixels_to_meters * screen_center;

  // NOTE: Default Transform
  result->transform.focal_length = 0.6f; // meters person sitting from monitor
  result->transform.distance_above_target = 12.f;
  result->transform.screen_center = screen_center;
  result->transform.meters_to_pixels = meters_to_pixels;
  result->transform.offset_pos = {};
  result->transform.scale = vec2(1);

  return result;
}

internal void Render_Group_To_Output(Render_Group* render_group, Loaded_Bitmap* draw_buffer, Rect2i clip_rect)
{
  BEGIN_TIMED_BLOCK(Render_Group_To_Output);

  F32 null_pixels_to_meters = 1.f;

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

        Draw_Rectf(draw_buffer, 0, 0, (F32)draw_buffer->width, (F32)draw_buffer->height, entry->color, clip_rect);
      }
      break;
      case E_RENDER_GROUP_ENTRY_Render_Entry_Bitmap:
      {
        Render_Entry_Bitmap* entry = (Render_Entry_Bitmap*)(void*)untyped_entry;
        base_address += sizeof(*entry);
        if (entry->bitmap->align_percentage.x > 1.f)
        {
          ASSERT(true);
        }

        ASSERT(entry->bitmap);
        // Draw_BMP_Subset(draw_buffer, entry->bitmap, basis.pos.x, basis.pos.y, (S32)entry->bitmap->width,
        //                 (S32)entry->bitmap->height, basis.scale, true, basis.fudged_alpha);
        Vec2 x_axis = vec2(entry->size.x, 0);
        Vec2 y_axis = vec2(0, entry->size.y);
        Draw_Rect_Quickly_Hot256(draw_buffer, entry->pos, x_axis, y_axis, entry->color, entry->bitmap,
                                 null_pixels_to_meters, clip_rect);
      }
      break;
      case E_RENDER_GROUP_ENTRY_Render_Entry_Rectangle:
      {
        Render_Entry_Rectangle* entry = (Render_Entry_Rectangle*)(void*)untyped_entry;
        base_address += sizeof(*entry);

        Rect2 rect = Rect_Center_Dim(entry->pos, entry->size);
        Draw_Rect(draw_buffer, rect, entry->color, clip_rect);
      }
      break;
      case E_RENDER_GROUP_ENTRY_Render_Entry_Rectangle_Outline:
      {
        Render_Entry_Rectangle_Outline* entry = (Render_Entry_Rectangle_Outline*)(void*)untyped_entry;
        base_address += sizeof(*entry);

        Rect2 rect = Rect_Center_Dim(entry->pos, entry->size);
        Draw_Rect_Outline(draw_buffer, rect, entry->color, clip_rect, entry->outline_pixel_thickness);
      }
      break;
      case E_RENDER_GROUP_ENTRY_Render_Entry_Coordinate_System:
      {
        Render_Entry_Coordinate_System* entry = (Render_Entry_Coordinate_System*)(void*)untyped_entry;
        base_address += sizeof(*entry);
#if 0
        Draw_Rect_Slowly(draw_buffer, entry->origin, entry->x_axis, entry->y_axis, entry->color, entry->texture,
                         entry->normal_map, entry->top_env_map, entry->middle_env_map, entry->bottom_env_map,
                         pixels_to_meters);

        Vec2 dim = vec2(4);
        Vec2 pos = entry->origin;
        Vec4 color = vec4(1, 1, 0, 1);
        Draw_Rect(draw_buffer, pos, pos + dim, color, clip_rect);
        pos = entry->origin + entry->x_axis;
        Draw_Rect(draw_buffer, pos, pos + dim, color, clip_rect);
        pos = entry->origin + entry->y_axis;
        Draw_Rect(draw_buffer, pos, pos + dim, color, clip_rect);
        pos = entry->origin + entry->y_axis + entry->x_axis;
        Draw_Rect(draw_buffer, pos, pos + dim, color, clip_rect);
        for (U32 point_index = 0; point_index < Array_Count(entry->points); ++point_index)
        {
          Vec2 point = entry->points[point_index];
          pos = entry->origin + point.x * entry->x_axis + point.y * entry->y_axis;
          Draw_Rect(draw_buffer, pos, pos + dim, entry->color, clip_rect);
        }
#endif
      }
      break;
        Invalid_Default_Case;
    }
  }
  END_TIMED_BLOCK(Render_Group_To_Output);
}

struct Tile_Render_Work
{
  Render_Group* render_group;
  Loaded_Bitmap* output_target;
  Rect2i clip_rect;
};

internal PLATFORM_WORK_QUEUE_CALLBACK_FUNC(Do_Tiled_Render_Work)
{
  Tile_Render_Work* work = (Tile_Render_Work*)data;
  Render_Group_To_Output(work->render_group, work->output_target, work->clip_rect);
}

internal void Tiled_Render_Group_To_Output(Platform_Work_Queue* render_queue, Render_Group* render_group,
                                           Loaded_Bitmap* output_target)
{
  const S32 tile_count_x = 4;
  const S32 tile_count_y = 8;
  Tile_Render_Work work_arr[tile_count_x * tile_count_y];

  // NOTE: align tile width to SIMD 8 wide
  S32 tile_width = ((output_target->width + tile_count_x - 1) / tile_count_x);
  tile_width = (tile_width + 7) & ~7;

  S32 tile_height = output_target->height / tile_count_y;

  S32 work_count = 0;
  for (S32 tile_y = 0; tile_y < tile_count_y; ++tile_y)
  {
    for (S32 tile_x = 0; tile_x < tile_count_x; ++tile_x)
    {
      Tile_Render_Work* work = work_arr + work_count++;
      Rect2i clip_rect;

      clip_rect.min_x = tile_x * tile_width;
      clip_rect.max_x = clip_rect.min_x + tile_width;
      clip_rect.min_y = tile_y * tile_height;
      clip_rect.max_y = clip_rect.min_y + tile_height;
      if (tile_x == tile_count_x - 1)
      {
        clip_rect.max_x = output_target->width;
      }
      if (tile_y == tile_count_y - 1)
      {
        clip_rect.max_y = output_target->height;
      }
      work->render_group = render_group;
      work->clip_rect = clip_rect;
      work->output_target = output_target;

      Platform_Add_Entry(render_queue, Do_Tiled_Render_Work, work);
    }
  }
  Platform_Complete_All_Work(render_queue);
}

struct Entity_Basis_Result
{
  B32 valid;
  Vec2 pos;
  F32 scale;
};

internal Entity_Basis_Result Get_Render_Entity_Basis(Render_Transform* transform, Vec3 original_pos)
{
  Entity_Basis_Result result = {};

  Vec3 pos = vec3(original_pos.xy, 0) + transform->offset_pos;

  F32 distance_above_target = transform->distance_above_target;
#if 0
  //TODO: How do we want to control debug camera
  if (1)
  {
    distance_above_target += 30.f;
  }
#endif
  F32 distance_to_entity_z = (distance_above_target - pos.z);
  F32 near_clip_plane = 0.2f;

  Vec3 raw_xy = vec3(pos.xy, 1.f);
  if (distance_to_entity_z > near_clip_plane)
  {
    Vec3 projected_xy = (1.f / distance_to_entity_z) * transform->focal_length * raw_xy;

    result.scale = transform->meters_to_pixels * projected_xy.z;
    result.pos = transform->screen_center + transform->meters_to_pixels * projected_xy.xy +
                 result.scale * vec2(0, original_pos.z);

    result.valid = true;
  }

  return result;
}

#define Push_Render_Element(group, type) (type*)Push_Render_Element_(group, sizeof(type), E_RENDER_GROUP_ENTRY_##type)

internal void* Push_Render_Element_(Render_Group* group, U32 size, E_Render_Group_Entry_Type type)
{
  void* result = 0;
  size += sizeof(Render_Group_Entry_Header);
  if (size & 7)
  {
    ASSERT(true);
  }
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

internal inline void Push_Render_Rectangle(Render_Group* group, Vec2 dim, Vec3 pos, Vec4 color)
{
  Entity_Basis_Result basis = Get_Render_Entity_Basis(&group->transform, pos);
  if (basis.valid)
  {
    Render_Entry_Rectangle* entry = Push_Render_Element(group, Render_Entry_Rectangle);
    if (entry)
    {
      entry->size = dim * basis.scale;
      entry->pos = basis.pos;
      color.a *= group->global_alpha;
      entry->color = color;
    }
  }
}

internal inline void Push_Render_Rectangle_Pixel_Outline(Render_Group* group, Vec2 dim, Vec3 pos, Vec4 color,
                                                         F32 outline_pixel_thickness = 1.f)
{
  Entity_Basis_Result basis = Get_Render_Entity_Basis(&group->transform, pos);
  if (basis.valid)
  {
    Render_Entry_Rectangle_Outline* entry = Push_Render_Element(group, Render_Entry_Rectangle_Outline);
    if (entry)
    {
      entry->size = dim * basis.scale;
      entry->pos = basis.pos;
      color.a *= group->global_alpha;
      entry->color = color;
      entry->outline_pixel_thickness = outline_pixel_thickness;
    }
  }
}

internal inline void Push_Render_Bitmap(Render_Group* group, Loaded_Bitmap* bitmap, Vec3 offset, F32 height,
                                        Vec4 color = vec4(1))
{
  Vec2 size = vec2(height * bitmap->width_over_height, height);
  Vec2 align = bitmap->align_percentage * size;
  Vec3 pos = offset - vec3(align, 0);

  Entity_Basis_Result basis = Get_Render_Entity_Basis(&group->transform, pos);
  if (basis.valid)
  {
    Render_Entry_Bitmap* entry = Push_Render_Element(group, Render_Entry_Bitmap);
    if (entry)
    {
      entry->bitmap = bitmap;
      entry->pos = basis.pos;
      entry->size = size * basis.scale;
      color.a *= group->global_alpha;
      entry->color = color;
    }
  }
}

internal inline Render_Entry_Coordinate_System*
Render_Coordinate_System(Render_Group* group, Vec2 origin, Vec2 x_axis, Vec2 y_axis, Vec4 color, Loaded_Bitmap* texture,
                         Loaded_Bitmap* normal_map, Environment_Map* top_env_map, Environment_Map* middle_env_map,
                         Environment_Map* bottom_env_map)
{
  Render_Entry_Coordinate_System* entry = 0;
  Entity_Basis_Result basis = Get_Render_Entity_Basis(&group->transform, vec3(origin, 0));
  if (basis.valid)
  {
    entry = Push_Render_Element(group, Render_Entry_Coordinate_System);
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
  }
  return entry;
}

internal inline void Push_Render_Rectangle_Outline(Render_Group* group, Vec2 dim, Vec3 offset, Vec4 color,
                                                   F32 thickness_in_meters = 0.1f)
{
  // HACK: to fix float rounding when straddling 0.5px boundary
  F32 epsilon = 0.0001f;
  Vec2 half_dim_y = vec2(0.f, .5f * dim.y);
  Vec2 half_dim_x = vec2(0.5f * dim.x, 0.f);

  Vec2 dim_horizontal = vec2(dim.x + thickness_in_meters, thickness_in_meters) + vec2(epsilon);
  Vec2 dim_vertical = vec2(thickness_in_meters, dim.y + thickness_in_meters) + vec2(epsilon);

  // TOP/BOTTOM
  Push_Render_Rectangle(group, dim_horizontal, offset + vec3(half_dim_y, 0), color);
  Push_Render_Rectangle(group, dim_horizontal, offset - vec3(half_dim_y, 0), color);

  // LEFT/RIGHT
  Push_Render_Rectangle(group, dim_vertical, offset - vec3(half_dim_x, 0), color);
  Push_Render_Rectangle(group, dim_vertical, offset + vec3(half_dim_x, 0), color);
}

inline Vec2 Unproject(Render_Group* render_group, Vec2 projected_xy, F32 at_distance_from_camera)
{
  Vec2 world_xy = (at_distance_from_camera / render_group->transform.focal_length) * projected_xy;

  return world_xy;
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
      Push_Render_Rectangle(render_group, dim, offset, vec4(1, 0, 0, 1));
    }
  }
  if (input->mouse_z > 0)
  {
    Vec3 offset = size * vec3(1.f, -3.f, 0);
    Push_Render_Rectangle(render_group, dim, offset, vec4(1, 0, 0, 1));
  }
  else if (input->mouse_z < 0)
  {
    Vec3 offset = size * vec3(3.f, -3.f, 0);
    Push_Render_Rectangle(render_group, dim, offset, vec4(1, 0, 0, 1));
  }

  Vec2 mouse_dim = vec2(0.1f);

  Vec2 mouse_offset_world = vec2i(input->mouse_x, -input->mouse_y) * (1.f / render_group->transform.meters_to_pixels);
  mouse_offset_world = Unproject(render_group, mouse_offset_world, render_group->transform.distance_above_target);
  mouse_offset_world += 0.5f * vec2(mouse_dim.x, -mouse_dim.y);

  Push_Render_Rectangle_Pixel_Outline(render_group, mouse_dim, vec3(mouse_offset_world, 0), vec4(1, 1, 0, 1));
}

inline Rect2 Get_Camera_Rect_At_Distance(Render_Group* render_group, F32 distance_from_camera)
{
  Vec2 raw_xy = Unproject(render_group, render_group->monitor_half_dim_meters, distance_from_camera);

  Rect2 result = Rect_Center_HalfDim(vec2(0), raw_xy);

  return result;
}

inline Rect2 Get_Camera_Rect_At_Target(Render_Group* render_group)
{
  Rect2 result = Get_Camera_Rect_At_Distance(render_group, render_group->transform.distance_above_target);
  return result;
}
