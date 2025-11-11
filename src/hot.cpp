
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

void Draw_Rectf_Hot(Loaded_Bitmap* buffer, F32 fmin_x, F32 fmin_y, F32 fmax_x, F32 fmax_y, Vec4 color, Rect2i clip_rect)
{
  fmin_x = CLAMP(fmin_x, 0, (F32)buffer->width);
  fmin_y = CLAMP(fmin_y, 0, (F32)buffer->width);
  fmax_x = CLAMP(fmax_x, 0, (F32)buffer->width);
  fmax_y = CLAMP(fmax_y, 0, (F32)buffer->width);

  Rect2i fill_rect;
  fill_rect.min_x = Round_F32_S32(fmin_x);
  fill_rect.min_y = Round_F32_S32(fmin_y);
  fill_rect.max_x = Round_F32_S32(fmax_x);
  fill_rect.max_y = Round_F32_S32(fmax_y);

  fill_rect = Rect_Intersect(fill_rect, clip_rect);

  // NOTE: premultiply alpha
  color = Color4F_SRGB_Premult(color);
  U32 out_color = Color4F_To_Color4(color).u32;

  U8* row_in_bytes =
      (U8*)buffer->memory + (fill_rect.min_y * buffer->pitch_in_bytes) + (fill_rect.min_x * BITMAP_BYTES_PER_PIXEL);

  for (S32 y = fill_rect.min_y; y < fill_rect.max_y; y++)
  {
    U32* pixel = (U32*)(void*)row_in_bytes;
    for (S32 x = fill_rect.min_x; x < fill_rect.max_x; x++)
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

void Draw_Rect_Quickly_Hot256(Loaded_Bitmap* buffer, Vec2 origin, Vec2 x_axis, Vec2 y_axis, Vec4 color,
                              Loaded_Bitmap* texture, F32 pixels_to_meters, Rect2i clip_rect)
{
  BEGIN_TIMED_BLOCK(Draw_Rect_Quickly_Hot);

  // NOTE: convert to linear and premultiply alpha
  color = Color4F_SRGB_To_Linear(color);
  color = Color4F_Linear_Premult(color);

  Vec2 points[4] = {origin, origin + x_axis, origin + y_axis, origin + x_axis + y_axis};

  Rect2i fill_rect = {clip_rect.max_x, clip_rect.max_y, clip_rect.min_x, clip_rect.min_y};

  for (U32 point_index = 0; point_index < Array_Count(points); ++point_index)
  {
    Vec2 p = points[point_index];
    S32 floor_x = Floor_F32_S32(p.x);
    S32 ceil_x = Ceil_F32_S32(p.x) + 1;
    S32 floor_y = Floor_F32_S32(p.y);
    S32 ceil_y = Ceil_F32_S32(p.y) + 1;

    Rect2i point_rect = {floor_x, floor_y, ceil_x, ceil_y};
    fill_rect = Rect_Union(point_rect, fill_rect);
  }

  fill_rect = Rect_Intersect(clip_rect, fill_rect);

  if (!Rect_Has_Area(fill_rect))
  {
    return;
  }

  F32 x_axis_len = Vec_Length(x_axis);
  F32 y_axis_len = Vec_Length(y_axis);

  F32 nxc = x_axis_len / y_axis_len;
  F32 nyc = y_axis_len / x_axis_len;

  F32 inv_x_axis_length_sq = 1.f / Vec_Length_Sq(x_axis);
  F32 inv_y_axis_length_sq = 1.f / Vec_Length_Sq(y_axis);

  Vec2 nx_axis = inv_x_axis_length_sq * x_axis;
  Vec2 ny_axis = inv_y_axis_length_sq * y_axis;

  // Broadcast constants to 256-bit
  __m256 inv_255 = _mm256_set1_ps(1.f / 255.f);
  __m256 one = _mm256_set1_ps(1.f);
  __m256 zero = _mm256_setzero_ps();
  __m256 one_255 = _mm256_set1_ps(255.f);

  __m256 color_r = _mm256_set1_ps(color.r);
  __m256 color_g = _mm256_set1_ps(color.g);
  __m256 color_b = _mm256_set1_ps(color.b);
  __m256 color_a = _mm256_set1_ps(color.a);

  __m256 nx_axis_x = _mm256_set1_ps(nx_axis.x);
  __m256 nx_axis_y = _mm256_set1_ps(nx_axis.y);
  __m256 ny_axis_x = _mm256_set1_ps(ny_axis.x);
  __m256 ny_axis_y = _mm256_set1_ps(ny_axis.y);

  __m256 width_m1 = _mm256_set1_ps((F32)texture->width - 1);
  __m256 height_m1 = _mm256_set1_ps((F32)texture->height - 1);

  __m256i texture_pitch = _mm256_set1_epi32(texture->pitch_in_bytes);
  __m256i bytes_per_pixel = _mm256_set1_epi32(BITMAP_BYTES_PER_PIXEL);

  U8* texture_memory = (U8*)texture->memory;

  U32 last_round_mode = _MM_GET_ROUNDING_MODE();
  _MM_SET_ROUNDING_MODE(_MM_ROUND_NEAREST);

  // clang-format off
  __m256i shuffle_mask_b = _mm256_setr_epi8(
       0,  -1, -1, -1,
       4,  -1, -1, -1,
       8,  -1, -1, -1,
      12,  -1, -1, -1,
      16,  -1, -1, -1,
      20,  -1, -1, -1,
      24,  -1, -1, -1,
      28,  -1, -1, -1
  );

  __m256i shuffle_mask_g = _mm256_setr_epi8(
       1,  -1, -1, -1,
       5,  -1, -1, -1,
       9,  -1, -1, -1,
      13,  -1, -1, -1,
      17,  -1, -1, -1,
      21,  -1, -1, -1,
      25,  -1, -1, -1,
      29,  -1, -1, -1
  );

  __m256i shuffle_mask_r = _mm256_setr_epi8(
       2,  -1, -1, -1,
       6,  -1, -1, -1,
      10,  -1, -1, -1,
      14,  -1, -1, -1,
      18,  -1, -1, -1,
      22,  -1, -1, -1,
      26,  -1, -1, -1,
      30,  -1, -1, -1
  );

  __m256i shuffle_mask_a = _mm256_setr_epi8(
       3,  -1, -1, -1,
       7,  -1, -1, -1,
      11,  -1, -1, -1,
      15,  -1, -1, -1,
      19,  -1, -1, -1,
      23,  -1, -1, -1,
      27,  -1, -1, -1,
      31,  -1, -1, -1
  );
  // clang-format on
  // NOTE: we want to align the last pixels along 8 wide simd
  //  so we clip the first 0-7 pixels based on alignment
  //  then disable the fill mask after the first write in each 8 pixels on x
  S32 fill_width = fill_rect.max_x - fill_rect.min_x;
  S32 fill_align = (fill_width & 7);

  __asm volatile("# LLVM-MCA-BEGIN clip" ::: "memory");
  __m256i startup_clip_mask = _mm256_set1_epi32(-1);

  if (fill_align > 0)
  {
    fill_width += 8 - fill_align;
    fill_rect.min_x = fill_rect.max_x - fill_width;

    __m128i lo = _mm_set1_epi32(-1); // rightmost 128 bits
    __m128i hi = _mm_set1_epi32(-1); // leftmost 128 bits

    S32 adjustment = 8 - fill_align;
    switch (adjustment)
    {
        // clang-format off
      case 1: {lo = _mm_slli_si128(lo, 4);} break;
      case 2: {lo = _mm_slli_si128(lo, 8);} break;
      case 3: {lo = _mm_slli_si128(lo, 12);} break;
      case 4: {lo = _mm_slli_si128(lo, 16);} break;
      case 5: {lo = _mm_slli_si128(lo, 16);hi = _mm_slli_si128(hi, 4);} break;
      case 6: {lo = _mm_slli_si128(lo, 16);hi = _mm_slli_si128(hi, 8);} break;
      case 7: {lo = _mm_slli_si128(lo, 16);hi = _mm_slli_si128(hi, 12);} break;
        // clang-format on
    }
    startup_clip_mask = _mm256_set_m128i(hi, lo);
  }

  __asm volatile("# LLVM-MCA-END" ::: "memory");
  U8* row_in_bytes =
      (U8*)buffer->memory + (fill_rect.min_y * buffer->pitch_in_bytes) + (fill_rect.min_x * BITMAP_BYTES_PER_PIXEL);
  S32 row_advance = buffer->pitch_in_bytes;
  BEGIN_TIMED_BLOCK(Process_Pixel);
  for (S32 y = fill_rect.min_y; y < fill_rect.max_y; y += 1)
  {
    U32* pixel = (U32*)(void*)row_in_bytes;

    __m256 pixel_pos_x =
        _mm256_sub_ps(_mm256_add_ps(_mm256_set1_ps(fill_rect.min_x), _mm256_setr_ps(0, 1, 2, 3, 4, 5, 6, 7)),
                      _mm256_set1_ps(origin.x));
    __m256 pixel_pos_y = _mm256_sub_ps(_mm256_set1_ps((F32)y), _mm256_set1_ps(origin.y));

    __m256i clip_mask = startup_clip_mask;

    for (S32 xi = fill_rect.min_x; xi < fill_rect.max_x; xi += 8)
    {
      __asm volatile("# LLVM-MCA-BEGIN draw_256" ::: "memory");

      // Compute normalized coordinates
      __m256 u = _mm256_add_ps(_mm256_mul_ps(pixel_pos_x, nx_axis_x), _mm256_mul_ps(pixel_pos_y, nx_axis_y));
      __m256 v = _mm256_add_ps(_mm256_mul_ps(pixel_pos_x, ny_axis_x), _mm256_mul_ps(pixel_pos_y, ny_axis_y));

      __m256 u_ge0 = _mm256_cmp_ps(u, zero, _CMP_GE_OS);
      __m256 u_le1 = _mm256_cmp_ps(u, one, _CMP_LE_OS);
      __m256 v_ge0 = _mm256_cmp_ps(v, zero, _CMP_GE_OS);
      __m256 v_le1 = _mm256_cmp_ps(v, one, _CMP_LE_OS);

      __m256i write_mask = (_mm256_and_ps(_mm256_and_ps(u_ge0, u_le1), _mm256_and_ps(v_ge0, v_le1)));
      write_mask = _mm256_and_si256(write_mask, clip_mask);
      // NOTE: disable clip mask after first use
      clip_mask = _mm256_set1_epi32(-1);

      // NOTE: helps a little bit to check this 4% faster
      if (_mm256_movemask_epi8(write_mask))
      {
        // NOTE: load destination
        __m256i dest = _mm256_loadu_si256((__m256i*)pixel);
        __m256 dest_r = _mm256_cvtepi32_ps(_mm256_shuffle_epi8(dest, shuffle_mask_r));
        __m256 dest_g = _mm256_cvtepi32_ps(_mm256_shuffle_epi8(dest, shuffle_mask_g));
        __m256 dest_b = _mm256_cvtepi32_ps(_mm256_shuffle_epi8(dest, shuffle_mask_b));
        __m256 dest_a = _mm256_cvtepi32_ps(_mm256_shuffle_epi8(dest, shuffle_mask_a));

        // Compute texel positions
        __m256 texel_u = _mm256_mul_ps(u, width_m1);
        __m256 texel_v = _mm256_mul_ps(v, height_m1);

        __m256i fetch_x = _mm256_cvttps_epi32(texel_u);
        __m256i fetch_y = _mm256_cvttps_epi32(texel_v);

        // Compute offsets for 4 bilinear samples
        __m256i offset_y = _mm256_mullo_epi32(fetch_y, texture_pitch);
        __m256i offset_x = _mm256_slli_epi32(fetch_x, 2);

        __m256i offset_A = _mm256_add_epi32(offset_y, offset_x);
        __m256i offset_B = _mm256_add_epi32(offset_A, bytes_per_pixel);
        __m256i offset_C = _mm256_add_epi32(offset_A, texture_pitch);
        __m256i offset_D = _mm256_add_epi32(offset_C, bytes_per_pixel);

        // Gather texture pixels (8-wide)
        __m256i sample_A = _mm256_i32gather_epi32(texture_memory, offset_A, 1);
        __m256i sample_B = _mm256_i32gather_epi32(texture_memory, offset_B, 1);

        __m256 texel_Ar = _mm256_cvtepi32_ps(_mm256_shuffle_epi8(sample_A, shuffle_mask_r));
        __m256 texel_Ag = _mm256_cvtepi32_ps(_mm256_shuffle_epi8(sample_A, shuffle_mask_g));
        __m256 texel_Ab = _mm256_cvtepi32_ps(_mm256_shuffle_epi8(sample_A, shuffle_mask_b));
        __m256 texel_Aa = _mm256_cvtepi32_ps(_mm256_shuffle_epi8(sample_A, shuffle_mask_a));

        __m256 texel_Br = _mm256_cvtepi32_ps(_mm256_shuffle_epi8(sample_B, shuffle_mask_r));
        __m256 texel_Bg = _mm256_cvtepi32_ps(_mm256_shuffle_epi8(sample_B, shuffle_mask_g));
        __m256 texel_Bb = _mm256_cvtepi32_ps(_mm256_shuffle_epi8(sample_B, shuffle_mask_b));
        __m256 texel_Ba = _mm256_cvtepi32_ps(_mm256_shuffle_epi8(sample_B, shuffle_mask_a));

#define mm256_Square(a) _mm256_mul_ps(a, a)
        // sRGB → linear
        texel_Ar = mm256_Square(_mm256_mul_ps(inv_255, texel_Ar));
        texel_Ag = mm256_Square(_mm256_mul_ps(inv_255, texel_Ag));
        texel_Ab = mm256_Square(_mm256_mul_ps(inv_255, texel_Ab));
        texel_Aa = _mm256_mul_ps(inv_255, texel_Aa);

        // Repeat for B, C, D
        texel_Br = mm256_Square(_mm256_mul_ps(inv_255, texel_Br));
        texel_Bg = mm256_Square(_mm256_mul_ps(inv_255, texel_Bg));
        texel_Bb = mm256_Square(_mm256_mul_ps(inv_255, texel_Bb));
        texel_Ba = _mm256_mul_ps(inv_255, texel_Ba);

        __m256i sample_C = _mm256_i32gather_epi32(texture_memory, offset_C, 1);
        __m256i sample_D = _mm256_i32gather_epi32(texture_memory, offset_D, 1);

        __m256 texel_Cr = _mm256_cvtepi32_ps(_mm256_shuffle_epi8(sample_C, shuffle_mask_r));
        __m256 texel_Cg = _mm256_cvtepi32_ps(_mm256_shuffle_epi8(sample_C, shuffle_mask_g));
        __m256 texel_Cb = _mm256_cvtepi32_ps(_mm256_shuffle_epi8(sample_C, shuffle_mask_b));
        __m256 texel_Ca = _mm256_cvtepi32_ps(_mm256_shuffle_epi8(sample_C, shuffle_mask_a));

        __m256 texel_Dr = _mm256_cvtepi32_ps(_mm256_shuffle_epi8(sample_D, shuffle_mask_r));
        __m256 texel_Dg = _mm256_cvtepi32_ps(_mm256_shuffle_epi8(sample_D, shuffle_mask_g));
        __m256 texel_Db = _mm256_cvtepi32_ps(_mm256_shuffle_epi8(sample_D, shuffle_mask_b));
        __m256 texel_Da = _mm256_cvtepi32_ps(_mm256_shuffle_epi8(sample_D, shuffle_mask_a));

        texel_Cr = mm256_Square(_mm256_mul_ps(inv_255, texel_Cr));
        texel_Cg = mm256_Square(_mm256_mul_ps(inv_255, texel_Cg));
        texel_Cb = mm256_Square(_mm256_mul_ps(inv_255, texel_Cb));
        texel_Ca = _mm256_mul_ps(inv_255, texel_Ca);

        texel_Dr = mm256_Square(_mm256_mul_ps(inv_255, texel_Dr));
        texel_Dg = mm256_Square(_mm256_mul_ps(inv_255, texel_Dg));
        texel_Db = mm256_Square(_mm256_mul_ps(inv_255, texel_Db));
        texel_Da = _mm256_mul_ps(inv_255, texel_Da);

        // Bilinear interpolation (u,v) - 256-bit
        __m256 fract_u = _mm256_sub_ps(texel_u, _mm256_cvtepi32_ps(fetch_x));
        __m256 fract_v = _mm256_sub_ps(texel_v, _mm256_cvtepi32_ps(fetch_y));

        __m256 ifU = _mm256_sub_ps(one, fract_u);
        __m256 ifV = _mm256_sub_ps(one, fract_v);

        __m256 lerp0 = _mm256_mul_ps(ifV, ifU);
        __m256 lerp1 = _mm256_mul_ps(ifV, fract_u);
        __m256 lerp2 = _mm256_mul_ps(fract_v, ifU);
        __m256 lerp3 = _mm256_mul_ps(fract_v, fract_u);

        // texel_r,g,b,a = lerp
        __m256 texel_r = _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(lerp0, texel_Ar), _mm256_mul_ps(lerp1, texel_Br)),
                                       _mm256_add_ps(_mm256_mul_ps(lerp2, texel_Cr), _mm256_mul_ps(lerp3, texel_Dr)));

        __m256 texel_g = _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(lerp0, texel_Ag), _mm256_mul_ps(lerp1, texel_Bg)),
                                       _mm256_add_ps(_mm256_mul_ps(lerp2, texel_Cg), _mm256_mul_ps(lerp3, texel_Dg)));

        __m256 texel_b = _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(lerp0, texel_Ab), _mm256_mul_ps(lerp1, texel_Bb)),
                                       _mm256_add_ps(_mm256_mul_ps(lerp2, texel_Cb), _mm256_mul_ps(lerp3, texel_Db)));

        __m256 texel_a = _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(lerp0, texel_Aa), _mm256_mul_ps(lerp1, texel_Ba)),
                                       _mm256_add_ps(_mm256_mul_ps(lerp2, texel_Ca), _mm256_mul_ps(lerp3, texel_Da)));

        // Modulate by color
        texel_r = _mm256_mul_ps(texel_r, color_r);
        texel_g = _mm256_mul_ps(texel_g, color_g);
        texel_b = _mm256_mul_ps(texel_b, color_b);
        texel_a = _mm256_mul_ps(texel_a, color_a);

        // Clamp 0..1
        texel_r = _mm256_min_ps(_mm256_max_ps(texel_r, zero), one);
        texel_g = _mm256_min_ps(_mm256_max_ps(texel_g, zero), one);
        texel_b = _mm256_min_ps(_mm256_max_ps(texel_b, zero), one);

        // NOTE: go from sRGB to "linear" brightness space
        dest_r = mm256_Square(_mm256_mul_ps(inv_255, dest_r));
        dest_g = mm256_Square(_mm256_mul_ps(inv_255, dest_g));
        dest_b = mm256_Square(_mm256_mul_ps(inv_255, dest_b));
        dest_a = _mm256_mul_ps(inv_255, dest_a);

        // NOTE: destination blend
        __m256 inv_texel_a = _mm256_sub_ps(one, texel_a);
        __m256 blended_r = _mm256_add_ps(_mm256_mul_ps(inv_texel_a, dest_r), texel_r);
        __m256 blended_g = _mm256_add_ps(_mm256_mul_ps(inv_texel_a, dest_g), texel_g);
        __m256 blended_b = _mm256_add_ps(_mm256_mul_ps(inv_texel_a, dest_b), texel_b);
        __m256 blended_a = _mm256_add_ps(_mm256_mul_ps(inv_texel_a, dest_a), texel_a);

        // Linear → sRGB
        blended_r = _mm256_mul_ps(_mm256_div_ps(one, _mm256_rsqrt_ps(blended_r)), one_255);
        blended_g = _mm256_mul_ps(_mm256_div_ps(one, _mm256_rsqrt_ps(blended_g)), one_255);
        blended_b = _mm256_mul_ps(_mm256_div_ps(one, _mm256_rsqrt_ps(blended_b)), one_255);
        blended_a = _mm256_mul_ps(blended_a, one_255);

        // Convert to integers
        __m256i int_r = _mm256_cvtps_epi32(blended_r);
        __m256i int_g = _mm256_cvtps_epi32(blended_g);
        __m256i int_b = _mm256_cvtps_epi32(blended_b);
        __m256i int_a = _mm256_cvtps_epi32(blended_a);

        // Shift & pack to BGRA
        __m256i shift_r = _mm256_slli_epi32(int_r, 16);
        __m256i shift_g = _mm256_slli_epi32(int_g, 8);
        __m256i shift_b = int_b;
        __m256i shift_a = _mm256_slli_epi32(int_a, 24);

        __m256i bgra = _mm256_or_si256(_mm256_or_si256(shift_r, shift_g), _mm256_or_si256(shift_b, shift_a));

        __m256i out = _mm256_blendv_epi8(dest, bgra, write_mask);

        _mm256_storeu_si256((__m256i*)pixel, out);
        __asm volatile("# LLVM-MCA-END" ::: "memory");
      }

      pixel_pos_x = _mm256_add_ps(pixel_pos_x, _mm256_set1_ps(8.f));
      pixel += 8;
    }
    row_in_bytes += row_advance;
  }

  _MM_SET_ROUNDING_MODE(last_round_mode);

  END_TIMED_BLOCK_COUNTED(Process_Pixel, Rect_Get_Clamped_Area(fill_rect));

  END_TIMED_BLOCK(Draw_Rect_Quickly_Hot);
}

void Draw_Rect_Quickly_Hot128(Loaded_Bitmap* buffer, Vec2 origin, Vec2 x_axis, Vec2 y_axis, Vec4 color,
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

  x_min = CLAMP(x_min, 0, max_width);
  x_max = CLAMP(x_max, 0, max_width);
  y_min = CLAMP(y_min, 0, max_height);
  y_max = CLAMP(y_max, 0, max_height);

  Vec2 nx_axis = inv_x_axis_length_sq * x_axis;
  Vec2 ny_axis = inv_y_axis_length_sq * y_axis;

  __m128 inv_255_4x = _mm_set1_ps(1.f / 255.f);
  __m128 one = _mm_set1_ps(1.f);
  __m128 zero = _mm_setzero_ps();
  __m128 one_255_4x = _mm_set1_ps(255.f);

  __m128 color_r = _mm_set1_ps(color.r);
  __m128 color_g = _mm_set1_ps(color.g);
  __m128 color_b = _mm_set1_ps(color.b);
  __m128 color_a = _mm_set1_ps(color.a);

  __m128 origin_x = _mm_set1_ps(origin.x);
  __m128 origin_y = _mm_set1_ps(origin.y);

  __m128 nx_axis_x = _mm_set1_ps(nx_axis.x);
  __m128 nx_axis_y = _mm_set1_ps(nx_axis.y);
  __m128 ny_axis_x = _mm_set1_ps(ny_axis.x);
  __m128 ny_axis_y = _mm_set1_ps(ny_axis.y);

  __m128 width_m1 = _mm_set1_ps((F32)texture->width - 1);
  __m128 height_m1 = _mm_set1_ps((F32)texture->height - 1);

  __m128i texture_pitch = _mm_set1_epi32(texture->pitch_in_bytes);
  __m128i bytes_per_pixel = _mm_set1_epi32(BITMAP_BYTES_PER_PIXEL);

  U8* texture_memory = (U8*)texture->memory;

  U32 last_round_mode = _MM_GET_ROUNDING_MODE();
  _MM_SET_ROUNDING_MODE(_MM_ROUND_NEAREST);

  __m128i shuffle_mask_b = _mm_setr_epi8(0, -1, -1, -1, // pixel 0 → lane 0
                                         4, -1, -1, -1, // pixel 1 → lane 1
                                         8, -1, -1, -1, // pixel 2 → lane 2
                                         12, -1, -1, -1 // pixel 3 → lane 3
  );

  __m128i shuffle_mask_g = _mm_setr_epi8(1, -1, -1, -1, // pixel 0 → lane 0
                                         5, -1, -1, -1, // pixel 1 → lane 1
                                         9, -1, -1, -1, // pixel 2 → lane 2
                                         13, -1, -1, -1 // pixel 3 → lane 3
  );

  __m128i shuffle_mask_r = _mm_setr_epi8(2, -1, -1, -1,  // pixel 0 → lane 0
                                         6, -1, -1, -1,  // pixel 1 → lane 1
                                         10, -1, -1, -1, // pixel 2 → lane 2
                                         14, -1, -1, -1  // pixel 3 → lane 3
  );

  __m128i shuffle_mask_a = _mm_setr_epi8(3, -1, -1, -1,  // pixel 0 → lane 0
                                         7, -1, -1, -1,  // pixel 1 → lane 1
                                         11, -1, -1, -1, // pixel 2 → lane 2
                                         15, -1, -1, -1  // pixel 3 → lane 3
  );

  U8* row_in_bytes = (U8*)buffer->memory + (y_min * buffer->pitch_in_bytes) + (x_min * BITMAP_BYTES_PER_PIXEL);
  BEGIN_TIMED_BLOCK(Process_Pixel);
  for (S32 y = y_min; y <= y_max; y++)
  {
    U32* pixel = (U32*)(void*)row_in_bytes;
    __m128 pixel_pos_x = _mm_sub_ps(_mm_setr_ps(x_min + 0, x_min + 1, x_min + 2, x_min + 3), origin_x);
    __m128 pixel_pos_y = _mm_sub_ps(_mm_set1_ps((F32)y), origin_y);

    for (S32 xi = x_min; xi <= x_max; xi += 4)
    {
      __asm volatile("# LLVM-MCA-BEGIN draw" ::: "memory");
      __m128 u = _mm_add_ps(_mm_mul_ps(pixel_pos_x, nx_axis_y), _mm_mul_ps(pixel_pos_x, nx_axis_x));
      __m128 v = _mm_add_ps(_mm_mul_ps(pixel_pos_y, ny_axis_x), _mm_mul_ps(pixel_pos_y, ny_axis_y));

      __m128 u_ge0 = _mm_cmpge_ps(u, zero); // u >= 0
      __m128 u_le1 = _mm_cmple_ps(u, one);  // u <= 1
      __m128 v_ge0 = _mm_cmpge_ps(v, zero); // v >= 0
      __m128 v_le1 = _mm_cmple_ps(v, one);  // v <=1

      __m128i write_mask = _mm_castps_si128(_mm_and_ps(_mm_and_ps(u_ge0, u_le1), _mm_and_ps(v_ge0, v_le1)));

      // NOTE: helps a little bit to check this 4% faster
      if (_mm_movemask_epi8(write_mask))
      {

        // NOTE: load destination
        __m128i dest = _mm_loadu_si128((__m128i*)pixel);
        __m128 dest_r = _mm_cvtepi32_ps(_mm_shuffle_epi8(dest, shuffle_mask_r));
        __m128 dest_g = _mm_cvtepi32_ps(_mm_shuffle_epi8(dest, shuffle_mask_g));
        __m128 dest_b = _mm_cvtepi32_ps(_mm_shuffle_epi8(dest, shuffle_mask_b));
        __m128 dest_a = _mm_cvtepi32_ps(_mm_shuffle_epi8(dest, shuffle_mask_a));

        __m128 texel_u = _mm_mul_ps(u, width_m1);
        __m128 texel_v = _mm_mul_ps(v, height_m1);

        __m128i fetch_x = _mm_cvttps_epi32(texel_u);
        __m128i fetch_y = _mm_cvttps_epi32(texel_v);

        // NOTE: bilinear sample
        // offset_y = pixel_y * pitch
        __m128i offset_y = _mm_mullo_epi32(fetch_y, texture_pitch);
        // bytes per pixel = 4 = shift left 2
        __m128i offset_x = _mm_slli_epi32(fetch_x, 2);

        __m128i offset_A = _mm_add_epi32(offset_y, offset_x);
        __m128i offset_B = _mm_add_epi32(offset_A, bytes_per_pixel);

        // NOTE: gather the 4 quads (4 pixels)
        __m128i sample_A = _mm_i32gather_epi32(texture_memory, offset_A, 1);
        __m128i sample_B = _mm_i32gather_epi32(texture_memory, offset_B, 1);

        __m128 texel_Ar = _mm_cvtepi32_ps(_mm_shuffle_epi8(sample_A, shuffle_mask_r));
        __m128 texel_Ag = _mm_cvtepi32_ps(_mm_shuffle_epi8(sample_A, shuffle_mask_g));
        __m128 texel_Ab = _mm_cvtepi32_ps(_mm_shuffle_epi8(sample_A, shuffle_mask_b));
        __m128 texel_Aa = _mm_cvtepi32_ps(_mm_shuffle_epi8(sample_A, shuffle_mask_a));

        __m128 texel_Br = _mm_cvtepi32_ps(_mm_shuffle_epi8(sample_B, shuffle_mask_r));
        __m128 texel_Bg = _mm_cvtepi32_ps(_mm_shuffle_epi8(sample_B, shuffle_mask_g));
        __m128 texel_Bb = _mm_cvtepi32_ps(_mm_shuffle_epi8(sample_B, shuffle_mask_b));
        __m128 texel_Ba = _mm_cvtepi32_ps(_mm_shuffle_epi8(sample_B, shuffle_mask_a));

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

        __m128i offset_C = _mm_add_epi32(offset_A, texture_pitch);
        __m128i offset_D = _mm_add_epi32(offset_C, bytes_per_pixel);
        __m128i sample_C = _mm_i32gather_epi32(texture_memory, offset_C, 1);
        __m128i sample_D = _mm_i32gather_epi32(texture_memory, offset_D, 1);

        __m128 texel_Cr = _mm_cvtepi32_ps(_mm_shuffle_epi8(sample_C, shuffle_mask_r));
        __m128 texel_Cg = _mm_cvtepi32_ps(_mm_shuffle_epi8(sample_C, shuffle_mask_g));
        __m128 texel_Cb = _mm_cvtepi32_ps(_mm_shuffle_epi8(sample_C, shuffle_mask_b));
        __m128 texel_Ca = _mm_cvtepi32_ps(_mm_shuffle_epi8(sample_C, shuffle_mask_a));

        __m128 texel_Dr = _mm_cvtepi32_ps((_mm_shuffle_epi8(sample_D, shuffle_mask_r)));
        __m128 texel_Dg = _mm_cvtepi32_ps((_mm_shuffle_epi8(sample_D, shuffle_mask_g)));
        __m128 texel_Db = _mm_cvtepi32_ps((_mm_shuffle_epi8(sample_D, shuffle_mask_b)));
        __m128 texel_Da = _mm_cvtepi32_ps((_mm_shuffle_epi8(sample_D, shuffle_mask_a)));

        texel_Cr = mm_Square(_mm_mul_ps(inv_255_4x, texel_Cr));
        texel_Cg = mm_Square(_mm_mul_ps(inv_255_4x, texel_Cg));
        texel_Cb = mm_Square(_mm_mul_ps(inv_255_4x, texel_Cb));
        texel_Ca = _mm_mul_ps(inv_255_4x, texel_Ca);

        texel_Dr = mm_Square(_mm_mul_ps(inv_255_4x, texel_Dr));
        texel_Dg = mm_Square(_mm_mul_ps(inv_255_4x, texel_Dg));
        texel_Db = mm_Square(_mm_mul_ps(inv_255_4x, texel_Db));
        texel_Da = _mm_mul_ps(inv_255_4x, texel_Da);

        // NOTE: bilinear texture blend
        __m128 fract_u = _mm_sub_ps(texel_u, _mm_cvtepi32_ps(fetch_x));
        __m128 fract_v = _mm_sub_ps(texel_v, _mm_cvtepi32_ps(fetch_y));

        __m128 ifU = _mm_sub_ps(one, fract_u);
        __m128 ifV = _mm_sub_ps(one, fract_v);
        __m128 lerp0 = _mm_mul_ps(ifV, ifU);
        __m128 lerp1 = _mm_mul_ps(ifV, fract_u);
        __m128 lerp2 = _mm_mul_ps(fract_v, ifU);
        __m128 lerp3 = _mm_mul_ps(fract_v, fract_u);

        __m128 texel_r = _mm_add_ps(_mm_add_ps(_mm_mul_ps(lerp0, texel_Ar), _mm_mul_ps(lerp1, texel_Br)),
                                    _mm_add_ps(_mm_mul_ps(lerp2, texel_Cr), _mm_mul_ps(lerp3, texel_Dr)));
        __m128 texel_g = _mm_add_ps(_mm_add_ps(_mm_mul_ps(lerp0, texel_Ag), _mm_mul_ps(lerp1, texel_Bg)),
                                    _mm_add_ps(_mm_mul_ps(lerp2, texel_Cg), _mm_mul_ps(lerp3, texel_Dg)));
        __m128 texel_b = _mm_add_ps(_mm_add_ps(_mm_mul_ps(lerp0, texel_Ab), _mm_mul_ps(lerp1, texel_Bb)),
                                    _mm_add_ps(_mm_mul_ps(lerp2, texel_Cb), _mm_mul_ps(lerp3, texel_Db)));
        __m128 texel_a = _mm_add_ps(_mm_add_ps(_mm_mul_ps(lerp0, texel_Aa), _mm_mul_ps(lerp1, texel_Ba)),
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
        __m128 blended_r = _mm_add_ps(_mm_mul_ps(inv_texel_a, dest_r), texel_r);
        __m128 blended_g = _mm_add_ps(_mm_mul_ps(inv_texel_a, dest_g), texel_g);
        __m128 blended_b = _mm_add_ps(_mm_mul_ps(inv_texel_a, dest_b), texel_b);
        __m128 blended_a = _mm_add_ps(_mm_mul_ps(inv_texel_a, dest_a), texel_a);

        // NOTE: go from "linear" brigthness space to sRGB
        // blended_r = _mm_mul_ps(_mm_sqrt_ps(blended_r), one_255_4x);
        // blended_g = _mm_mul_ps(_mm_sqrt_ps(blended_g), one_255_4x);
        // blended_b = _mm_mul_ps(_mm_sqrt_ps(blended_b), one_255_4x);
        blended_r = _mm_mul_ps(_mm_div_ps(one, _mm_rsqrt_ps(blended_r)), one_255_4x);
        blended_g = _mm_mul_ps(_mm_div_ps(one, _mm_rsqrt_ps(blended_g)), one_255_4x);
        blended_b = _mm_mul_ps(_mm_div_ps(one, _mm_rsqrt_ps(blended_b)), one_255_4x);
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

        __m128i out = _mm_blendv_epi8(dest, bgra8, write_mask);

        _mm_storeu_si128((__m128i*)pixel, out);
        __asm volatile("# LLVM-MCA-END" ::: "memory");
      }

      pixel_pos_x = _mm_add_ps(pixel_pos_x, _mm_set1_ps(4.f));
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
