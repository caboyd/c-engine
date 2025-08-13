#pragma once

#define M_PI 3.14159265358979323846


typedef union {
  F32 v[2];
  struct {
    F32 x;
    F32 y;
  };
} vec2;

