#include <math.h>

double asin(double x) {
    if (x > 1.0 || x < -1.0) {
        return 0.0 / 0.0;
    }
    return atan2(x, sqrt((1.0 - x) * (1.0 + x)));
}
