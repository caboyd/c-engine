
#include "hot.h"

void Draw_BMP_Subset_Hot(Game_Offscreen_Buffer* buffer, Loaded_Bitmap* bitmap, F32 x, F32 y, S32 bmp_x_offset,
                         S32 bmp_y_offset, S32 bmp_x_dim, S32 bmp_y_dim, F32 scale, B32 alpha_blend, F32 c_alpha)
{
  if (!bitmap || !bitmap->pixels)
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

  U8* dest_row_in_bytes = (U8*)buffer->memory + (min_y * buffer->pitch_in_bytes) + (min_x * buffer->bytes_per_pixel);

  for (S32 y_index = min_y; y_index < max_y; y_index++)
  {
    U8* pixel = dest_row_in_bytes;
    S32 y_src_offset = Trunc_F32_S32((F32)(y_index - min_y + y_draw_offset) / scale);
    // NOTE: flip the bmp to render into buffer top to bottom
    S32 y_src = (bitmap->height - 1) - y_src_offset;

    for (S32 x_index = min_x; x_index < max_x; x_index++)
    {
      S32 x_src = Trunc_F32_S32((F32)(x_index - min_x + x_draw_offset) / scale);
      ASSERT(x_src < bitmap->width);
      ASSERT(y_src < bitmap->height);

      U8* src = (U8*)(void*)(bitmap->pixels + y_src * bitmap->width + x_src);
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
}

void Draw_Rectf_Hot(Game_Offscreen_Buffer* buffer, F32 fmin_x, F32 fmin_y, F32 fmax_x, F32 fmax_y, F32 r, F32 g, F32 b,
                    B32 wire_frame)
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

  U32 color = (255u << 24) | (F32_to_U32_255(r) << 16) | (F32_to_U32_255(g) << 8) | (F32_to_U32_255(b) << 0);

  U8* row_in_bytes = (U8*)buffer->memory + (min_y * buffer->pitch_in_bytes) + (min_x * buffer->bytes_per_pixel);

  for (S32 y = min_y; y < max_y; y++)
  {
    U32* pixel = (U32*)(void*)row_in_bytes;
    for (S32 x = min_x; x < max_x; x++)
    {
      if (wire_frame)
      {
        if ((y == min_y || y == max_y - 1) || (x == min_x || x == max_x - 1))
        {
          *pixel++ = color;
        }
        else
        {
          pixel++;
        }
      }
      else
      {
        *pixel++ = color;
      }
    }
    row_in_bytes += buffer->pitch_in_bytes;
  }
}
