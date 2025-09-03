#ifndef BASE_MATH_H
#define BASE_MATH_H

#define M_PI 3.14159265358979323846
#define M_PI_2 1.57079632679489661923
#define M_PI_4 0.78539816339744830962

typedef struct
{
  union
  {
    F32 v[2];
    struct
    {
      F32 x;
      F32 y;
    };
  };
} Vec2;

typedef struct
{
  union
  {
    U32 u32;
    U8 v[4];
    struct
    {
      // NOTE: alpha is higest byte in argb
      U8 b, g, r, a;
    } argb;
  };
} Vec4_U8;

typedef struct
{
  union
  {
    F32 v[4];
    struct
    {
      F32 r, g, b, a;
    };
    struct
    {
      F32 x, y, z, w;
    };
  };
} Vec4_F32;

typedef Vec4_F32 Vec4;
#endif // BASE_MATH_H
