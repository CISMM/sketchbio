#ifndef SKETCHTESTS_H
#define SKETCHTESTS_H

#include <quat.h>

// do I have to explain these?
inline double min(double a, double b) {
    return a < b ? a : b;
}
inline double max(double a, double b) {
    return a > b ? a : b;
}

// tests if two vectors are the same
inline bool q_vec_equals(const q_vec_type a, const q_vec_type b,
                         double eps = Q_EPSILON) {
    if (Q_ABS(a[Q_X] - b[Q_X]) > eps) {
        return false;
    }
    if (Q_ABS(a[Q_Y] - b[Q_Y]) > eps) {
        return false;
    }
    if (Q_ABS(a[Q_Z] - b[Q_Z]) > eps) {
        return false;
    }
    return true;
}

// tests if two quaternions are the same
inline bool q_equals(const q_type a, const q_type b,
					 double eps = Q_EPSILON) {
    q_type c;
    q_copy(c,b);
    bool inverses = true;
    for (int i = 0; i < 4; i++)
    {
        if (Q_ABS(a[i] + b[i]) > eps)
            inverses = false;
    }
    if (inverses)
    {
        for (int i = 0; i < 4; i++)
        {
            c[i] = -c[i];
        }
    }
    if (Q_ABS(a[Q_W] - c[Q_W]) > eps) {
        return false;
    }
    return q_vec_equals(a,c);
}
#endif // SKETCHTESTS_H
