#ifndef COLOR_H
#define COLOR_H

typedef Vec4_U8 Color4;
typedef Vec4_F32 Color4F;
inline Color4F Color4F_from_RGB255(U32 r, U32 g, U32 b);

#define rgb(r, g, b) Color4F_from_RGB255(r, g, b)

global const Color4F Color_Pastel_Red = rgb(254, 128, 128);
global const Color4F Color_Pastel_Green = rgb(128, 254, 128);
global const Color4F Color_Pastel_Blue = rgb(128, 128, 254);
global const Color4F Color_Pastel_Yellow = rgb(254, 254, 128);
global const Color4F Color_Pastel_Cyan = rgb(128, 254, 254);
global const Color4F Color_Pastel_Pink = rgb(254, 128, 254);

inline Color4F Color4F_from_RGB255(U32 r, U32 g, U32 b)
{
  Color4F result;
  result.r = F32(r) / 255.f;
  result.g = F32(g) / 255.f;
  result.b = F32(b) / 255.f;
  result.a = 1.f;
  return result;
}

internal inline Color4F Color_To_Premult(Color4F color)
{
  Color4F result;
  result.r = color.r * color.a;
  result.g = color.g * color.a;
  result.b = color.b * color.a;
  result.a = color.a;
  return result;
}

// NOTE: blend alpha
internal inline U8 blend_alpha_U8(U8 dest_alpha, U8 src_alpha)
{
  U8 result;
  result = src_alpha + (dest_alpha * (255 - src_alpha) + 127) / 255;
  return result;
}

internal inline F32 blend_alpha_F32(F32 dest_alpha, F32 src_alpha)
{
  F32 result;
  result = src_alpha + (dest_alpha * (1.0f - src_alpha));
  return result;
}

// NOTE: normal blend premultiplied alpha
// this is called "source over" alpha compositing.
internal inline U8 blend_normal_U8(U8 dest_color, U8 dest_alpha, U8 src_color, U8 src_alpha)
{
  U8 result;
  result = (src_color * src_alpha + (dest_color * dest_alpha * (255 - src_alpha) + 127) / 255) / 255;
  return result;
}

// NOTE: Assumes everything is premultiplied alpha when loaded in
internal inline F32 blend_normal_F32(F32 dest_color, F32 dest_alpha, F32 src_color, F32 src_alpha)
{
  F32 result;
  result = src_color + (dest_color * (1.0f - src_alpha));
  return result;
}

internal inline Color4 blend_normal_Color4(Color4 dest, Color4 src, F32 c_alpha)
{
  // NOTE: about 1.7x-2x faster using floats over ints
  // NOTE: c_alpha must be premultiplied in to src as well
  Color4 result;
  F32 dest_alpha = (F32)dest.argb.a / 255.0f;
  F32 src_alpha = c_alpha * ((F32)src.argb.a / 255.0f);
  F32 inv_src_alpha = (1.f - src_alpha);

  ASSERT(src.argb.r <= src.argb.a + 1 && src.argb.g <= src.argb.a + 1 && src.argb.b <= src.argb.a + 1);

  result.argb.a = (U8)(255.f * (src_alpha + (dest_alpha * inv_src_alpha)));
  result.argb.r = (U8)(c_alpha * (F32)src.argb.r + inv_src_alpha * (F32)dest.argb.r);
  result.argb.g = (U8)(c_alpha * (F32)src.argb.g + inv_src_alpha * (F32)dest.argb.g);
  result.argb.b = (U8)(c_alpha * (F32)src.argb.b + inv_src_alpha * (F32)dest.argb.b);

  return result;
}

#endif // COLOR_H
