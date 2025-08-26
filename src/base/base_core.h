#ifndef BASE_CORE_H
#define BASE_CORE_H

/////////////////////////////
// Foreign includes
#include <stdbool.h>
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
#define MEM_ZERO_(x, sz) memset((x), 0, sz)

#define Kilobytes(value) ((value) * 1024LL)
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

internal void cstring_append(char* restrict dest, S32 dest_len, char* src1, S32 src1_len)
{
  if (dest_len == 0)
  {
    return;
  }
  S32 dest_index = 0;
  S32 dest_len_minus_one = dest_len - 1;

  // NOTE:scan until you hit null terminator in first string
  for (S32 i = 0; i < dest_len_minus_one && dest[dest_index]; i++)
  {
    dest_index = i;
  }

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

internal S32 cstring_len(char* string)
{
  S32 result = 0;
  for (char* c = string; *c; c++, result++)
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
  // TODO: try SSE4.2 _mm_cmpistri
  if (!(*needle))
  {
    return haystack;
  }

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

internal U32 memory_last_nonzero_byte(void* memory, U32 size_in_bytes)
{
  U32* mem = (U32*)memory;
  U32 size_in_bytes_rounded = (size_in_bytes + 3) & ~3u;

  // NOTE: Scan 1 page of bytes
  U32 block_size = 4096 / sizeof(U32);
  U32 low = 0;
  U32 size_in_U32 = size_in_bytes_rounded / sizeof(U32);
  U32 high = size_in_U32;
  U32 last_nonzero_index = 0;

  while (low < high)
  {
    U32 mid = low + ((high - low) / 2);
    mid = (mid / block_size) * block_size; // align to block

    // check if this block has any non-zero
    U32 end = (mid + block_size < size_in_U32) ? mid + block_size : size_in_U32;
    B32 found = 0;
    for (U32 i = mid; i < end; i++)
    {
      if (mem[i] != 0)
      {
        found = 1;
        break;
      }
    }

    if (found)
    {
      last_nonzero_index = end;
      low = end;
    }
    else
    {
      high = mid;
    }
  }

  // refine to exact last non-zero U32
  while (last_nonzero_index > 0 && mem[last_nonzero_index - 1] == 0)
  {
    last_nonzero_index--;
  }

  return last_nonzero_index * sizeof(U32);
}

#endif
