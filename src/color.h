#ifndef COLOR_H
#define COLOR_H

typedef Vec4_U8 Color4;
typedef Vec4 Color4F;
inline Color4F Color4F_from_RGB255(U32 r, U32 g, U32 b, U32 a = 255);

#define rgb(r, g, b) Color4F_from_RGB255(r, g, b)

global const Color4F Color_Pastel_Red = rgb(254, 128, 128);
global const Color4F Color_Pastel_Green = rgb(128, 254, 128);
global const Color4F Color_Pastel_Blue = rgb(128, 128, 254);
global const Color4F Color_Pastel_Yellow = rgb(254, 254, 128);
global const Color4F Color_Pastel_Cyan = rgb(128, 254, 254);
global const Color4F Color_Pastel_Pink = rgb(254, 128, 254);

inline Color4F Color4F_from_RGB255(U32 r, U32 g, U32 b, U32 a)
{
  Color4F result;
  result.r = F32(r) / 255.f;
  result.g = F32(g) / 255.f;
  result.b = F32(b) / 255.f;
  result.a = F32(a) / 255.f;
  return result;
}

inline Color4F Color4_To_Color4F(Color4 c)
{
  Color4F result;
  result.r = F32(c.argb.r) / 255.f;
  result.g = F32(c.argb.g) / 255.f;
  result.b = F32(c.argb.b) / 255.f;
  result.a = F32(c.argb.a) / 255.f;
  return result;
}

inline Color4F Color4_SRGB_To_Color4F_Linear(Color4 c)
{
  Color4F result;
  F32 inv_255 = 1.f / 255.f;
  result.r = Square(inv_255 * c.argb.r);
  result.g = Square(inv_255 * c.argb.g);
  result.b = Square(inv_255 * c.argb.b);
  result.a = (inv_255 * c.argb.a);

  return result;
}

inline Color4 Color4F_Linear_To_Color4_SRGB(Color4F c)
{
  Color4 result;
  result.argb.a = (U8)F32_to_U32_255(c.a);
  result.argb.r = (U8)F32_to_U32_255(Sqrt_F32(c.r));
  result.argb.g = (U8)F32_to_U32_255(Sqrt_F32(c.g));
  result.argb.b = (U8)F32_to_U32_255(Sqrt_F32(c.b));
  ;
  return result;
}

inline U8 Lerp_RGB255(U8 a, U8 b, F32 t)
{
  ASSERT(t >= 0.f && t <= 1.f);
  U8 result = (U8)((1.f - t) * (F32)a + t * (F32)b);
  return result;
}

inline Color4 Color4F_To_Color4(Color4F color)
{
  Color4 result;
  result.u32 = (F32_to_U32_255(color.a) << 24) | (F32_to_U32_255(color.r) << 16) | (F32_to_U32_255(color.g) << 8) |
               (F32_to_U32_255(color.b) << 0);
  return result;
}

inline Color4F Color4F_SRGB_To_Linear(Color4F color)
{
  Color4F result;
  result.r = Square(color.r);
  result.g = Square(color.g);
  result.b = Square(color.b);
  result.a = color.a;
  return result;
}

inline Color4F Color4F_Linear_To_SRGB(Color4F color)
{
  Color4F result;
  result.r = Sqrt_F32(color.r);
  result.g = Sqrt_F32(color.g);
  result.b = Sqrt_F32(color.b);
  result.a = color.a;
  return result;
}

inline Color4F Color4F_Linear_Premult(Color4F color)
{
  Color4F result = color;
  result.rgb *= result.a;
  return result;
}

inline Color4F Color4F_SRGB_Premult(Color4F color)
{
  Color4F result = Color4F_SRGB_To_Linear(color);
  result.rgb *= result.a;
  result = Color4F_Linear_To_SRGB(color);
  return result;
}

// NOTE: blend alpha
// inline U8 blend_alpha_U8(U8 dest_alpha, U8 src_alpha)
// {
//   U8 result;
//   result = src_alpha + (dest_alpha * (255 - src_alpha) + 127) / 255;
//   return result;
// }
//
// inline F32 blend_alpha_F32(F32 dest_alpha, F32 src_alpha)
// {
//   F32 result;
//   result = src_alpha + (dest_alpha * (1.0f - src_alpha));
//   return result;
// }
//
// // NOTE: normal blend premultiplied alpha
// // this is called "source over" alpha compositing.
// inline U8 blend_normal_U8(U8 dest_color, U8 dest_alpha, U8 src_color, U8 src_alpha)
// {
//   U8 result;
//   result = (src_color * src_alpha + (dest_color * dest_alpha * (255 - src_alpha) + 127) / 255) / 255;
//   return result;
// }
//
// // NOTE: Assumes everything is premultiplied alpha when loaded in
// inline F32 blend_normal_F32(F32 dest_color, F32 dest_alpha, F32 src_color, F32 src_alpha)
// {
//   F32 result;
//   result = src_color + (dest_color * (1.0f - src_alpha));
//   return result;
// }
//
inline Color4 Color4_Blend_Normal(Color4 dest, Color4 src, F32 c_alpha = 1.f)
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

inline Color4F Color4F_Blend_Normal(Color4F dest, Color4F src, Color4F c_color)

{
  // NOTE: premultiply color
  c_color.rgb *= c_color.a;

  F32 src_alpha = c_color.a * src.a;
  F32 inv_src_alpha = (1.f - src_alpha);

  F32 epsilon = 0.001f;
  ASSERT(src.r <= src.a + epsilon && src.g <= src.a + epsilon && src.b <= src.a + epsilon);

  Color4F result = c_color * src + inv_src_alpha * dest;

  return result;
}

inline Color4 Color4_Blend_Bilinear(Color4 a, Color4 b, F32 t)
{
  Color4 result;

  result.argb.a = Lerp_RGB255(a.argb.a, b.argb.a, t);
  result.argb.r = Lerp_RGB255(a.argb.r, b.argb.r, t);
  result.argb.g = Lerp_RGB255(a.argb.g, b.argb.g, t);
  result.argb.b = Lerp_RGB255(a.argb.b, b.argb.b, t);

  return result;
}

#endif // COLOR_H
