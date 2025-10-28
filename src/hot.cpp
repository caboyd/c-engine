
#include "hot.h"

void Change_Saturation(Loaded_Bitmap* buffer, F32 saturation)
{
  U8* dest_row_in_bytes = (U8*)buffer->memory;
  for (S32 y_index = 0; y_index < buffer->height; y_index++)
  {
    U8* pixel = dest_row_in_bytes;

    for (S32 x_index = 0; x_index < buffer->width; x_index++)
    {

      Color4F dest = Color4_SRGB_To_Color4F_Linear(*(Color4*)(void*)pixel);

      F32 avg = (1.f / 3.f) * (dest.r + dest.g + dest.b);
      Vec3 delta = dest.rgb - vec3(avg);
      dest.rgb = vec3(avg) + saturation * delta;
      Color4 out = Color4F_Linear_To_Color4_SRGB(dest);
      // Note: BMP may not be 4 byte aligned so assign byte by byte
      *pixel++ = out.argb.b; // B
      *pixel++ = out.argb.g; // G
      *pixel++ = out.argb.r; // R
      *pixel++ = out.argb.a; // A
    }
    dest_row_in_bytes += buffer->pitch_in_bytes;
  }
}

void Draw_BMP_Subset_Hot(Loaded_Bitmap* buffer, Loaded_Bitmap* bitmap, F32 x, F32 y, S32 bmp_x_dim, S32 bmp_y_dim,
                         F32 scale, B32 alpha_blend, F32 c_alpha)
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
  S32 x_draw_offset = 0;
  if (min_x < 0)
  {
    x_draw_offset += (-min_x) % Round_F32_S32(scale * (F32)bmp_x_dim);
  }
  S32 y_draw_offset = 0;
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
    y_src = CLAMP(y_src, 0, bmp_y_dim - 1);
    ASSERT(y_src < bitmap->height);

    for (S32 x_index = min_x; x_index < max_x; x_index++)
    {
      S32 x_src = Trunc_F32_S32((F32)(x_index - min_x + x_draw_offset) / scale);
      x_src = CLAMP(x_src, 0, bmp_x_dim - 1);
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
}

struct Bilinear_Sample_Result
{
  Vec4_U8 a, b, c, d;
};

inline Bilinear_Sample_Result Bilinear_Sample(Loaded_Bitmap* texture, S32 x, S32 y)
{
  Bilinear_Sample_Result result;
  U8* sample_ptr = ((U8*)texture->memory + (y * texture->pitch_in_bytes) + (x * BITMAP_BYTES_PER_PIXEL));
  result.a = (*(Vec4_U8*)(sample_ptr));
  result.b = (*(Vec4_U8*)(sample_ptr + sizeof(U32)));
  result.c = (*(Vec4_U8*)(sample_ptr + texture->pitch_in_bytes));
  result.d = (*(Vec4_U8*)(sample_ptr + texture->pitch_in_bytes + sizeof(U32)));
  return result;
}

inline Vec4 Sample_Normal_Map(Loaded_Bitmap* normal_map, Vec2 uv)

{
  F32 tu = (uv.x * ((F32)normal_map->width - 2.f));
  F32 tv = (uv.y * ((F32)normal_map->height - 2.f));
  S32 texture_u = Trunc_F32_S32(tu);
  S32 texture_v = Trunc_F32_S32(tv);

  F32 fu = tu - (F32)texture_u;
  F32 fv = tv - (F32)texture_v;

  ASSERT(texture_u >= 0 && texture_u < normal_map->width);
  ASSERT(texture_v >= 0 && texture_v < normal_map->height);

  Bilinear_Sample_Result normal_sample = Bilinear_Sample(normal_map, texture_u, texture_v);

  Vec4 normal_a = Vec_Unpack_Vec4_U8(normal_sample.a);
  Vec4 normal_b = Vec_Unpack_Vec4_U8(normal_sample.b);
  Vec4 normal_c = Vec_Unpack_Vec4_U8(normal_sample.c);
  Vec4 normal_d = Vec_Unpack_Vec4_U8(normal_sample.d);

  Vec4 normal = Vec_Lerp(Vec_Lerp(normal_a, normal_b, fu), Vec_Lerp(normal_c, normal_d, fu), fv);

  return normal;
}

inline Vec4 SRGB_Bilinear_Blend(Bilinear_Sample_Result texel_sample, F32 fract_u, F32 fract_v)
{
  Color4F texel_a = Color4_SRGB_To_Color4F_Linear(texel_sample.a);
  Color4F texel_b = Color4_SRGB_To_Color4F_Linear(texel_sample.b);
  Color4F texel_c = Color4_SRGB_To_Color4F_Linear(texel_sample.c);
  Color4F texel_d = Color4_SRGB_To_Color4F_Linear(texel_sample.d);

  Color4F texel_ab = Vec_Lerp(texel_a, texel_b, fract_u);
  Color4F texel_cd = Vec_Lerp(texel_c, texel_d, fract_u);
  Color4F texel = Vec_Lerp(texel_ab, texel_cd, fract_v);
  return texel;
}

inline Vec4 Sample_Texture(Loaded_Bitmap* texture, Vec2 uv)
{
  F32 texel_u = (uv.x * ((F32)texture->width - 2.f));
  F32 texel_v = (uv.y * ((F32)texture->height - 2.f));
  S32 pixel_x = Trunc_F32_S32(texel_u);
  S32 pixel_y = Trunc_F32_S32(texel_v);

  F32 fract_u = texel_u - (F32)pixel_x;
  F32 fract_v = texel_v - (F32)pixel_y;

  ASSERT(pixel_x >= 0 && pixel_x < texture->width - 1);
  ASSERT(pixel_y >= 0 && pixel_y < texture->height - 1);

  Bilinear_Sample_Result texel_sample = Bilinear_Sample(texture, pixel_x, pixel_y);
  Vec4 texel = SRGB_Bilinear_Blend(texel_sample, fract_u, fract_v);

  return texel;
}

inline Vec3 Sample_Env_Map(Environment_Map* map, Vec2 screen_space_uv, Vec3 sample_direction, F32 roughness,
                           F32 distance_from_map_in_z)
{

  U32 lod_index = (U32)(roughness * (F32)(Array_Count(map->lod) - 1) + 0.5f);
  ASSERT(lod_index < Array_Count(map->lod));

  F32 uvs_per_meter = 0.1f;
  F32 c = (uvs_per_meter * distance_from_map_in_z) / sample_direction.y;
  Vec2 offset = c * vec2(sample_direction.x, sample_direction.z);

  Vec2 uv = screen_space_uv + offset;
  uv = Vec_Clamp01(uv);

  Loaded_Bitmap* lod = &map->lod[lod_index];

  F32 texel_u = (uv.x * ((F32)lod->width - 2.f));
  F32 texel_v = (uv.y * ((F32)lod->height - 2.f));

  S32 pixel_x = Trunc_F32_S32(texel_u);
  S32 pixel_y = Trunc_F32_S32(texel_v);

  F32 fract_u = texel_u - (F32)pixel_x;
  F32 fract_v = texel_v - (F32)pixel_y;

  ASSERT(pixel_x >= 0 && pixel_x < lod->width);
  ASSERT(pixel_y >= 0 && pixel_y < lod->height);

  Bilinear_Sample_Result texel_sample = Bilinear_Sample(lod, pixel_x, pixel_y);
  Vec3 result = SRGB_Bilinear_Blend(texel_sample, fract_u, fract_v).xyz;

  // U8* texel_ptr = ((U8*)lod->memory) + pixel_y * lod->pitch_in_bytes + pixel_x * sizeof(U32);
  // *(U32*)texel_ptr = 0xFFFFFFFF;

  return result;
}

inline Vec4 Unscale_And_Bias_Normal(Vec4 normal)
{
  Vec4 result;
  F32 inv_255 = 1.f / 255.f;
  result.xyz = vec3(-1.f) + 2.f * (inv_255 * normal.xyz);
  result.w *= inv_255;
  return result;
}

void Draw_Rect_Quickly_Hot(Loaded_Bitmap* buffer, Vec2 origin, Vec2 x_axis, Vec2 y_axis, Vec4 color,
                           Loaded_Bitmap* texture, F32 pixels_to_meters)
{
  BEGIN_TIMED_BLOCK(Draw_Rect_Quickly_Hot);
  // NOTE: convert to linear and premultiply alpha
  color = Color4F_SRGB_To_Linear(color);
  color = Color4F_Linear_Premult(color);

  S32 max_width = buffer->width - 1;
  S32 max_height = buffer->height - 1;

  F32 x_axis_len = Vec_Length(x_axis);
  F32 y_axis_len = Vec_Length(y_axis);
  F32 nxc = x_axis_len / y_axis_len;
  F32 nyc = y_axis_len / x_axis_len;
  Vec2 normal_x_axis = nxc * x_axis;
  Vec2 normal_y_axis = nyc * y_axis;
  F32 normal_z_scale = 0.5f * (x_axis_len + y_axis_len);

  F32 inv_x_axis_length_sq = 1.f / Vec_Length_Sq(x_axis);
  F32 inv_y_axis_length_sq = 1.f / Vec_Length_Sq(y_axis);

  F32 inv_max_width = 1.f / (F32)max_width;
  F32 inv_max_height = 1.f / (F32)max_height;

  // TODO: this will need to specified separately
  F32 origin_z = 0.0f;
  F32 origin_y = (origin + 0.5f * x_axis + 0.5f * y_axis).y;
  F32 fixed_cast_y = inv_max_height * origin_y;

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

  x_min = CLAMP(x_min, 0, max_width);
  x_max = CLAMP(x_max, 0, max_width);
  y_min = CLAMP(y_min, 0, max_height);
  y_max = CLAMP(y_max, 0, max_height);

  Vec2 nx_axis = inv_x_axis_length_sq * x_axis;
  Vec2 ny_axis = inv_y_axis_length_sq * y_axis;
  F32 inv_255 = 1.f / 255.f;

  U8* row_in_bytes = (U8*)buffer->memory + (y_min * buffer->pitch_in_bytes) + (x_min * BITMAP_BYTES_PER_PIXEL);

  for (S32 y = y_min; y <= y_max; y++)
  {
    U32* pixel = (U32*)(void*)row_in_bytes;
    for (S32 x = x_min; x <= x_max; x++)
    {
      BEGIN_TIMED_BLOCK(Test_Pixel);
      Vec2 pixel_pos = vec2i(x, y);

      Vec2 d = pixel_pos - origin;

      Vec2 uv = vec2(Vec_Dot(d, nx_axis), Vec_Dot(d, ny_axis));

      if ((uv.u >= 0.f) && (uv.u <= 1.f) && (uv.v >= 0.f) && (uv.v < 1.f))
      {
        BEGIN_TIMED_BLOCK(Fill_Pixel);

        Vec2 screen_space_uv = vec2(inv_max_width * (F32)x, fixed_cast_y);
        F32 z_diff = pixels_to_meters * ((F32)y - origin_y);

        F32 texel_u = (uv.x * ((F32)texture->width - 2.f));
        F32 texel_v = (uv.y * ((F32)texture->height - 2.f));
        S32 pixel_x = Trunc_F32_S32(texel_u);
        S32 pixel_y = Trunc_F32_S32(texel_v);

        F32 fract_u = texel_u - (F32)pixel_x;
        F32 fract_v = texel_v - (F32)pixel_y;

        ASSERT(pixel_x >= 0 && pixel_x < texture->width - 1);
        ASSERT(pixel_y >= 0 && pixel_y < texture->height - 1);

        // NOTE: bilinear sample
        U8* sample_ptr =
            ((U8*)texture->memory + (pixel_y * texture->pitch_in_bytes) + (pixel_x * BITMAP_BYTES_PER_PIXEL));
        Vec4_U8 sample_A = (*(Vec4_U8*)(sample_ptr));
        Vec4_U8 sample_B = (*(Vec4_U8*)(sample_ptr + sizeof(U32)));
        Vec4_U8 sample_C = (*(Vec4_U8*)(sample_ptr + texture->pitch_in_bytes));
        Vec4_U8 sample_D = (*(Vec4_U8*)(sample_ptr + texture->pitch_in_bytes + sizeof(U32)));

        F32 texel_Aa = sample_A.argb.a;
        F32 texel_Ar = sample_A.argb.r;
        F32 texel_Ag = sample_A.argb.g;
        F32 texel_Ab = sample_A.argb.b;

        F32 texel_Ba = sample_B.argb.a;
        F32 texel_Br = sample_B.argb.r;
        F32 texel_Bg = sample_B.argb.g;
        F32 texel_Bb = sample_B.argb.b;

        F32 texel_Ca = sample_C.argb.a;
        F32 texel_Cr = sample_C.argb.r;
        F32 texel_Cg = sample_C.argb.g;
        F32 texel_Cb = sample_C.argb.b;

        F32 texel_Da = sample_D.argb.a;
        F32 texel_Dr = sample_D.argb.r;
        F32 texel_Dg = sample_D.argb.g;
        F32 texel_Db = sample_D.argb.b;

        // NOTE: go from sRGB to "linear" brightness space
        texel_Ar = Square(inv_255 * texel_Ar);
        texel_Ag = Square(inv_255 * texel_Ag);
        texel_Ab = Square(inv_255 * texel_Ab);
        texel_Aa = (inv_255 * texel_Aa);

        texel_Br = Square(inv_255 * texel_Br);
        texel_Bg = Square(inv_255 * texel_Bg);
        texel_Bb = Square(inv_255 * texel_Bb);
        texel_Ba = (inv_255 * texel_Ba);

        texel_Cr = Square(inv_255 * texel_Cr);
        texel_Cg = Square(inv_255 * texel_Cg);
        texel_Cb = Square(inv_255 * texel_Cb);
        texel_Ca = (inv_255 * texel_Ca);

        texel_Dr = Square(inv_255 * texel_Dr);
        texel_Dg = Square(inv_255 * texel_Dg);
        texel_Db = Square(inv_255 * texel_Db);
        texel_Da = (inv_255 * texel_Da);

        // NOTE: bilinear texture blend
        F32 ifU = 1.f - fract_u;
        F32 ifV = 1.f - fract_v;
        F32 lerp0 = ifV * ifU;
        F32 lerp1 = ifV * fract_u;
        F32 lerp2 = fract_v * ifU;
        F32 lerp3 = fract_v * fract_u;

        F32 texel_r = lerp0 * texel_Ar + lerp1 * texel_Br + lerp2 * texel_Cr + lerp3 * texel_Dr;
        F32 texel_g = lerp0 * texel_Ag + lerp1 * texel_Bg + lerp2 * texel_Cg + lerp3 * texel_Dg;
        F32 texel_b = lerp0 * texel_Ab + lerp1 * texel_Bb + lerp2 * texel_Cb + lerp3 * texel_Db;
        F32 texel_a = lerp0 * texel_Aa + lerp1 * texel_Ba + lerp2 * texel_Ca + lerp3 * texel_Da;

        // NOTE: modulate by incoming color
        texel_r = texel_r * color.r;
        texel_g = texel_g * color.g;
        texel_b = texel_b * color.b;
        texel_a = texel_a * color.a;

        // NOTE: clamp to valid range
        texel_r = Clamp01(texel_r);
        texel_g = Clamp01(texel_g);
        texel_b = Clamp01(texel_b);

        // NOTE: load destination
        F32 dest_r = (*(Vec4_U8*)pixel).argb.r;
        F32 dest_g = (*(Vec4_U8*)pixel).argb.g;
        F32 dest_b = (*(Vec4_U8*)pixel).argb.b;
        F32 dest_a = (*(Vec4_U8*)pixel).argb.a;

        // NOTE: go from sRGB to "linear" brightness space
        dest_r = Square(inv_255 * dest_r);
        dest_g = Square(inv_255 * dest_g);
        dest_b = Square(inv_255 * dest_b);
        dest_a = (inv_255 * dest_a);

        // NOTE: destination blend
        F32 inv_texel_a = (1.f - texel_a);
        F32 blended_r = inv_texel_a * dest_r + texel_r;
        F32 blended_g = inv_texel_a * dest_g + texel_g;
        F32 blended_b = inv_texel_a * dest_b + texel_b;
        F32 blended_a = inv_texel_a * dest_a + texel_a;

        // NOTE: go from "linear" brigthness space to sRGB
        blended_r = Sqrt_F32(blended_r) * 255.f;
        blended_g = Sqrt_F32(blended_g) * 255.f;
        blended_b = Sqrt_F32(blended_b) * 255.f;
        blended_a = blended_a * 255.f;

        // NOTE: repack color
        Vec4_U8 packed_color;
        packed_color.argb.r = blended_r + 0.5f;
        packed_color.argb.g = blended_g + 0.5f;
        packed_color.argb.b = blended_b + 0.5f;
        packed_color.argb.a = blended_a + 0.5f;

        *pixel = packed_color.u32;

        END_TIMED_BLOCK(Fill_Pixel);
      }

      pixel++;

      END_TIMED_BLOCK(Test_Pixel);
    }
    row_in_bytes += buffer->pitch_in_bytes;
  }

  END_TIMED_BLOCK(Draw_Rect_Quickly_Hot);
}

void Draw_Rect_Slowly_Hot(Loaded_Bitmap* buffer, Vec2 origin, Vec2 x_axis, Vec2 y_axis, Vec4 color,
                          Loaded_Bitmap* texture, Loaded_Bitmap* normal_map, Environment_Map* top_env_map,
                          Environment_Map* middle_env_map, Environment_Map* bottom_env_map, F32 pixels_to_meters)
{
  BEGIN_TIMED_BLOCK(Draw_Rect_Slowly_Hot);
  // NOTE: convert to linear and premultiply alpha
  color = Color4F_SRGB_To_Linear(color);
  color = Color4F_Linear_Premult(color);

  S32 max_width = buffer->width - 1;
  S32 max_height = buffer->height - 1;

  F32 x_axis_len = Vec_Length(x_axis);
  F32 y_axis_len = Vec_Length(y_axis);
  F32 nxc = x_axis_len / y_axis_len;
  F32 nyc = y_axis_len / x_axis_len;
  Vec2 normal_x_axis = nxc * x_axis;
  Vec2 normal_y_axis = nyc * y_axis;
  F32 normal_z_scale = 0.5f * (x_axis_len + y_axis_len);

  F32 inv_x_axis_length_sq = 1.f / Vec_Length_Sq(x_axis);
  F32 inv_y_axis_length_sq = 1.f / Vec_Length_Sq(y_axis);

  F32 inv_max_width = 1.f / (F32)max_width;
  F32 inv_max_height = 1.f / (F32)max_height;

  // TODO: this will need to specified separately
  F32 origin_z = 0.0f;
  F32 origin_y = (origin + 0.5f * x_axis + 0.5f * y_axis).y;
  F32 fixed_cast_y = inv_max_height * origin_y;

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

  x_min = CLAMP(x_min, 0, max_width);
  x_max = CLAMP(x_max, 0, max_width);
  y_min = CLAMP(y_min, 0, max_height);
  y_max = CLAMP(y_max, 0, max_height);

  U8* row_in_bytes = (U8*)buffer->memory + (y_min * buffer->pitch_in_bytes) + (x_min * BITMAP_BYTES_PER_PIXEL);

  for (S32 y = y_min; y <= y_max; y++)
  {
    U32* pixel = (U32*)(void*)row_in_bytes;
    for (S32 x = x_min; x <= x_max; x++)
    {
      BEGIN_TIMED_BLOCK(Test_Pixel);
      Vec2 pixel_pos = vec2i(x, y);
      // TODO: perp dot
      Vec2 d = pixel_pos - origin;
      F32 edge_0 = Vec_Dot(d, -Vec_Perp(x_axis));
      F32 edge_1 = Vec_Dot(d - x_axis, -Vec_Perp(y_axis));
      F32 edge_2 = Vec_Dot(d - x_axis - y_axis, Vec_Perp(x_axis));
      F32 edge_3 = Vec_Dot(d - y_axis, Vec_Perp(y_axis));

      if ((edge_0 < 0) && (edge_1 < 0) && (edge_2 < 0) && (edge_3 < 0))
      {
        BEGIN_TIMED_BLOCK(Fill_Pixel);
        Vec2 uv = vec2(Clamp01(inv_x_axis_length_sq * Vec_Dot(d, x_axis)),
                       Clamp01(inv_y_axis_length_sq * Vec_Dot(d, y_axis)));

        Vec2 screen_space_uv = vec2(inv_max_width * (F32)x, fixed_cast_y);
        F32 z_diff = pixels_to_meters * ((F32)y - origin_y);
        // Vec2 screen_space_uv = vec2(inv_max_width * (F32)x, inv_max_height * (F32)y);
        // F32 z_diff = 0.f;

        ASSERT(uv.x >= 0.f && uv.x <= 1.f);
        ASSERT(uv.y >= 0.f && uv.y <= 1.f);

        Color4F texel = Sample_Texture(texture, uv);
        if (normal_map)
        {
          Vec4 normal = Sample_Normal_Map(normal_map, uv);
          normal = Unscale_And_Bias_Normal(normal);

          // NOTE: rootate normals based on x/y axis

          normal.xy = normal.x * normal_x_axis + normal.y * normal_y_axis;
          normal.z *= normal_z_scale;
          normal.xyz = Vec_Normalize(normal.xyz);

          // NOTE: eye vector is assumed to always be 0,0,1
          // Vec3 eye_vector = vec3(0, 0, 1);

          // NOTE: this is just the simplified version of the reflection of the negative eye vector on the normal
          Vec3 bounce_direction = 2.f * normal.z * normal.xyz;
          bounce_direction.z -= 1.f;
          bounce_direction.z = -bounce_direction.z;

          Environment_Map* far_map = NULL;
          F32 pos_z = origin_z + z_diff;
          F32 t_env_map = bounce_direction.y;
          F32 t_far_map = 0.f;
          if (t_env_map < -0.5f)
          {
            far_map = bottom_env_map;
            t_far_map = -1.f - 2.f * t_env_map;
          }
          else if (t_env_map > 0.5f)
          {
            far_map = top_env_map;
            t_far_map = 2.f * (t_env_map - 0.5f);
          }
          else
          {
          }
          t_far_map *= t_far_map;

          Vec3 light_color = vec3(0); // Sample_Env_Map(middle_env_map, screen_space_uv, normal.xyz, normal.w);
          if (far_map)
          {
            F32 distance_from_map_in_z = far_map->pos_z - pos_z;
            Vec3 far_map_color =
                Sample_Env_Map(far_map, screen_space_uv, bounce_direction, normal.w, distance_from_map_in_z);

            light_color = Vec_Lerp(light_color, far_map_color, t_far_map);
          }
          texel.rgb += texel.a * light_color;

          // NOTE: draw bounce direction
          // texel.rgb = (vec3(0.5f) + 0.5f * bounce_direction) * texel.a;
        }

        texel *= color;
        texel = Vec_Clamp01(texel);

        Color4F dest_color = Color4_SRGB_To_Color4F_Linear(*(Color4*)pixel);

        Color4F out_color = (1.f - texel.a) * dest_color + texel;

        *pixel = Color4F_Linear_To_Color4_SRGB(out_color).u32;

        END_TIMED_BLOCK(Fill_Pixel);
      }
      else
      {
        // *pixel = out_color.u32;
      }

      pixel++;

      END_TIMED_BLOCK(Test_Pixel);
    }
    row_in_bytes += buffer->pitch_in_bytes;
  }

  END_TIMED_BLOCK(Draw_Rect_Slowly_Hot);
}
