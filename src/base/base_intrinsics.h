#ifndef BASE_INTRIN_H
#define BASE_INTRIN_H

// TODO: convert all of these to platform-efficient versions
//  and remove math.h
#include <math.h>

typedef struct
{
  B32 found;
  S32 index;
} Bit_Scan_Result;

internal inline Bit_Scan_Result Find_Least_Significant_Set_Bit(U32 value)
{
  Bit_Scan_Result result = {0};
#if COMPILER_CLANG

  result.index = __builtin_ctz(value);
  result.found = (B32)value;

#else

  for (U32 index = 0; index < 32; ++index)
  {
    if (value & (1 << index))
    {
      result.index = index;
      result.found = true;
      break;
    }
  }
#endif
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

internal inline F32 Sqrtf(F32 x)

{
  F32 result = sqrtf(x);
  return result;
}

#endif // BASE_INTRIN_H
