#include "render_group.h"
#ifdef CLANGD
#include "hot.cpp"
#endif

internal void Draw_BMP_Subset(Loaded_Bitmap* buffer, Loaded_Bitmap* bitmap, F32 x, F32 y, S32 bmp_x_offset,
                              S32 bmp_y_offset, S32 bmp_x_dim, S32 bmp_y_dim, F32 scale, B32 alpha_blend, F32 c_alpha)
{
#if 1
  Draw_BMP_Subset_Hot(buffer, bitmap, x, y, bmp_x_offset, bmp_y_offset, bmp_x_dim, bmp_y_dim, scale, alpha_blend,
                      c_alpha);
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
        out = blend_normal_Color4(*(Color4*)(void*)pixel, *(Color4*)(void*)src, c_alpha);
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
  Sprite sprite = sprite_sheet->sprites[sprite_index];

  Draw_BMP_Subset(buffer, &sprite_sheet->bitmap, x, y, sprite.x, sprite.y, sprite.width, sprite.height, scale,
                  alpha_blend, c_alpha);
}

internal void Draw_BMP(Loaded_Bitmap* buffer, Loaded_Bitmap* bitmap, F32 x, F32 y, F32 scale = 1.f,
                       B32 alpha_blend = true, F32 c_alpha = 1.0f)
{
  Draw_BMP_Subset(buffer, bitmap, x, y, 0, 0, bitmap->width, bitmap->height, scale, alpha_blend, c_alpha);
}

internal void Draw_Rect_Slowly(Loaded_Bitmap* buffer, Vec2 origin, Vec2 x_axis, Vec2 y_axis, Vec4 color,
                               Loaded_Bitmap* texture)
{
  S32 max_width = buffer->width - 1;
  S32 max_height = buffer->height - 1;
  F32 inv_x_axis_length_sq = 1.f / Vec_Length_Sq(x_axis);
  F32 inv_y_axis_length_sq = 1.f / Vec_Length_Sq(y_axis);

  Vec2 points[4] = {origin, origin + x_axis, origin + y_axis, origin + x_axis + y_axis};

  S32 x_min = max_width;
  S32 y_min = max_height;
  S32 x_max = 0;
  S32 y_max = 0;

  for (U32 point_index = 0; point_index < Array_Count(points); ++point_index)
  {
    Vec2 p = points[point_index];
    S32 floor_x = Floor_F32_S32(p.x);
    S32 ceil_x = Ceil_F32_S32(p.x);
    S32 floor_y = Floor_F32_S32(p.y);
    S32 ceil_y = Ceil_F32_S32(p.y);
    // clang-format off
    if(x_min > floor_x) {x_min = floor_x;}
    if(y_min > floor_y) {y_min = floor_y;}
    if(x_max < ceil_x) {x_max = ceil_x;}
    if(y_max < ceil_y) {y_max = ceil_y;}
    // clang-format on
  }

  Color4 out_color = Color4F_To_Color4(color);

  U8* row_in_bytes = (U8*)buffer->memory + (y_min * buffer->pitch_in_bytes) + (x_min * BITMAP_BYTES_PER_PIXEL);

  for (S32 y = y_min; y <= y_max; y++)
  {
    U32* pixel = (U32*)(void*)row_in_bytes;
    for (S32 x = x_min; x <= x_max; x++)
    {
      Vec2 pixel_pos = vec2i(x, y);
      // TODO: perp dot
      Vec2 d = pixel_pos - origin;
      F32 edge_0 = Vec_Dot(d, -Vec_Perp(x_axis));
      F32 edge_1 = Vec_Dot(d - x_axis, -Vec_Perp(y_axis));
      F32 edge_2 = Vec_Dot(d - x_axis - y_axis, Vec_Perp(x_axis));
      F32 edge_3 = Vec_Dot(d - y_axis, Vec_Perp(y_axis));

      if ((edge_0 < 0) && (edge_1 < 0) && (edge_2 < 0) && (edge_3 < 0))
      {
        F32 u = Clamp01(inv_x_axis_length_sq * Vec_Dot(d, x_axis));
        F32 v = Clamp01(inv_y_axis_length_sq * Vec_Dot(d, y_axis));

        ASSERT(u >= 0.f && u <= 1.f);
        ASSERT(v >= 0.f && v <= 1.f);

        // NOTE: sample texel center
        // TODO: Formalize textures boundaries
        F32 texel_u = (u * ((F32)texture->width - 2.f));
        F32 texel_v = (v * ((F32)texture->height - 2.f));
        S32 texture_u = Trunc_F32_S32(texel_u);
        S32 texture_v = Trunc_F32_S32(texel_v);

        F32 fu = texel_u - (F32)texture_u;
        F32 fv = texel_v - (F32)texture_v;

        ASSERT(texture_u >= 0 && texture_u < texture->width);
        ASSERT(texture_v >= 0 && texture_v < texture->height);

        U8* texel_ptr =
            ((U8*)texture->memory + (texture_v * texture->pitch_in_bytes) + (texture_u * BITMAP_BYTES_PER_PIXEL));

        Color4F texel_a = Color4_To_Color4F(*(Color4*)(void*)(texel_ptr));
        Color4F texel_b = Color4_To_Color4F(*(Color4*)(void*)(texel_ptr + sizeof(U32)));
        Color4F texel_c = Color4_To_Color4F(*(Color4*)(void*)(texel_ptr + texture->pitch_in_bytes));
        Color4F texel_d = Color4_To_Color4F(*(Color4*)(void*)(texel_ptr + texture->pitch_in_bytes + sizeof(U32)));

        Color4F texel_ab = Vec_Lerp(texel_a, texel_b, fu);
        Color4F texel_cd = Vec_Lerp(texel_c, texel_d, fu);
        Color4F texel = Vec_Lerp(texel_ab, texel_cd, fv);

        Color4F dest_color = Color4_To_Color4F(*(Color4*)(void*)pixel);
        Color4F out = Color4F_Blend_Normal(dest_color, texel);

        *pixel = Color4F_To_Color4(out).u32;
      }
      else
      {
        *pixel = out_color.u32;
      }

      pixel++;
    }
    row_in_bytes += buffer->pitch_in_bytes;
  }
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

  U32 out_color = (F32_to_U32_255(color.a) << 24) | (F32_to_U32_255(color.r) << 16) | (F32_to_U32_255(color.g) << 8) |
                  (F32_to_U32_255(color.b) << 0);

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
  result->sprite_scale = sprite_scale;

  result->max_push_buffer_size = max_push_buffer_size;
  result->push_buffer_size = 0;
  return result;
}

struct Entity_Render_Data
{
  Vec2 pos;
  F32 fudged_alpha;
  F32 fudged_scale;
};

internal Entity_Render_Data Get_Entity_Render_Data(Render_Group* render_group, Render_Group_Entry_Base* base,
                                                   Vec2 screen_center)
{
  Entity_Render_Data result;

  F32 meters_to_pixels = render_group->meters_to_pixels;
  Vec3 entity_base_pos = base->basis->pos;
  F32 z_fudge = (1.f + 0.04f * base->entity_cz * (entity_base_pos.z + base->offset_z));
  z_fudge = MAX(0.5f, z_fudge);

  Vec2 entity_pos = vec2(screen_center.x + meters_to_pixels * entity_base_pos.x * z_fudge,
                         screen_center.y - meters_to_pixels * entity_base_pos.y * z_fudge);

  F32 z = -meters_to_pixels * entity_base_pos.z;
  F32 x = entity_pos.x + base->offset.x * base->scale;
  F32 y = entity_pos.y + (base->offset.y) * base->scale + (base->entity_cz * z);

  F32 alpha_fudge = CLAMP(1.f + (1.f - entity_base_pos.z), 0.55f, 1.f);

  result.pos = vec2(x, y);
  result.fudged_alpha = base->color.a * alpha_fudge;
  result.fudged_scale = base->scale * z_fudge;
  return result;
}

internal void Render_Group_To_Output(Render_Group* render_group, Loaded_Bitmap* draw_buffer)
{

  Vec2 screen_center = Vec2{{0.5f * (F32)draw_buffer->width, 0.5f * (F32)draw_buffer->height}};
  for (U32 base_address = 0; base_address < render_group->push_buffer_size;)
  {
    Render_Group_Entry_Header* header =
        (Render_Group_Entry_Header*)(void*)(render_group->push_buffer_base + base_address);
    switch (header->type)
    {
      case E_RENDER_GROUP_ENTRY_Render_Entry_Clear:
      {
        Render_Entry_Clear* entry = (Render_Entry_Clear*)(void*)header;
        base_address += sizeof(*entry);

        Draw_Rectf(draw_buffer, 0, 0, (F32)draw_buffer->width, (F32)draw_buffer->height, entry->color);
      }
      break;
      case E_RENDER_GROUP_ENTRY_Render_Entry_Bitmap:
      {
        Render_Entry_Bitmap* entry = (Render_Entry_Bitmap*)(void*)header;
        base_address += sizeof(*entry);
        if ((size_t)header == 0x20005040c88)
        {
          ASSERT(true);
        }
        Render_Group_Entry_Base* base = &entry->base;
        Entity_Render_Data data = Get_Entity_Render_Data(render_group, base, screen_center);

        ASSERT(entry->bitmap);

        Draw_BMP_Subset(draw_buffer, entry->bitmap, data.pos.x, data.pos.y, entry->bmp_offset_x, entry->bmp_offset_y,
                        (S32)base->dim.x, (S32)base->dim.y, data.fudged_scale, true, data.fudged_alpha);
      }
      break;
      case E_RENDER_GROUP_ENTRY_Render_Entry_Rectangle:
      {
        Render_Entry_Rectangle* entry = (Render_Entry_Rectangle*)(void*)header;
        base_address += sizeof(*entry);
        Render_Group_Entry_Base* base = &entry->base;
        Entity_Render_Data data = Get_Entity_Render_Data(render_group, base, screen_center);

        Rect2 rect = Rect_Center_Dim(data.pos, base->dim * data.fudged_scale);
        Draw_Rect(draw_buffer, rect, base->color);
      }
      break;
      case E_RENDER_GROUP_ENTRY_Render_Entry_Rectangle_Outline:
      {
        Render_Entry_Rectangle_Outline* entry = (Render_Entry_Rectangle_Outline*)(void*)header;
        base_address += sizeof(*entry);
        Render_Group_Entry_Base* base = &entry->base;
        Entity_Render_Data data = Get_Entity_Render_Data(render_group, base, screen_center);

        Rect2 rect = Rect_Center_Dim(data.pos, base->dim * data.fudged_scale);
        Draw_Rect_Outline(draw_buffer, rect, base->color, entry->outline_pixel_thickness);
      }
      break;
      case E_RENDER_GROUP_ENTRY_Render_Entry_Coordinate_System:
      {
        Render_Entry_Coordinate_System* entry = (Render_Entry_Coordinate_System*)(void*)header;
        base_address += sizeof(*entry);

        Draw_Rect_Slowly(draw_buffer, entry->origin, entry->x_axis, entry->y_axis, entry->color, entry->texture);

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

internal Render_Group_Entry_Header* Push_Render_Element_(Render_Group* group, U32 size, E_Render_Group_Entry_Type type)
{
  Render_Group_Entry_Header* result = 0;
  if ((group->push_buffer_size + size) < group->max_push_buffer_size)
  {
    result = (Render_Group_Entry_Header*)(void*)(group->push_buffer_base + group->push_buffer_size);
    result->type = type;
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
    entry->color = Color4F_To_Premult(color);
  }
}

internal inline void Fill_Render_Base(Render_Group* group, Render_Group_Entry_Base* base, Vec2 dim, Vec2 offset,
                                      F32 offset_z, Vec2 align, F32 entity_cz, F32 scale, Vec4 color)
{
  base->basis = group->default_basis;
  base->dim = dim;
  base->offset = ((vec2(offset.x, -offset.y) * group->meters_to_pixels) - align);
  base->offset_z = offset_z;
  base->entity_cz = entity_cz;
  base->scale = scale;
  base->color = color;
}

internal inline void Push_Render_Rectangle(Render_Group* group, Vec2 dim, Vec2 offset, F32 offset_z, Vec2 align,
                                           F32 scale, Vec4 color, F32 entity_cz)

{
  Render_Entry_Rectangle* entry = Push_Render_Element(group, Render_Entry_Rectangle);
  if (entry)
  {
    Fill_Render_Base(group, &entry->base, dim, offset, offset_z, align, entity_cz, scale, color);
  }
}

internal inline void Push_Render_Rectangle_Outline(Render_Group* group, Vec2 dim, Vec2 offset, F32 offset_z, Vec2 align,
                                                   F32 scale, Vec4 color, F32 entity_cz,
                                                   F32 outline_pixel_thickness = 1.f)

{
  Render_Entry_Rectangle_Outline* entry = Push_Render_Element(group, Render_Entry_Rectangle_Outline);
  if (entry)
  {
    Fill_Render_Base(group, &entry->base, dim, offset, offset_z, align, entity_cz, scale, color);
    entry->outline_pixel_thickness = outline_pixel_thickness;
  }
}

internal inline void Push_Render_Bitmap(Render_Group* group, Loaded_Bitmap* bitmap, Vec2 bmp_inner_offset, Vec2 dim,
                                        Vec2 offset, F32 offset_z, Vec2 align, F32 scale, Vec4 color, F32 entity_cz)

{
  Render_Entry_Bitmap* entry = Push_Render_Element(group, Render_Entry_Bitmap);
  if (entry)
  {
    Fill_Render_Base(group, &entry->base, dim, offset, offset_z, align, entity_cz, scale, color);
    entry->bitmap = bitmap;
    entry->bmp_offset_x = (S32)bmp_inner_offset.x;
    entry->bmp_offset_y = (S32)bmp_inner_offset.y;
  }
}

internal inline Render_Entry_Coordinate_System*
Render_Coordinate_System(Render_Group* group, Vec2 origin, Vec2 x_axis, Vec2 y_axis, Vec4 color, Loaded_Bitmap* texture)
{
  Render_Entry_Coordinate_System* entry = Push_Render_Element(group, Render_Entry_Coordinate_System);
  if (entry)
  {
    entry->origin = origin;
    entry->x_axis = x_axis;
    entry->y_axis = y_axis;
    entry->color = color;
    entry->texture = texture;
  }
  return entry;
}

internal inline void Add_Bitmap_Render(Render_Group* group, Loaded_Bitmap* bitmap, Vec2 offset, F32 offset_z,
                                       Vec2 align, F32 scale, F32 alpha = 1.f, F32 entity_cz = 1.f)
{
  Push_Render_Bitmap(group, bitmap, vec2(0.f, 0.f), {{(F32)bitmap->width, (F32)bitmap->height}}, offset, offset_z,
                     align, scale, {{0, 0, 0, alpha}}, entity_cz);
}

internal inline void Add_Sprite_Bitmap_Render(Render_Group* group, Loaded_Bitmap* bitmap, Vec2 offset, F32 offset_z,
                                              Vec2 align, F32 scale, F32 alpha = 1.f, F32 entity_cz = 1.f)
{
  scale = scale * group->sprite_scale;
  Add_Bitmap_Render(group, bitmap, offset, offset_z, align, scale, alpha, entity_cz);
}

internal inline void Add_Sprite_Sheet_Render(Render_Group* group, Sprite_Sheet* sprite_sheet, U32 sprite_index,
                                             Vec2 offset, F32 offset_z, Vec2 align, F32 scale, F32 alpha = 1.f,
                                             F32 entity_cz = 1.f)
{
  Sprite sprite = sprite_sheet->sprites[sprite_index];
  scale = scale * group->sprite_scale;
  Push_Render_Bitmap(group, &sprite_sheet->bitmap, vec2((F32)sprite.x, (F32)sprite.y),
                     vec2((F32)sprite.width, (F32)sprite.height), offset, offset_z, align, scale, vec4(0, 0, 0, alpha),
                     entity_cz);
}

internal inline void Add_Bitmap_Render_Centered(Render_Group* group, Loaded_Bitmap* bitmap, Vec2 offset_meters,
                                                F32 offset_z, Vec2 align_pixels, F32 scale, F32 alpha = 1.f,
                                                F32 entity_cz = 1.f)
{
  align_pixels = vec2(align_pixels.x + 0.5f * (F32)bitmap->width, align_pixels.y + 0.5f * (F32)bitmap->height);
  Add_Bitmap_Render(group, bitmap, offset_meters, offset_z, align_pixels, scale, alpha, entity_cz);
}

internal inline void Add_Rect_Render(Render_Group* group, Vec2 offset, F32 offset_z, Vec2 align, Vec2 dim, F32 scale,
                                     Color4F color, F32 entity_cz = 1.f)
{
  Push_Render_Rectangle(group, dim * group->meters_to_pixels, offset, offset_z, align, scale, color, entity_cz);
}

internal inline void Add_Rect_Pixel_Outline_Render(Render_Group* group, Vec2 offset, F32 offset_z, Vec2 align, Vec2 dim,
                                                   F32 scale, Color4F color, F32 entity_cz = 1.f,
                                                   F32 outline_pixel_thickness = 1.f)
{
  Push_Render_Rectangle_Outline(group, dim * group->meters_to_pixels, offset, offset_z, align, scale, color, entity_cz,
                                outline_pixel_thickness);
}

internal inline void Add_Sprite_Scale_Rect_Render(Render_Group* group, Vec2 pixel_align, Vec2 dim, F32 scale,
                                                  Color4F color, F32 entity_cz = 1.f)
{
  scale = scale * group->sprite_scale;
  Push_Render_Rectangle(group, dim, vec2(0), 0, pixel_align, scale, color, entity_cz);
}

internal inline void Add_Rect_Outline_Render(Render_Group* group, Vec2 offset, F32 offset_z, Vec2 align, Vec2 dim,
                                             F32 scale, Color4F color, F32 entity_cz = 1.f,
                                             F32 thickness_in_meters = 0.1f)
{
  // HACK: to fix float rounding when straddling 0.5px boundary
  F32 epsilon = 0.0001f;
  Vec2 half_dim_y = vec2(0.f, .5f * dim.y);
  Vec2 half_dim_x = vec2(0.5f * dim.x, 0.f);

  Vec2 dim_horizontal = vec2(dim.x + thickness_in_meters, thickness_in_meters) + vec2(epsilon);
  Vec2 dim_vertical = vec2(thickness_in_meters, dim.y + thickness_in_meters) + vec2(epsilon);

  // TOP/BOTTOM
  Add_Rect_Render(group, offset + half_dim_y, offset_z, align, dim_horizontal, scale, color, entity_cz);
  Add_Rect_Render(group, offset - half_dim_y, offset_z, align, dim_horizontal, scale, color, entity_cz);

  // LEFT/RIGHT
  Add_Rect_Render(group, offset - half_dim_x, offset_z, align, dim_vertical, scale, color, entity_cz);
  Add_Rect_Render(group, offset + half_dim_x, offset_z, align, dim_vertical, scale, color, entity_cz);
}

internal void Add_Inputs_Render(Render_Group* render_group, Game_Input* input)
{

  Vec2 dim = vec2(0.4f);
  for (S32 i = 0; i < (S32)Array_Count(input->mouse_buttons); i++)
  {
    if (input->mouse_buttons[i].ended_down)
    {
      Vec2 offset = vec2((F32)i * 20.f + 10.f, 10.f);
      Add_Rect_Render(render_group, vec2(0), 0, -offset, dim, 1.f, vec4(1, 0, 0, 1));
    }
  }
  if (input->mouse_z > 0)
  {
    Vec2 offset = vec2(10.f, 30.f);
    Add_Rect_Render(render_group, vec2(0), 0, -offset, dim, 1.f, vec4(1, 0, 0, 1));
  }
  else if (input->mouse_z < 0)
  {
    Vec2 offset = vec2(30.f, 30.f);
    Add_Rect_Render(render_group, vec2(0), 0, -offset, dim, 1.f, vec4(1, 0, 0, 1));
  }

  Vec2 mouse_dim = vec2(0.1f);
  Vec2 mouse = vec2i(-input->mouse_x, -input->mouse_y) - 0.5f * mouse_dim * render_group->meters_to_pixels;
  Add_Rect_Pixel_Outline_Render(render_group, vec2(0), 0, mouse, mouse_dim, 1.f, vec4(1, 1, 0, 1), 1.f);
}
