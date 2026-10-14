#include <math.h>

float asinf(float x) {
    if (x > 1.0f || x < -1.0f) {
        return 0.0f / 0.0f;
    }
    return atan2f(x, sqrtf((1.0f - x) * (1.0f + x)));
}
