#ifndef RECT_H
#define RECT_H

struct Rect2
{
  Vec2 min;
  Vec2 max;
};

struct Rect3
{
  Vec3 min;
  Vec3 max;
};

//
//  NOTE: Rect2
//

inline Vec2 Rect_Get_Min_Corner(Rect2 rect)
{
  Vec2 result = rect.min;
  return result;
}

inline Vec2 Rect_Get_Max_Corner(Rect2 rect)
{
  Vec2 result = rect.max;
  return result;
}

inline Vec2 Rect_Get_Center(Rect2 rect)
{
  Vec2 result = 0.5f * (rect.min + rect.max);
  return result;
}

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

inline B32 Is_In_Rect(Rect2 rect, Vec2 test_pos)
{
  B32 result = (test_pos.x >= rect.min.x) && (test_pos.y >= rect.min.y);
  result = result && (test_pos.x < rect.max.x) && (test_pos.y < rect.max.y);

  return result;
}
inline Rect2 Rect_Add_Radius(Rect2 rect, Vec2 radius)
{
  Rect2 result;
  result.min = rect.min - radius;
  result.max = rect.max + radius;
  return result;
}

inline Vec2 Rect_Get_Barycentric_Coord(Rect2 rect, Vec2 p)

{
  Vec2 result;
  result.x = Safe_Ratio0((p.x - rect.min.x), (rect.max.x - rect.min.x));
  result.y = Safe_Ratio0((p.y - rect.min.y), (rect.max.y - rect.min.y));
  return result;
}

//
//  NOTE: Rect3
//
inline Vec3 Rect_Get_Min_Corner(Rect3 rect)
{
  Vec3 result = rect.min;
  return result;
}

inline Vec3 Rect_Get_Max_Corner(Rect3 rect)
{
  Vec3 result = rect.max;
  return result;
}

inline Vec3 Rect_Get_Center(Rect3 rect)
{
  Vec3 result = 0.5f * (rect.min + rect.max);
  return result;
}

inline Rect3 Rect_Min_Max(Vec3 min, Vec3 max)
{
  Rect3 result;
  result.min = min;
  result.max = max;

  return result;
}
inline Rect3 Rect_Min_Dim(Vec3 min, Vec3 dim)
{
  Rect3 result;
  result.min = min;
  result.max = min + dim;

  return result;
}
inline Rect3 Rect_Center_HalfDim(Vec3 center, Vec3 half_dim)
{
  Rect3 result;
  result.min = center - half_dim;
  result.max = center + half_dim;

  return result;
}
inline Rect3 Rect_Center_Dim(Vec3 center, Vec3 dim)
{
  Rect3 result = Rect_Center_HalfDim(center, 0.5f * dim);

  return result;
}

inline B32 Is_In_Rect(Rect3 rect, Vec3 test_pos)
{
  B32 result = (test_pos.x >= rect.min.x) && (test_pos.y >= rect.min.y) && (test_pos.z >= rect.min.z);
  result = result && (test_pos.x < rect.max.x) && (test_pos.y < rect.max.y) && (test_pos.z < rect.max.z);

  return result;
}

inline Rect3 Rect_Add_Radius(Rect3 rect, Vec3 radius)
{
  Rect3 result;
  result.min = rect.min - radius;
  result.max = rect.max + radius;
  return result;
}

inline Rect3 Rect_Add_Offset(Rect3 rect, Vec3 offset)
{
  Rect3 result;
  result.min += offset;
  result.max += offset;
  return result;
}

inline B32 Rect_Intersect_Rect(Rect3 a, Rect3 b)
{
  B32 result = ((a.max.x > b.min.x) && (a.min.x < b.max.x) && (a.max.y > b.min.y) && (a.min.y < b.max.y) &&
                (a.max.z > b.min.z) && (a.min.z < b.max.z));
  return result;
}
inline Vec3 Rect_Get_Barycentric_Coord(Rect3 rect, Vec3 p)
{
  Vec3 result;
  result.x = Safe_Ratio0((p.x - rect.min.x), (rect.max.x - rect.min.x));
  result.y = Safe_Ratio0((p.y - rect.min.y), (rect.max.y - rect.min.y));
  result.z = Safe_Ratio0((p.z - rect.min.z), (rect.max.z - rect.min.z));
  return result;
}

#endif // RECT_H
