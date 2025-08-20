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

#ifdef CENGINE_SLOW
#define ASSERT(expression)                                                                                             \
  do                                                                                                                   \
  {                                                                                                                    \
    if (!(expression))                                                                                                 \
    {                                                                                                                  \
      *(volatile int*)0 = 0;                                                                                           \
    }                                                                                                                  \
  } while (0)

#else
#define ASSERT(expression)
#endif

#define STATIC_ASSERT(cond, msg) typedef char static_assert_##msg[(cond) ? 1 : -1]

#define Array_Count(array) (sizeof(array) / sizeof((array)[0]))

#define MEM_ZERO(p) memset(&(p), 0, sizeof(p))
#define MEM_ZERO_(x, sz) memset((p), 0, sz)

#define Kilobytes(value) (value * 1024LL)
#define Megabytes(value) (Kilobytes(value) * 1024LL)
#define Gigabytes(value) (Megabytes(value) * 1024LL)
#define Terabytes(value) (Gigabytes(value) * 1024LL)

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define CLAMP(x, lo, hi) (MIN(MAX((x), (lo)), (hi)))

typedef union
{
  F32 v[2];
  struct
  {
    F32 x;
    F32 y;
  };
} Vec2;

// NOTE: static is requried in C but not C++
static inline U32 Safe_Truncate_U64(U64 value)
{
  ASSERT(value <= UINT32_MAX);
  U32 result = (U32)value;
  return result;
}

internal void cstring_append(char* restrict dest, S32 dest_len, char* src1, S32 src1_len)
{
  if (dest_len == 0)
  {
    return;
  }
  S32 dest_index = 0;
  S32 dest_len_minus_one = dest_len - 1;

  for (S32 src_index = 0; dest_index < dest_len_minus_one && src_index < src1_len; dest_index++, src_index++)
  {
    if (!src1[src_index])
    {
      break;
    }
    dest[dest_index] = src1[src_index];
  }

  dest[dest_index] = '\0';
}

internal S32 cstring_len(char* s)
{
  S32 result = 0;
  char* c;
  for (c = s; *c; c++, result++)
    ;

  return result;
}

// dest must not overlap with both src1 and src2
internal void cstring_cat(char* restrict dest, S32 dest_len, char* src1, S32 src1_len, char* src2, S32 src2_len)
{
  if (dest_len == 0)
  {
    return;
  }
  S32 dest_index = 0;
  S32 dest_len_minus_one = dest_len - 1;

  for (S32 src_index = 0; dest_index < dest_len_minus_one && src_index < src1_len; dest_index++, src_index++)
  {
    if (!src1[src_index])
    {
      break;
    }
    dest[dest_index] = src1[src_index];
  }
  for (S32 src_index = 0; dest_index < dest_len_minus_one && src_index < src2_len; dest_index++, src_index++)
  {
    if (!src2[src_index])
    {
      break;
    }
    dest[dest_index] = src2[src_index];
  }
  dest[dest_index] = '\0';
}

internal char* cstring_find_substr(char* haystack, char* needle)
{
  if (!(*needle))
    return haystack;

  for (char* h = haystack; *h; h++)
  {
    char* h_iter = h;
    char* n_iter = needle;

    while (*h_iter && *n_iter && *h_iter == *n_iter)
    {
      h_iter++;
      n_iter++;
    }

    // NOTE: reached end of needle and found it
    if (!(*n_iter))
    {
      return h;
    }
  }
  return 0;
}
#endif
