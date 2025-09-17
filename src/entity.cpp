#ifdef CLANGD
#include "sim_region.cpp"
#endif

inline Move_Spec Default_Move_Spec()
{
  Move_Spec result;

  result.normalize_accel = false;
  result.speed = 1.f;
  result.drag = 0.f;

  return result;
}
