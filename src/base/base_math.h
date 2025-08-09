#pragma once

#include "base_core.h"

typedef union {
  F32 v[2];
  struct {
    F32 x;
    F32 y;
  };
} vec2;
