#include <xmmintrin.h>
#include <smmintrin.h>
#include <stdio.h>

int main() {
    float x = 3.7f;
    int y = _mm_cvtss_i32(_mm_round_ss(_mm_set_ss(x), _mm_set_ss(x), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC));
    printf("%d\n", y);
    return 0;
}
