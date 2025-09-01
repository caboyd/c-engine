#ifndef BASE_MATH_H
#define BASE_MATH_H
// TODO: convert all of these to platform-efficient versions
//  and remove math.h

#include <math.h>

#define M_PI 3.14159265358979323846
#define M_PI_2 1.57079632679489661923
#define M_PI_4 0.78539816339744830962

internal inline U32 Safe_Truncate_U64(U64 value)
{
  ASSERT(value <= 0xFFFFFFFF);
  U32 result = (U32)value;
  return result;
}

internal inline U32 F32_to_U32_255(F32 value)
{
  value = CLAMP(value, 0.f, 1.f);
  U32 result = (U32)(value * 255.f + 0.5f);
  return result;
}

internal inline S32 Trunc_F32_S32(F32 value)
{
  S32 result = (S32)value;
  return result;
}
internal inline S32 Round_F32_S32(F32 value)
{
  S32 result = (S32)roundf(value);
  return result;
}

internal inline S32 Floor_F32_S32(F32 value)
{
  S32 result = (S32)floorf(value);
  return result;
}
internal inline S32 Ceil_F32_S32(F32 value)
{
  S32 result = (S32)ceilf(value);
  return result;
}

internal inline F32 Floor_F32(F32 value)
{
  F32 result = floorf(value);
  return result;
}

internal inline F64 Sin(F64 angle)
{
  F64 result = sin(angle);
  return result;
}

internal inline F32 Sinf(F32 angle)
{
  F32 result = sinf(angle);
  return result;
}

internal inline F32 Cosf(F32 angle)
{
  F32 result = cosf(angle);
  return result;
}

internal inline F32 Atan2f(F32 y, F32 x)
{
  F32 result = atan2f(y, x);
  return result;
}


#endif //BASE_MATH_H
