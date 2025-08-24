#ifndef BASE_MATH_H
#define BASE_MATH_H

#define M_PI 3.14159265358979323846
#define M_PI_2 1.57079632679489661923
#define M_PI_4 0.78539816339744830962




internal S32 Round_F32_to_S32(F32 value)
{
  S32 result;
  result = (S32)(value + 0.5);
  return result;
}

internal F32 Floor_F32(F32 value)
{
  F32 result = value;
  if (result >= 0.0f)
  {
    result = (F32)((S32)(value));
  }
  else
  {
    result = (F32)((S32)(value - 1.f));
  }
  return result;
}









#endif
