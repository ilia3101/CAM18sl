#ifndef _cam18sl_h_
#define _cam18sl_h_

void cam18sl(double L, double M, double S, double BackgroundValue, double * aOut, double * bOut, double * AOut, double * MOut, double * QOut);

/* Input a,b required, plus one of A or Q. Output LMS. */
void cam18sl_inverse(double BackgroundValue, double a, double b, double * AIn, double * QIn, double * LOut, double * MOut, double * SOut);

#endif