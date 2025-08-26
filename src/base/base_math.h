#ifndef BASE_MATH_H
#define BASE_MATH_H

#define M_PI 3.14159265358979323846
#define M_PI_2 1.57079632679489661923
#define M_PI_4 0.78539816339744830962

internal inline S32 Round_F32_S32(F32 value)
{
  S32 result = (S32)(value + 0.5);
  return result;
}

internal F32 Floor_F32(F32 value)
{
  S32 i = (S32)value;
  F32 result = (F32)i;
  if (value < result)
  {
    result -= 1.f;
  }
  return result;
}

internal inline S32 Floor_F32_S32(F32 value)
{
  S32 result = (S32)value - (value < (F32)(S32)value);
  return result;
}
internal inline S32 Ceil_F32_S32(F32 value)
{
  S32 result = (S32)value + (value > (F32)(S32)value);
  return result;
}
#endif
