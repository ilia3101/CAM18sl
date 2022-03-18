#include <stddef.h>
#include <math.h>

/* Matrix code from mlv app 0.1 */
#include "matrix/matrix.h"

#define p 0.58

double cam18sl_cone_compress(double x, double a)
{
    return pow(x, p) / (pow(x, p) + pow(a, p));
}

double cam18sl_cone_compress_inverse(double x, double a)
{
    return pow((x * pow(a, p)) / (1.0 - x), 1.0/p);
}

double LMSc_to_Aab[9] = {
    2.0,            1.0,            1.0/20.0,
    63.0/100.0,    -189.0/275.0,    63.0/1100.0,
    3.0/25.0,       3.0/25.0,      -6.0/25.0
};

/* colourfulness */
double cam18sl_M(double a, double b)
{
    return 3260.0 * sqrt(a*a + b*b);
}

/* Brightness */
double cam18sl_Q(double a, double b, double A)
{
    return 0.937 * (A + 7.824 * pow(a*a+b*b, 0.545));
}

/* Luminance-ish */
double cam18sl_A(double a, double b, double Q)
{
    return Q / 0.937 - 7.824 * pow(a*a+b*b, 0.545);
}

void cam18sl(double L, double M, double S, double BackgroundValue, double * aOut, double * bOut, double * AOut, double * MOut, double * QOut)
{
    double a = (291.2 + 71.8*pow(BackgroundValue, 0.78));
    double LMS_c[3] = {
        cam18sl_cone_compress(L, a),
        cam18sl_cone_compress(M, a),
        cam18sl_cone_compress(S, a)
    };

    double * Aab = LMS_c;
    applyMatrix(Aab, LMSc_to_Aab);

    if (AOut) *AOut = Aab[0];
    if (aOut) *aOut = Aab[1];
    if (bOut) *bOut = Aab[2];
    if (MOut) *MOut = cam18sl_M(Aab[1], Aab[2]);
    if (QOut) *QOut = cam18sl_Q(Aab[1], Aab[2], Aab[0]);
}

void cam18sl_inverse(double BackgroundValue, double a, double b, double * AIn, double * QIn, double * LOut, double * MOut, double * SOut)
{
    double Aab_to_LMSc[9];
    invertMatrix(LMSc_to_Aab, Aab_to_LMSc);

    double Aab[3] = {0, a, b};

    /* If no A (lumninanceish) signal, get it out from brightness (Q) */
    if (AIn == NULL) {
        Aab[0] = cam18sl_A(a, b, *QIn);
    }

    /* Get back to LMS compressed signal */
    double * LMS_c = Aab;
    applyMatrix(LMS_c, Aab_to_LMSc);

    /* Uncompress LMS signal */
    double _a = (291.2 + 71.8*pow(BackgroundValue, 0.78));
    if (LOut) *LOut = cam18sl_cone_compress_inverse(LMS_c[0], _a);
    if (MOut) *MOut = cam18sl_cone_compress_inverse(LMS_c[1], _a);
    if (SOut) *SOut = cam18sl_cone_compress_inverse(LMS_c[2], _a);
}
