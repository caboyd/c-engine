#ifndef BASE_CORE_H
#define BASE_CORE_H

/////////////////////////////
// Foreign includes

#include <stdint.h>

/////////////////////////////
// Codebase keywords
#if defined(CLANGD)
#define internal __attribute__((unused)) static
#else
#define internal static
#endif
#define local_persist static
#define global static

typedef uint8_t U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;
typedef int8_t S8;
typedef int16_t S16;
typedef int32_t S32;
typedef int64_t S64;
typedef S8 B8;
typedef S16 B16;
typedef S32 B32;
typedef S64 B64;
typedef float F32;
typedef double F64;

// To prevent -Wunused warnings
#if defined(__GNUC__)
#define ATTRIBUTE_UNUSED __attribute__((unused))
#else
#define ATTRIBUTE_UNUSED
#endif

#define Array_Count(array) (sizeof(array) / sizeof((array)[0]))


typedef union
{
  F32 v[2];
  struct
  {
    F32 x;
    F32 y;
  };
} Vec2;



#endif
