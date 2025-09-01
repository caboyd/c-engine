#ifndef BASE_INTRIN_H
#define BASE_INTRIN_H

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
  result.found = value;

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

#endif // BASE_INTRIN_H
