#ifndef BASE_COLOR_H
#define BASE_COLOR_H

typedef Vec4_U8 Color4;
typedef Vec4_F32 Color4F;

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
internal inline F32 blend_normal_F32(F32 dest_color, F32 dest_alpha, F32 src_color, F32 src_alpha)
{
  F32 result;
  result = src_color * src_alpha + (dest_color * dest_alpha * (1.0f - src_alpha));
  return result;
}

internal inline Color4 blend_normal_Color4(Color4 dest, Color4 src)
{
  Color4 result;
#if 0

  result.argb.a = blend_alpha_U8(dest.argb.a, src.argb.a);
  result.argb.r = blend_normal_U8(dest.argb.r, dest.argb.a, src.argb.r, src.argb.a);
  result.argb.g = blend_normal_U8(dest.argb.g, dest.argb.a, src.argb.g, src.argb.a);
  result.argb.b = blend_normal_U8(dest.argb.b, dest.argb.a, src.argb.b, src.argb.a);
#else
  //NOTE: about 1.7x-2x faster
  Color4F color;
  F32 dest_alpha = (F32)dest.argb.a / 255.0f;
  F32 src_alpha = (F32)src.argb.a / 255.0f;

  color.a = blend_alpha_F32(dest_alpha, src_alpha);
  color.r = blend_normal_F32((F32)dest.argb.r / 255.f, dest_alpha, (F32)src.argb.r / 255.f, src_alpha);
  color.g = blend_normal_F32((F32)dest.argb.g / 255.f, dest_alpha, (F32)src.argb.g / 255.f, src_alpha);
  color.b = blend_normal_F32((F32)dest.argb.b / 255.f, dest_alpha, (F32)src.argb.b / 255.f, src_alpha);

  result.argb.a = (U8)(color.a * 255.f);
  result.argb.r = (U8)(color.r * 255.f);
  result.argb.g = (U8)(color.g * 255.f);
  result.argb.b = (U8)(color.b * 255.f);

#endif
  return result;
}

#endif // BASE_COLOR_H
