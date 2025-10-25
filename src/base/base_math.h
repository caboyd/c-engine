#ifndef BASE_MATH_H

#define BASE_MATH_H

#define M_2PI 6.28318530717958647692
#define M_PI 3.14159265358979323846
#define M_PI_2 1.57079632679489661923
#define M_PI_4 0.78539816339744830962

#define SQRT2_2 0.7071067811865476
#define SQRT2_2F 0.7071067811865476f

inline F32 Square(F32 a)
{
  F32 result = a * a;
  return result;
}

inline F32 Safe_Ratio(F32 numerator, F32 divisor, F32 fallback)
{
  F32 result = fallback;
  if (divisor != 0.f)
  {
    result = numerator / divisor;
  }
  return result;
}

inline F32 Safe_Ratio0(F32 numerator, F32 divisor)
{
  F32 result = Safe_Ratio(numerator, divisor, 0.f);
  return result;
}

inline F32 Safe_Ratio1(F32 numerator, F32 divisor)
{
  F32 result = Safe_Ratio(numerator, divisor, 1.f);
  return result;
}

inline F32 Lerp(F32 a, F32 b, F32 t)
{
  F32 result = ((1.f - t) * a) + (t * b);
  return result;
}

inline F32 Clamp01(F32 a)
{
  F32 result = CLAMP(a, 0.f, 1.f);
  return result;
}

inline F32 Clamp01_Map_To_Range(F32 min, F32 max, F32 t)
{
  F32 result = 0.f;
  F32 range = max - min;
  if (range != 0.f)
  {
    result = Clamp01((t - min) / range);
  }
  return result;
}

#endif // BASE_MATH_H
