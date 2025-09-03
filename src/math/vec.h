#ifndef VEC_H
#define VEC_H

inline Vec2 operator-(Vec2 a)
{
  Vec2 result;

  result.x = -a.x;
  result.y = -a.y;

  return result;
}

//
// Scalar operations
//

inline Vec2 operator*(Vec2 a, F32 s)
{
  Vec2 result;

  result.x = a.x * s;
  result.y = a.y * s;

  return result;
}

inline Vec2 operator*(F32 s, Vec2 a)
{
  Vec2 result = a * s;

  return result;
}

inline Vec2 operator/(Vec2 a, F32 s)
{
  Vec2 result;

  result.x = a.x / s;
  result.y = a.y / s;

  return result;
}

//
//  Vector element-wise operations
//

inline Vec2 operator+(Vec2 a, Vec2 b)
{
  Vec2 result;

  result.x = a.x + b.x;
  result.y = a.y + b.y;

  return result;
}

inline Vec2 operator-(Vec2 a, Vec2 b)
{
  Vec2 result;

  result.x = a.x - b.x;
  result.y = a.y - b.y;

  return result;
}

Vec2 operator*(Vec2 a, Vec2 b)
{
  Vec2 result;

  result.x = a.x * b.x;
  result.y = a.y * b.y;

  return result;
}

//
//   Compound Operations
//
inline Vec2& operator*=(Vec2& a, F32 s)
{
  a = a * s;

  return a;
}

inline Vec2& operator/=(Vec2& a, F32 s)
{
  a = a / s;

  return a;
}
inline Vec2& operator+=(Vec2& a, Vec2 b)
{
  a = a + b;

  return a;
}
inline Vec2& operator-=(Vec2& a, Vec2 b)
{
  a = a - b;

  return a;
}

inline Vec2& operator*=(Vec2& a, Vec2 b)
{
  a = b * a;

  return a;
}

//
//  Vec2 Functions
//

inline F32 Vec2_Length_Sq(Vec2 a)
{
  F32 result = a.x * a.x + a.y * a.y;
  return result;
}

inline Vec2 Vec2_Normalize(Vec2 a)
{
  Vec2 result = a;
  F32 length_sq = a.x * a.x + a.y * a.y;
  ASSERT(length_sq > 0);
  F32 length = Sqrtf(length_sq);
  result.x /= length;
  result.y /= length;
  return result;
}
inline Vec2 Vec2_NormalizeSafe(Vec2 a)
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

inline F32 Vec2_Dot(Vec2 a, Vec2 b)
{
  F32 result;
  result = a.x * b.x + a.y * b.y;
  return result;
}

#endif
