#ifndef RECT_H
#define RECT_H

struct Rect2
{
  Vec2 min;
  Vec2 max;
};

inline Rect2 Rect_Min_Max(Vec2 min, Vec2 max)
{
  Rect2 result;
  result.min = min;
  result.max = max;

  return result;
}
inline Rect2 Rect_Min_Dim(Vec2 min, Vec2 dim)
{
  Rect2 result;
  result.min = min;
  result.max = min + dim;

  return result;
}
inline Rect2 Rect_Center_HalfDim(Vec2 center, Vec2 half_dim)
{
  Rect2 result;
  result.min = center - half_dim;
  result.max = center + half_dim;

  return result;
}
inline Rect2 Rect_Center_Dim(Vec2 center, Vec2 dim)
{
  Rect2 result = Rect_Center_HalfDim(center, 0.5f * dim);

  return result;
}

internal inline B32 Is_In_Rect(Rect2 rect, Vec2 test_pos)
{
  B32 result = (test_pos.x >= rect.min.x) && (test_pos.y >= rect.min.y);
  result = result && (test_pos.x < rect.max.x) && (test_pos.y < rect.max.y);

  return result;
}

#endif // RECT_H
