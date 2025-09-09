#ifndef BASE_CORE_H
#define BASE_CORE_H

/////////////////////////////
// Foreign includes
#include <stdbool.h>
#include <stdint.h>

/////////////////////////////

//
// NOTE:Compilers
//

#if !defined(COMPILER_CLANG)
#define COMPILER_CLANG 0
#endif

#if !defined(COMPILE_MSVC)
#define COMPILER_MSVC 0
#endif

#if !COMPILER_MSVC && !COMPILER_CLANG
#if __clang__
#undef COMPILER_CLANG
#define COMPILER_CLANG 1
#endif
#endif

#include "base_def.h"

//
// NOTE: Types
//
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

#if CENGINE_SLOW
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

#define Invalid_Code_Path ASSERT(0 == "Invalid Code Path");

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

// NOTE:--------ARENA----------------
typedef struct Arena Arena;
struct Arena
{
  U8* base;
  U64 size;
  U64 used;
};
internal void Initialize_Arena(Arena* arena, U64 size, U8* base)
{
  arena->size = size;
  arena->base = base;
  arena->used = 0;
}
#define Push_Struct(arena, type) (type*)Push_Size_(arena, sizeof(type))
#define Push_Array(arena, count, type) (type*)Push_Size_(arena, (count) * sizeof(type))

internal void* Push_Size_(Arena* arena, U64 size)
{
  // TODO:
  // IMPORTANT: CLEAR TO ZERO OPTION
  ASSERT((arena->used + size) <= arena->size);
  void* result = arena->base + arena->used;
  arena->used += size;
  return result;
}

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

internal S32 Trunc_F32_S32(F32 value)

{
  S32 result = (S32)value;
  return result;
}

//-------------------------------------------

internal void Memory_Copy(void* dest, void* src, S32 size)
{
  if (((uintptr_t)dest % 4 == 0) && ((uintptr_t)src % 4 == 0))
  {
    // TODO: copy 4 bytes if aligned
  }
  U8* d = (U8*)dest;
  U8* s = (U8*)src;
  for (S32 i = 0; i < size; ++i)
  {
    d[i] = s[i];
  }
}

internal void cstring_append(char* __restrict dest, S32 dest_len, char* src1, S32 src1_len)
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
internal void cstring_cat(char* __restrict dest, S32 dest_len, char* src1, S32 src1_len, char* src2, S32 src2_len)
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

#endif // BASE_CORE_H
