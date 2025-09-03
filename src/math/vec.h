#ifndef VEC_H
#define VEC_H

internal inline Vec2 Vec2_Add(Vec2 a, Vec2 b)
{
  Vec2 result;
  result.x = a.x + b.x;
  result.y = a.y + b.y;
  return result;
}

internal inline Vec2 Vec2_Addf(Vec2 a, F32 b)
{
  Vec2 result;
  result.x = a.x + b;
  result.y = a.y + b;
  return result;
}

internal inline Vec2 Vec2_Sub(Vec2 a, Vec2 b)
{
  Vec2 result;
  result.x = a.x - b.x;
  result.y = a.y - b.y;
  return result;
}

internal inline Vec2 Vec2_Subf(Vec2 a, F32 b)
{
  Vec2 result;
  result.x = a.x - b;
  result.y = a.y - b;
  return result;
}

internal inline Vec2 Vec2_Negate(Vec2 a)
{
  Vec2 result;
  result.x = -a.x;
  result.y = -a.y;
  return result;
}

internal inline Vec2 Vec2_AddScaled(Vec2 a, Vec2 b, F32 b_scalar)
{
  Vec2 result;
  result.x = a.x + b.x * b_scalar;
  result.y = a.y + b.y * b_scalar;
  return result;
}

internal inline Vec2 Vec2_Scale(Vec2 a, F32 scale)
{
  Vec2 result;
  result.x = a.x * scale;
  result.y = a.y * scale;
  return result;
}

internal inline F32 Vec2_Length_Sq(Vec2 a)
{
  F32 result = a.x * a.x + a.y * a.y;
  return result;
}

internal inline Vec2 Vec2_Normalize(Vec2 a)
{
  Vec2 result = a;
  F32 length_sq = a.x * a.x + a.y * a.y;
  ASSERT(length_sq > 0);
  F32 length = Sqrtf(length_sq);
  result.x /= length;
  result.y /= length;
  return result;
}
internal inline Vec2 Vec2_NormalizeSafe(Vec2 a)
{
  Vec2 result = a;
  F32 length_sq = a.x * a.x + a.y * a.y;
  if (length_sq > 0)
  {
    F32 length = Sqrtf(length_sq);
    result.x /= length;
    result.y /= length;
  }
  return result;
}

internal inline F32 Vec2_Dot(Vec2 a, Vec2 b)
{
  F32 result;
  result = a.x * b.x + a.y * b.y;
  return result;
}

#endif
