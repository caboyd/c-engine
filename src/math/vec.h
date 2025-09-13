#ifndef VEC_H
#define VEC_H

inline Vec2 vec2(F32 x, F32 y)
{
  Vec2 result = {{x, y}};
  return result;
}

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

inline Vec2 operator*(Vec2 a, Vec2 b)
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
inline F32 Vec2_Length(Vec2 a)
{
  F32 result = a.x * a.x + a.y * a.y;
  result = Sqrt_F32(result);
  return result;
}

inline Vec2 Vec2_Normalize(Vec2 a)
{
  Vec2 result = a;
  F32 length = Vec2_Length(a);
  result /= length;
  return result;
}
inline Vec2 Vec2_NormalizeSafe(Vec2 a)
{
  Vec2 result = a;
  F32 length = Vec2_Length(a);
  if (length > 0)
  {
    result /= length;
  }
  return result;
}

inline F32 Vec2_Dot(Vec2 a, Vec2 b)
{
  F32 result;
  result = a.x * b.x + a.y * b.y;
  return result;
}
inline Vec2 Vec2_Slide(Vec2 v, Vec2 unit_normal)
{
  Vec2 result;
  // NOTE: Once to project v onto tangent of normal, (slide along wall)
  Vec2 v_proj_n = Vec2_Dot(v, unit_normal) * unit_normal;
  result = v - v_proj_n;
  return result;
}
inline Vec2 Vec2_SlidePreserveMomentum(Vec2 v, Vec2 unit_normal)
{
  Vec2 result;
  // NOTE: Once to project v onto tangent of normal, (slide along wall)
  Vec2 v_proj_n = Vec2_Dot(v, unit_normal) * unit_normal;
  result = v - v_proj_n;
  if (Vec2_Length_Sq(result) > 0)
  {
    result = Vec2_Normalize(result) * Vec2_Length(v);
  }
  return result;
}
inline Vec2 Vec2_SlideSafe(Vec2 v, Vec2 normal)
{
  Vec2 result;
  Vec2 unit_normal = Vec2_NormalizeSafe(normal);
  result = Vec2_Slide(v, unit_normal);
  return result;
}
inline Vec2 Vec2_SlideSafePreserveMomentum(Vec2 v, Vec2 normal)
{
  Vec2 result;
  Vec2 unit_normal = Vec2_NormalizeSafe(normal);
  result = Vec2_SlidePreserveMomentum(v, unit_normal);
  return result;
}

inline Vec2 Vec2_Reflect(Vec2 v, Vec2 unit_normal)
{
  Vec2 result;
  // NOTE: Once to project v onto tangent of normal, (slide along wall)
  // twice to reflect v into direction of normal (bounce off wall)
  Vec2 twice_v_proj_n = 2 * Vec2_Dot(v, unit_normal) * unit_normal;
  result = v - twice_v_proj_n;
  return result;
}

inline Vec2 Vec2_ReflectSafe(Vec2 v, Vec2 normal)
{
  Vec2 result;
  Vec2 unit_normal = Vec2_NormalizeSafe(normal);
  result = Vec2_Reflect(v, unit_normal);
  return result;
}
inline Vec2 Vec2_Snap_Cardinal(Vec2 v)
{
  Vec2 result;
  if (Abs_F32(v.x) > Abs_F32(v.y))
  {
    if (v.x > 0)
    {
      result = {{1.f, 0.f}};
    }
    else
    {
      result = {{-1.f, 0.f}};
    }
  }
  else
  {
    if (v.y > 0)
    {
      result = {{0.f, 1.0f}};
    }
    else
    {
      result = {{0.f, -1.f}};
    }
  }
  return result;
}

#endif
