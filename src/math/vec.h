#ifndef VEC_H
#define VEC_H

typedef union
{
  F32 e[2];

  struct
  {
    F32 x, y;
  };

  struct
  {
    F32 u, v;
  };
} Vec2;

typedef union
{
  F32 e[3];

  struct
  {
    union
    {
      Vec2 xy;
      Vec2 rg;
      Vec2 uv;

      struct
      {
        F32 x, y;
      };

      struct
      {
        F32 r, g;
      };

      struct
      {
        F32 u, v;
      };
    };

    union
    {
      F32 b;
      F32 z;
      F32 w;
    };
  };
} Vec3;

typedef union
{
  U32 u32;
  U8 e[4];

  struct
  {
    // NOTE: alpha is higest byte in argb
    U8 b, g, r, a;
  } argb;
} Vec4_U8;

typedef union
{
  F32 e[4];

  struct
  {
    Vec2 xy;
    Vec2 zw;
  };

  struct
  {
    union
    {
      Vec3 xyz;
      Vec3 rgb;

      struct
      {
        F32 x, y, z;
      };

      struct
      {
        F32 r, g, b;
      };
    };

    union
    {
      F32 a;
      F32 w;
    };
  };
} Vec4_F32;

typedef Vec4_F32 Vec4;

//
//  NOTE: VEC2 Vec2 vec2
//

inline Vec2 vec2(F32 x, F32 y)
{
  Vec2 result;
  result.x = x;
  result.y = y;
  return result;
}

inline Vec2 vec2i(S32 x, S32 y)
{
  Vec2 result = {{(F32)x, (F32)y}};
  return result;
}

inline Vec2 vec2(F32 v)
{
  Vec2 result;
  result.x = v;
  result.y = v;
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

inline Vec2 Vec_Clamp01(Vec2 a)
{
  Vec2 result;
  result.x = Clamp01(a.x);
  result.y = Clamp01(a.y);
  return result;
}

inline F32 Vec_Length_Sq(Vec2 a)
{
  F32 result = a.x * a.x + a.y * a.y;
  return result;
}

inline F32 Vec_Length(Vec2 a)
{
  F32 result = a.x * a.x + a.y * a.y;
  result = Sqrt_F32(result);
  return result;
}

inline Vec2 Vec_Normalize(Vec2 a)
{
  Vec2 result = a;
  F32 length = Vec_Length(a);
  result /= length;
  return result;
}

inline Vec2 Vec_NormalizeSafe(Vec2 a)
{
  Vec2 result = a;
  F32 length = Vec_Length(a);
  if (length > 0)
  {
    result /= length;
  }
  return result;
}

inline F32 Vec_Dot(Vec2 a, Vec2 b)
{
  F32 result;
  result = a.x * b.x + a.y * b.y;
  return result;
}

inline Vec2 Vec_Perp(Vec2 v)
{
  Vec2 result = vec2(-v.y, v.x);
  return result;
}

inline Vec2 Vec_Slide(Vec2 v, Vec2 unit_normal)
{
  Vec2 result;
  // NOTE: Once to project v onto tangent of normal, (slide along wall)
  Vec2 v_proj_n = Vec_Dot(v, unit_normal) * unit_normal;
  result = v - v_proj_n;
  return result;
}

inline Vec2 Vec_SlidePreserveMomentum(Vec2 v, Vec2 unit_normal)
{
  Vec2 result;
  // NOTE: Once to project v onto tangent of normal, (slide along wall)
  Vec2 v_proj_n = Vec_Dot(v, unit_normal) * unit_normal;
  result = v - v_proj_n;
  if (Vec_Length_Sq(result) > 0)
  {
    result = Vec_Normalize(result) * Vec_Length(v);
  }
  return result;
}

inline Vec2 Vec_SlideSafe(Vec2 v, Vec2 normal)
{
  Vec2 result;
  Vec2 unit_normal = Vec_NormalizeSafe(normal);
  result = Vec_Slide(v, unit_normal);
  return result;
}

inline Vec2 Vec_SlideSafePreserveMomentum(Vec2 v, Vec2 normal)
{
  Vec2 result;
  Vec2 unit_normal = Vec_NormalizeSafe(normal);
  result = Vec_SlidePreserveMomentum(v, unit_normal);
  return result;
}

inline Vec2 Vec_Reflect(Vec2 v, Vec2 unit_normal)
{
  Vec2 result;
  // NOTE: Once to project v onto tangent of normal, (slide along wall)
  // twice to reflect v into direction of normal (bounce off wall)
  Vec2 twice_v_proj_n = 2 * Vec_Dot(v, unit_normal) * unit_normal;
  result = v - twice_v_proj_n;
  return result;
}

inline Vec2 Vec_ReflectSafe(Vec2 v, Vec2 normal)
{
  Vec2 result;
  Vec2 unit_normal = Vec_NormalizeSafe(normal);
  result = Vec_Reflect(v, unit_normal);
  return result;
}

inline Vec2 Vec_Snap_Cardinal(Vec2 v)
{
  Vec2 result;
  if (Abs_F32(v.x) > Abs_F32(v.y))
  {
    if (v.x > 0)
    {
      result = vec2(1.f, 0.f);
    }
    else
    {
      result = vec2(-1.f, 0.f);
    }
  }
  else
  {
    if (v.y > 0)
    {
      result = vec2(0.f, 1.0f);
    }
    else
    {
      result = vec2(0.f, -1.f);
    }
  }
  return result;
}

//
//  NOTE: VEC3 Vec3 vec3
//

inline Vec3 vec3(F32 x, F32 y, F32 z)
{
  Vec3 result;
  result.x = x;
  result.y = y;
  result.z = z;
  return result;
}

inline Vec3 vec3i(S32 x, S32 y, S32 z)
{
  Vec3 result = {{(F32)x, (F32)y, (F32)z}};
  return result;
}

inline Vec3 vec3(Vec2 xy, F32 z)
{
  Vec3 result;
  result.x = xy.x;
  result.y = xy.y;
  result.z = z;
  return result;
}

inline Vec3 vec3(F32 v)
{
  Vec3 result;
  result.x = v;
  result.y = v;
  result.z = v;
  return result;
}

inline Vec3 operator-(Vec3 a)
{
  Vec3 result;

  result.x = -a.x;
  result.y = -a.y;
  result.z = -a.z;

  return result;
}

//
// Scalar operations
//

inline Vec3 operator*(Vec3 a, F32 s)
{
  Vec3 result;

  result.x = a.x * s;
  result.y = a.y * s;
  result.z = a.z * s;

  return result;
}

inline Vec3 operator*(F32 s, Vec3 a)
{
  Vec3 result = a * s;

  return result;
}

inline Vec3 operator/(Vec3 a, F32 s)
{
  Vec3 result;

  result.x = a.x / s;
  result.y = a.y / s;
  result.z = a.z / s;

  return result;
}

//
//  Vector element-wise operations
//

inline Vec3 operator+(Vec3 a, Vec3 b)
{
  Vec3 result;

  result.x = a.x + b.x;
  result.y = a.y + b.y;
  result.z = a.z + b.z;

  return result;
}

inline Vec3 operator-(Vec3 a, Vec3 b)
{
  Vec3 result;

  result.x = a.x - b.x;
  result.y = a.y - b.y;
  result.z = a.z - b.z;

  return result;
}

inline Vec3 operator*(Vec3 a, Vec3 b)
{
  Vec3 result;

  result.x = a.x * b.x;
  result.y = a.y * b.y;
  result.z = a.z * b.z;

  return result;
}

//
//   Compound Operations
//
inline Vec3& operator*=(Vec3& a, F32 s)
{
  a = a * s;

  return a;
}

inline Vec3& operator/=(Vec3& a, F32 s)
{
  a = a / s;

  return a;
}

inline Vec3& operator+=(Vec3& a, Vec3 b)
{
  a = a + b;

  return a;
}

inline Vec3& operator-=(Vec3& a, Vec3 b)
{
  a = a - b;

  return a;
}

inline Vec3& operator*=(Vec3& a, Vec3 b)
{
  a = b * a;

  return a;
}

//
//  Vec3 Functions
//

inline Vec3 Vec_Clamp01(Vec3 a)
{
  Vec3 result;
  result.x = Clamp01(a.x);
  result.y = Clamp01(a.y);
  result.z = Clamp01(a.z);
  return result;
}

inline F32 Vec_Length_Sq(Vec3 a)
{
  F32 result = a.x * a.x + a.y * a.y + a.z * a.z;
  return result;
}

inline F32 Vec_Length(Vec3 a)
{
  F32 result = a.x * a.x + a.y * a.y + a.z * a.z;
  result = Sqrt_F32(result);
  return result;
}

inline Vec3 Vec_Normalize(Vec3 a)
{
  Vec3 result = a;
  F32 length = Vec_Length(a);
  result /= length;
  return result;
}

inline Vec3 Vec_NormalizeSafe(Vec3 a)
{
  Vec3 result = a;
  F32 length = Vec_Length(a);
  if (length > 0)
  {
    result /= length;
  }
  return result;
}

inline F32 Vec_Dot(Vec3 a, Vec3 b)
{
  F32 result;
  result = a.x * b.x + a.y * b.y + a.z * b.z;
  return result;
}

inline Vec3 Vec_Lerp(Vec3 a, Vec3 b, F32 t)
{
  Vec3 result = (1.f - t) * a + t * b;

  return result;
}

inline Vec3 Vec_Slide(Vec3 v, Vec3 unit_normal)
{
  Vec3 result;
  // NOTE: Once to project v onto tangent of normal, (slide along wall)
  Vec3 v_proj_n = Vec_Dot(v, unit_normal) * unit_normal;
  result = v - v_proj_n;
  return result;
}

inline Vec3 Vec_SlidePreserveMomentum(Vec3 v, Vec3 unit_normal)
{
  Vec3 result;
  // NOTE: Once to project v onto tangent of normal, (slide along wall)
  Vec3 v_proj_n = Vec_Dot(v, unit_normal) * unit_normal;
  result = v - v_proj_n;
  if (Vec_Length_Sq(result) > 0)
  {
    result = Vec_Normalize(result) * Vec_Length(v);
  }
  return result;
}

inline Vec3 Vec_SlideSafe(Vec3 v, Vec3 normal)
{
  Vec3 result;
  Vec3 unit_normal = Vec_NormalizeSafe(normal);
  result = Vec_Slide(v, unit_normal);
  return result;
}

inline Vec3 Vec_SlideSafePreserveMomentum(Vec3 v, Vec3 normal)
{
  Vec3 result;
  Vec3 unit_normal = Vec_NormalizeSafe(normal);
  result = Vec_SlidePreserveMomentum(v, unit_normal);
  return result;
}

inline Vec3 Vec_Reflect(Vec3 v, Vec3 unit_normal)
{
  Vec3 result;
  // NOTE: Once to project v onto tangent of normal, (slide along wall)
  // twice to reflect v into direction of normal (bounce off wall)
  Vec3 twice_v_proj_n = 2 * Vec_Dot(v, unit_normal) * unit_normal;
  result = v - twice_v_proj_n;
  return result;
}

inline Vec3 Vec_ReflectSafe(Vec3 v, Vec3 normal)
{
  Vec3 result;
  Vec3 unit_normal = Vec_NormalizeSafe(normal);
  result = Vec_Reflect(v, unit_normal);
  return result;
}

//
//  NOTE: VEC4 Vec4 vec4
//

inline Vec4 vec4(F32 x, F32 y, F32 z, F32 w)
{
  Vec4 result;
  result.x = x;
  result.y = y;
  result.z = z;
  result.w = w;
  return result;
}

inline Vec4 vec4(F32 v)
{
  Vec4 result;
  result.x = v;
  result.y = v;
  result.z = v;
  result.w = v;
  return result;
}

inline Vec4 vec4(Vec3 xyz, F32 w)
{
  Vec4 result;
  result.x = xyz.x;
  result.y = xyz.y;
  result.z = xyz.z;
  result.w = w;
  return result;
}

inline Vec4 operator-(Vec4 a)
{
  Vec4 result;

  result.x = -a.x;
  result.y = -a.y;
  result.z = -a.z;
  result.w = -a.w;

  return result;
}

//
// Scalar operations
//

inline Vec4 operator*(Vec4 a, F32 s)
{
  Vec4 result;

  result.x = a.x * s;
  result.y = a.y * s;
  result.z = a.z * s;
  result.w = a.w * s;

  return result;
}

inline Vec4 operator*(F32 s, Vec4 a)
{
  Vec4 result = a * s;

  return result;
}

inline Vec4 operator/(Vec4 a, F32 s)
{
  Vec4 result;

  result.x = a.x / s;
  result.y = a.y / s;
  result.z = a.z / s;
  result.w = a.w / s;

  return result;
}

//
//  Vector element-wise operations
//

inline Vec4 operator+(Vec4 a, Vec4 b)
{
  Vec4 result;

  result.x = a.x + b.x;
  result.y = a.y + b.y;
  result.z = a.z + b.z;
  result.w = a.w + b.w;

  return result;
}

inline Vec4 operator-(Vec4 a, Vec4 b)
{
  Vec4 result;

  result.x = a.x - b.x;
  result.y = a.y - b.y;
  result.z = a.z - b.z;
  result.w = a.w - b.w;

  return result;
}

inline Vec4 operator*(Vec4 a, Vec4 b)
{
  Vec4 result;

  result.x = a.x * b.x;
  result.y = a.y * b.y;
  result.z = a.z * b.z;
  result.w = a.w * b.w;

  return result;
}

//
//   Compound Operations
//
inline Vec4& operator*=(Vec4& a, F32 s)
{
  a = a * s;

  return a;
}

inline Vec4& operator/=(Vec4& a, F32 s)
{
  a = a / s;

  return a;
}

inline Vec4& operator+=(Vec4& a, Vec4 b)
{
  a = a + b;

  return a;
}

inline Vec4& operator-=(Vec4& a, Vec4 b)
{
  a = a - b;

  return a;
}

inline Vec4& operator*=(Vec4& a, Vec4 b)
{
  a = b * a;

  return a;
}

//
//  Vec4 Functions
//

inline Vec4 Vec_Clamp01(Vec4 a)
{
  Vec4 result;
  result.x = Clamp01(a.x);
  result.y = Clamp01(a.y);
  result.z = Clamp01(a.z);
  result.w = Clamp01(a.w);
  return result;
}

inline F32 Vec_Length_Sq(Vec4 a)
{
  F32 result = a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w;
  return result;
}

inline F32 Vec_Length(Vec4 a)
{
  F32 result = a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w;
  result = Sqrt_F32(result);
  return result;
}

inline Vec4 Vec_Normalize(Vec4 a)
{
  Vec4 result = a;
  F32 length = Vec_Length(a);
  result /= length;
  return result;
}

inline Vec4 Vec_NormalizeSafe(Vec4 a)
{
  Vec4 result = a;
  F32 length = Vec_Length(a);
  if (length > 0)
  {
    result /= length;
  }
  return result;
}

inline F32 Vec_Dot(Vec4 a, Vec4 b)
{
  F32 result;
  result = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
  return result;
}

inline Vec4 Vec_Lerp(Vec4 a, Vec4 b, F32 t)
{
  Vec4 result = (1.f - t) * a + t * b;

  return result;
}

inline Vec4 Vec_Unpack_Vec4_U8(Vec4_U8 packed)
{
  Vec4 result;
  result.a = packed.argb.a;
  result.r = packed.argb.r;
  result.g = packed.argb.g;
  result.b = packed.argb.b;
  return result;
}

inline Vec4 Vec_Unpack_4x8(U32 packed)
{
  Vec4 result;
  Vec4_U8 temp;
  temp.u32 = packed;
  result = Vec_Unpack_Vec4_U8(temp);
  return result;
}

#endif
