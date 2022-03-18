#include <stdio.h>

#include "cam18sl.h"

int main()
{
    /* CAM18sl test with inverse */
    double L_cone = 100;
    double M_cone = 100;
    double S_cone = 100;
    printf("  LMS = %f, %f, %f\n", L_cone, M_cone, S_cone);

    /* Surround cone value */
    double bg_cone = 0.0;

    /* M = colourfulness, Q = brightness, A = sorta luminance, ab = opponent signals */
    double A, a, b, Q, M;
    cam18sl(L_cone, M_cone, S_cone, bg_cone, &a, &b, &A, &M, &Q);
    printf(" abQA = %f, %f, %f, %f\n", a, b, Q, A);

    /* Inverse */
    double L_cone2, M_cone2, S_cone2;
    cam18sl_inverse(bg_cone, a, b, NULL, &Q, &L_cone2, &M_cone2, &S_cone2);
    printf("  LMS = %f, %f, %f\n", L_cone2, M_cone2, S_cone2);

    return 0;
}