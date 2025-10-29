
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

  // TODO: remove clipping once we have real row loading
  S32 max_width = (buffer->width - 1) - 3;
  S32 max_height = (buffer->height - 1) - 3;

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
  __m128 inv_255_4x = _mm_set1_ps(1.f / 255.f);
  __m128 one = _mm_set1_ps(1.f);
  __m128 zero = _mm_set1_ps(0.f);
  __m128 one_255_4x = _mm_set1_ps(255.f);

  __m128 color_r = _mm_set1_ps(color.r);
  __m128 color_g = _mm_set1_ps(color.g);
  __m128 color_b = _mm_set1_ps(color.b);
  __m128 color_a = _mm_set1_ps(color.a);

  __m128 origin_x_4x = _mm_set1_ps(origin.x);
  __m128 origin_y_4x = _mm_set1_ps(origin.y);

  __m128 nx_axis_x_4x = _mm_set1_ps(nx_axis.x);
  __m128 nx_axis_y_4x = _mm_set1_ps(nx_axis.y);
  __m128 ny_axis_x_4x = _mm_set1_ps(ny_axis.x);
  __m128 ny_axis_y_4x = _mm_set1_ps(ny_axis.y);

  U32 last_round_mode = _MM_GET_ROUNDING_MODE();
  _MM_SET_ROUNDING_MODE(_MM_ROUND_NEAREST);

  U8* row_in_bytes = (U8*)buffer->memory + (y_min * buffer->pitch_in_bytes) + (x_min * BITMAP_BYTES_PER_PIXEL);

  BEGIN_TIMED_BLOCK(Process_Pixel);
  for (S32 y = y_min; y <= y_max; y++)
  {
    U32* pixel = (U32*)(void*)row_in_bytes;
    for (S32 xi = x_min; xi <= x_max; xi += 4)
    {

      __m128 texel_Aa;
      __m128 texel_Ar;
      __m128 texel_Ag;
      __m128 texel_Ab;

      __m128 texel_Ba;
      __m128 texel_Br;
      __m128 texel_Bg;
      __m128 texel_Bb;

      __m128 texel_Ca;
      __m128 texel_Cr;
      __m128 texel_Cg;
      __m128 texel_Cb;

      __m128 texel_Da;
      __m128 texel_Dr;
      __m128 texel_Dg;
      __m128 texel_Db;

      __m128 texel_r;
      __m128 texel_g;
      __m128 texel_b;
      __m128 texel_a;

      __m128 dest_a;
      __m128 dest_r;
      __m128 dest_g;
      __m128 dest_b;

      __m128 blended_a;
      __m128 blended_r;
      __m128 blended_g;
      __m128 blended_b;

      __m128 fract_u;
      __m128 fract_v;

      __m128 pixel_pos_x = _mm_set_ps(xi + 3, xi + 2, xi + 1, xi + 0);
      __m128 pixel_pos_y = _mm_set1_ps((F32)y);

      __m128 dx = _mm_sub_ps(pixel_pos_x, origin_x_4x);
      __m128 dy = _mm_sub_ps(pixel_pos_y, origin_y_4x);

      __m128 u = _mm_add_ps(_mm_mul_ps(dx, nx_axis_y_4x), _mm_mul_ps(dx, nx_axis_x_4x));

      __m128 v = _mm_add_ps(_mm_mul_ps(dy, ny_axis_x_4x), _mm_mul_ps(dy, ny_axis_y_4x));

      __m128 u_ge0 = _mm_cmpge_ps(u, zero); // u >= 0
      __m128 u_le1 = _mm_cmple_ps(u, one);  // u <= 1
      __m128 v_ge0 = _mm_cmpge_ps(v, zero); // v >= 0
      __m128 v_lt1 = _mm_cmplt_ps(v, one);  // v < 1

      __m128 mask = _mm_and_ps(_mm_and_ps(u_ge0, u_le1), _mm_and_ps(v_ge0, v_lt1));
      S32 should_fill = _mm_movemask_ps(mask);

      for (S32 i = 0; i < 4; ++i)
      {
        if (should_fill & (1 << i))
        {

          // Vec2 screen_space_uv = vec2(inv_max_width * (F32)x, fixed_cast_y);
          F32 z_diff = pixels_to_meters * ((F32)y - origin_y);

          F32 texel_u = (u[i] * ((F32)texture->width - 2.f));
          F32 texel_v = (v[i] * ((F32)texture->height - 2.f));
          S32 pixel_x = Trunc_F32_S32(texel_u);
          S32 pixel_y = Trunc_F32_S32(texel_v);

          fract_u[i] = texel_u - (F32)pixel_x;
          fract_v[i] = texel_v - (F32)pixel_y;

          ASSERT(pixel_x >= 0 && pixel_x < texture->width - 1);
          ASSERT(pixel_y >= 0 && pixel_y < texture->height - 1);

          // NOTE: bilinear sample
          U8* sample_ptr =
              ((U8*)texture->memory + (pixel_y * texture->pitch_in_bytes) + (pixel_x * BITMAP_BYTES_PER_PIXEL));
          Vec4_U8 sample_A = (*(Vec4_U8*)(sample_ptr));
          Vec4_U8 sample_B = (*(Vec4_U8*)(sample_ptr + sizeof(U32)));
          Vec4_U8 sample_C = (*(Vec4_U8*)(sample_ptr + texture->pitch_in_bytes));
          Vec4_U8 sample_D = (*(Vec4_U8*)(sample_ptr + texture->pitch_in_bytes + sizeof(U32)));

          // __m128i texel_Aa8 = _mm_cvtsi32_si128(sample_A.u32);
          // __m128i texel_Aa32 = _mm_cvtepu8_epi32(texel_Aa8);

          // texel_Aa = _mm_cvtepi32_ps(texel_Aa32);

          texel_Ar[i] = sample_A.argb.r;
          texel_Ag[i] = sample_A.argb.g;
          texel_Ab[i] = sample_A.argb.b;
          texel_Aa[i] = sample_A.argb.a;

          texel_Br[i] = sample_B.argb.r;
          texel_Bg[i] = sample_B.argb.g;
          texel_Bb[i] = sample_B.argb.b;
          texel_Ba[i] = sample_B.argb.a;

          texel_Cr[i] = sample_C.argb.r;
          texel_Cg[i] = sample_C.argb.g;
          texel_Cb[i] = sample_C.argb.b;
          texel_Ca[i] = sample_C.argb.a;

          texel_Dr[i] = sample_D.argb.r;
          texel_Dg[i] = sample_D.argb.g;
          texel_Db[i] = sample_D.argb.b;
          texel_Da[i] = sample_D.argb.a;

          // NOTE: load destination
          dest_r[i] = (*(Vec4_U8*)(pixel + i)).argb.r;
          dest_g[i] = (*(Vec4_U8*)(pixel + i)).argb.g;
          dest_b[i] = (*(Vec4_U8*)(pixel + i)).argb.b;
          dest_a[i] = (*(Vec4_U8*)(pixel + i)).argb.a;
        }
      }

#define mm_Square(a) _mm_mul_ps(a, a)

      // NOTE: convert from sRGB to "linear" brightness space
      texel_Ar = mm_Square(_mm_mul_ps(inv_255_4x, texel_Ar));
      texel_Ag = mm_Square(_mm_mul_ps(inv_255_4x, texel_Ag));
      texel_Ab = mm_Square(_mm_mul_ps(inv_255_4x, texel_Ab));
      texel_Aa = _mm_mul_ps(inv_255_4x, texel_Aa);

      texel_Br = mm_Square(_mm_mul_ps(inv_255_4x, texel_Br));
      texel_Bg = mm_Square(_mm_mul_ps(inv_255_4x, texel_Bg));
      texel_Bb = mm_Square(_mm_mul_ps(inv_255_4x, texel_Bb));
      texel_Ba = _mm_mul_ps(inv_255_4x, texel_Ba);

      texel_Cr = mm_Square(_mm_mul_ps(inv_255_4x, texel_Cr));
      texel_Cg = mm_Square(_mm_mul_ps(inv_255_4x, texel_Cg));
      texel_Cb = mm_Square(_mm_mul_ps(inv_255_4x, texel_Cb));
      texel_Ca = _mm_mul_ps(inv_255_4x, texel_Ca);

      texel_Dr = mm_Square(_mm_mul_ps(inv_255_4x, texel_Dr));
      texel_Dg = mm_Square(_mm_mul_ps(inv_255_4x, texel_Dg));
      texel_Db = mm_Square(_mm_mul_ps(inv_255_4x, texel_Db));
      texel_Da = _mm_mul_ps(inv_255_4x, texel_Da);

      // NOTE: bilinear texture blend
      __m128 ifU = _mm_sub_ps(one, fract_u);
      __m128 ifV = _mm_sub_ps(one, fract_v);
      __m128 lerp0 = _mm_mul_ps(ifV, ifU);
      __m128 lerp1 = _mm_mul_ps(ifV, fract_u);
      __m128 lerp2 = _mm_mul_ps(fract_v, ifU);
      __m128 lerp3 = _mm_mul_ps(fract_v, fract_u);

      texel_r = _mm_add_ps(_mm_add_ps(_mm_mul_ps(lerp0, texel_Ar), _mm_mul_ps(lerp1, texel_Br)),
                           _mm_add_ps(_mm_mul_ps(lerp2, texel_Cr), _mm_mul_ps(lerp3, texel_Dr)));
      texel_g = _mm_add_ps(_mm_add_ps(_mm_mul_ps(lerp0, texel_Ag), _mm_mul_ps(lerp1, texel_Bg)),
                           _mm_add_ps(_mm_mul_ps(lerp2, texel_Cg), _mm_mul_ps(lerp3, texel_Dg)));
      texel_b = _mm_add_ps(_mm_add_ps(_mm_mul_ps(lerp0, texel_Ab), _mm_mul_ps(lerp1, texel_Bb)),
                           _mm_add_ps(_mm_mul_ps(lerp2, texel_Cb), _mm_mul_ps(lerp3, texel_Db)));
      texel_a = _mm_add_ps(_mm_add_ps(_mm_mul_ps(lerp0, texel_Aa), _mm_mul_ps(lerp1, texel_Ba)),
                           _mm_add_ps(_mm_mul_ps(lerp2, texel_Ca), _mm_mul_ps(lerp3, texel_Da)));

      // NOTE: modulate by incoming color
      texel_r = _mm_mul_ps(texel_r, color_r);
      texel_g = _mm_mul_ps(texel_g, color_g);
      texel_b = _mm_mul_ps(texel_b, color_b);
      texel_a = _mm_mul_ps(texel_a, color_a);

      // NOTE: clamp to valid range
      texel_r = _mm_min_ps(_mm_max_ps(texel_r, zero), one);
      texel_g = _mm_min_ps(_mm_max_ps(texel_g, zero), one);
      texel_b = _mm_min_ps(_mm_max_ps(texel_b, zero), one);

      // NOTE: go from sRGB to "linear" brightness space
      dest_r = mm_Square(_mm_mul_ps(inv_255_4x, dest_r));
      dest_g = mm_Square(_mm_mul_ps(inv_255_4x, dest_g));
      dest_b = mm_Square(_mm_mul_ps(inv_255_4x, dest_b));
      dest_a = _mm_mul_ps(inv_255_4x, dest_a);

      // NOTE: destination blend
      __m128 inv_texel_a = _mm_sub_ps(one, texel_a);
      blended_r = _mm_add_ps(_mm_mul_ps(inv_texel_a, dest_r), texel_r);
      blended_g = _mm_add_ps(_mm_mul_ps(inv_texel_a, dest_g), texel_g);
      blended_b = _mm_add_ps(_mm_mul_ps(inv_texel_a, dest_b), texel_b);
      blended_a = _mm_add_ps(_mm_mul_ps(inv_texel_a, dest_a), texel_a);

      // NOTE: go from "linear" brigthness space to sRGB

      blended_r = _mm_mul_ps(_mm_sqrt_ps(blended_r), one_255_4x);
      blended_g = _mm_mul_ps(_mm_sqrt_ps(blended_g), one_255_4x);
      blended_b = _mm_mul_ps(_mm_sqrt_ps(blended_b), one_255_4x);
      blended_a = _mm_mul_ps(blended_a, one_255_4x);

      __m128i int_r = _mm_cvtps_epi32(blended_r);
      __m128i int_g = _mm_cvtps_epi32(blended_g);
      __m128i int_b = _mm_cvtps_epi32(blended_b);
      __m128i int_a = _mm_cvtps_epi32(blended_a);

      __m128i shift_r = _mm_slli_epi32(int_r, 16);
      __m128i shift_g = _mm_slli_epi32(int_g, 8);
      __m128i shift_b = int_b;
      __m128i shift_a = _mm_slli_epi32(int_a, 24);

      __m128i bgra8 = _mm_or_si128(_mm_or_si128(shift_r, shift_g), _mm_or_si128(shift_b, shift_a));

      __m128i mask_epi = _mm_castps_si128(mask);
      __m128i pixel_dest = _mm_loadu_si128((__m128i*)pixel);

      __m128i out = _mm_blendv_epi8(pixel_dest, bgra8, mask_epi);

      _mm_storeu_si128((__m128i*)pixel, out);

      pixel += 4;
    }
    row_in_bytes += buffer->pitch_in_bytes;
  }
  _MM_SET_ROUNDING_MODE(last_round_mode);
  END_TIMED_BLOCK_COUNTED(Process_Pixel, (x_max - x_min + 1) * (y_max - y_min + 1));

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
      Vec2 pixel_pos = vec2i(x, y);
      // TODO: perp dot
      Vec2 d = pixel_pos - origin;
      F32 edge_0 = Vec_Dot(d, -Vec_Perp(x_axis));
      F32 edge_1 = Vec_Dot(d - x_axis, -Vec_Perp(y_axis));
      F32 edge_2 = Vec_Dot(d - x_axis - y_axis, Vec_Perp(x_axis));
      F32 edge_3 = Vec_Dot(d - y_axis, Vec_Perp(y_axis));

      if ((edge_0 < 0) && (edge_1 < 0) && (edge_2 < 0) && (edge_3 < 0))
      {
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
      }
      else
      {
        // *pixel = out_color.u32;
      }

      pixel++;
    }
    row_in_bytes += buffer->pitch_in_bytes;
  }

  END_TIMED_BLOCK(Draw_Rect_Slowly_Hot);
}
