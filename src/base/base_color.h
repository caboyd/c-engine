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

// NOTE: normal blend premultiplied alpha
internal inline U8 blend_normal_U8(U8 dest_color, U8 dest_alpha, U8 src_color, U8 src_alpha)
{
  U8 result;
  result = (src_color * src_alpha + (dest_color * dest_alpha * (255 - src_alpha) + 127) / 255) / 255;
  return result;
}

internal inline Color4 blend_normal_Color4(Color4 dest, Color4 src)
{
  Color4 result;

  result.bgra.a = blend_alpha_U8(dest.bgra.a, src.bgra.a);
  result.bgra.r = blend_normal_U8(dest.bgra.r, dest.bgra.a, src.bgra.r, src.bgra.a);
  result.bgra.g = blend_normal_U8(dest.bgra.g, dest.bgra.a, src.bgra.g, src.bgra.a);
  result.bgra.b = blend_normal_U8(dest.bgra.b, dest.bgra.a, src.bgra.b, src.bgra.a);

  return result;
}

#endif // BASE_COLOR_H
