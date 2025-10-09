
#include "hot.h"

void Draw_BMP_Subset_Hot(Loaded_Bitmap* buffer, Loaded_Bitmap* bitmap, F32 x, F32 y, S32 bmp_x_offset, S32 bmp_y_offset,
                         S32 bmp_x_dim, S32 bmp_y_dim, F32 scale, B32 alpha_blend, F32 c_alpha)
{
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
}

void Draw_Rectf_Hot(Loaded_Bitmap* buffer, F32 fmin_x, F32 fmin_y, F32 fmax_x, F32 fmax_y, Vec4 color)
{
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
}

void Draw_Rect_Slowly_Hot(Loaded_Bitmap* buffer, Vec2 origin, Vec2 x_axis, Vec2 y_axis, Vec4 color,
                          Loaded_Bitmap* texture)
{
  color.rgb *= color.a;
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

  // Color4 out_color = Color4F_To_Color4(color);

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

        // NOTE: Bilinear Filter
        // TODO: Trilinear filter, sample mipmaps
        U8* texel_ptr =
            ((U8*)texture->memory + (texture_v * texture->pitch_in_bytes) + (texture_u * BITMAP_BYTES_PER_PIXEL));

        Color4F texel_a = Color4_SRGB_To_Color4F_Linear(*(Color4*)(void*)(texel_ptr));
        Color4F texel_b = Color4_SRGB_To_Color4F_Linear(*(Color4*)(void*)(texel_ptr + sizeof(U32)));
        Color4F texel_c = Color4_SRGB_To_Color4F_Linear(*(Color4*)(void*)(texel_ptr + texture->pitch_in_bytes));
        Color4F texel_d =
            Color4_SRGB_To_Color4F_Linear(*(Color4*)(void*)(texel_ptr + texture->pitch_in_bytes + sizeof(U32)));

        Color4F texel_ab = Vec_Lerp(texel_a, texel_b, fu);
        Color4F texel_cd = Vec_Lerp(texel_c, texel_d, fu);
        Color4F src_texel = Vec_Lerp(texel_ab, texel_cd, fv);

        Color4F dest_color = Color4_SRGB_To_Color4F_Linear(*(Color4*)(void*)pixel);

        src_texel *= color;

        Color4F out_color = (1.f - src_texel.a) * dest_color + src_texel;

        *pixel = Color4F_Linear_To_Color4_SRGB(out_color).u32;
      }
      else
      {
        // *pixel = out_color.u32;
      }

      pixel++;
    }
    row_in_bytes += buffer->pitch_in_bytes;
  }
}
