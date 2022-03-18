/* Test program to verift if inverse works correctly. */

#include <stdio.h>

#include "cam18sl.h"

int main()
{
    /* Just put any random (reasonable) values here... */
    double L_cone = 35;
    double M_cone = 100;
    double S_cone = 142;
    printf("       original LMS = %f, %f, %f\n", L_cone, M_cone, S_cone);

    /* Surround cone value */
    double bg_cone = 0.0;

    /* M = colourfulness, Q = brightness, A = sorta luminance, ab = opponent signals */
    double A, a, b, Q, M;
    cam18sl(L_cone, M_cone, S_cone, bg_cone, &a, &b, &A, &M, &Q);
    // printf(" abQA = %f, %f, %f, %f\n", a, b, Q, A);

    /* Inverse */
    double L_cone2, M_cone2, S_cone2;
    cam18sl_inverse(bg_cone, a, b, NULL, &Q, &L_cone2, &M_cone2, &S_cone2);
    printf(" post inversion LMS = %f, %f, %f\n", L_cone2, M_cone2, S_cone2);

    return 0;
}