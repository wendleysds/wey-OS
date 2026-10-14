#include <math.h>

float acosf(float x) {
    if (x > 1.0f || x < -1.0f) {
        return 0.0f / 0.0f;
    }
    return atan2f(sqrtf((1.0f - x) * (1.0f + x)), x);
}
