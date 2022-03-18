static float CIELAB_f(float t)
{
    float six29_cubed = 0.00885645168; /* 6/29 cubed */
    float six29_squared_3 = 0.1284185493; /* 6/29 squared * 3 */
    float four29 = 0.1379310345;  /* 4/29 */

    if (t > six29_cubed)
        return powf(t, 0.333333333333333);
    else
        return t / six29_squared_3 + four29;
}

static float CIELAB_f_inverse(float t)
{
    float six29 = 0.2068965517; /* 6/29 */
    float six29_squared_3 = 0.1284185493; /* 6/29 squared * 3 */
    float four29 = 0.1379310345;  /* 4/29 */

    if (t > six29)
        return t * t * t;
    else
        return six29_squared_3 * (t - four29);
}

void XYZ_to_CIELAB(float * InputXYZ, float * OutputLAB, float * WhitePointXYZ, uint64_t N)
{
    float * XYZ = InputXYZ;
    float * LAB = OutputLAB;

    float Xn = WhitePointXYZ[0];
    float Yn = WhitePointXYZ[1];
    float Zn = WhitePointXYZ[2];

    for (uint64_t i = 0; i < N; ++i, XYZ += 3, LAB += 3)
    {
        float f_X_Xn = CIELAB_f(XYZ[0] / Xn);
        float f_Y_Yn = CIELAB_f(XYZ[1] / Yn);
        float f_Z_Zn = CIELAB_f(XYZ[2] / Zn);

        LAB[0] = 116.0f * f_Y_Yn - 16.0f;
        LAB[1] = 500.0f * (f_X_Xn - f_Y_Yn);
        LAB[2] = 200.0f * (f_Y_Yn - f_Z_Zn);
    }
}

void CIELAB_to_XYZ(float * InputLAB, float * OutputXYZ, float * WhitePointXYZ, uint64_t N)
{
    float * LAB = InputLAB;
    float * XYZ = OutputXYZ;

    float Xn = WhitePointXYZ[0];
    float Yn = WhitePointXYZ[1];
    float Zn = WhitePointXYZ[2];

    for (uint64_t i = 0; i < N; ++i, XYZ += 3, LAB += 3)
    {
        float L_16_116 = ((float)LAB[0] + 16.0f) / 116.0f;
        float a = LAB[1];
        float b = LAB[2];

        XYZ[0] = Xn * CIELAB_f_inverse(L_16_116 + a / 500.0f);
        XYZ[1] = Yn * CIELAB_f_inverse(L_16_116);
        XYZ[2] = Zn * CIELAB_f_inverse(L_16_116 - b / 200.0f);
    }
}

/* Jzazbz colour model (Links accessed 29 Septemper 2020)
 *  https://www.osapublishing.org/DirectPDFAccess/62D9F721-B15B-4AD2-A1FCB6544611ACD8_368272/oe-25-13-15131.pdf
 *  https://observablehq.com/@jrus/jzazbz
 */

/* Constants for Jzazbz */
#define b 1.15
#define g 0.66
#define c1 (3424.0 / 4096.0)
#define c2 (2413.0 / 128.0)
#define c3 (2392.0 / 128.0)
#define n (2392.0 / 16384.0)
#define p (1.7 * (2523.0 / 32.0))
#define d 0.56
#define d0 (1.6295499532821566 * pow(10, -11)) /* I really don't understand what this constant does and how it can do anything helpful!!!! */

static double Jzazbz_M[9] = { 0.41478972, 0.579999, 0.0146480,
                             -0.2015100, 1.120649, 0.0531008,
                             -0.0166008, 0.264800, 0.6684799 };

static double Jzazbz_M2[9] = { 0.500000,  0.500000,  0.000000,
                               3.524000, -4.066708,  0.542708,
                               0.199076,  1.096799, -1.295875 };

static inline double Jzazbz_forward_nonlinear(double LMS)
{
    double LMS_10k = pow(LMS / 10000.0, n);

    double top = c1 + c2 * LMS_10k;
    double bot = 1.0 + c3 * LMS_10k;

    return pow(top / bot, p);
}

static inline double Jzazbz_inverse_nonlinearf(float LMS)
{
    float LMS_pow_1p = powf(LMS, 1.0f/p);
    float x = (c1 - LMS_pow_1p) / (c3*LMS_pow_1p - c2);
    return powf(x, 1.0f/n) * 10000.0f;
}

static inline double Jzazbz_forward_nonlinearf(float LMS)
{
    float LMS_10k = powf(LMS / 10000.0f, n);

    float top = c1 + c2 * LMS_10k;
    float bot = 1.0f + c3 * LMS_10k;

    return powf(top / bot, p);
}

void XYZ_to_Jzazbz(float * InputXYZ, float * OutputJzazbz, uint64_t N)
{
    float * Jzazbz = OutputJzazbz;
    float * XYZ = InputXYZ;

    float M1[9];
    float M2[9];
    for (int i = 0; i < 9; ++i) {
        M1[i] = Jzazbz_M[i];
        M2[i] = Jzazbz_M2[i];
    }

    for (uint64_t i = 0; i < N; ++i, XYZ += 3, Jzazbz += 3)
    {
        float Z = XYZ[2];
        float X_ = (b * XYZ[0]) - ((b-1.0f) * Z);
        float Y_ = (g * XYZ[1]) - ((g-1.0f) * XYZ[0]);

        float L = (X_*M1[0]) + (Y_*M1[1]) + (Z*M1[2]);
        float M = (X_*M1[3]) + (Y_*M1[4]) + (Z*M1[5]);
        float S = (X_*M1[6]) + (Y_*M1[7]) + (Z*M1[8]);

        L = Jzazbz_forward_nonlinearf(L);
        M = Jzazbz_forward_nonlinearf(M);
        S = Jzazbz_forward_nonlinearf(S);

        float Iz = (L + M) / 2.0f;
        Jzazbz[0] = ((1.0f+d) * Iz) / (1.0f + d*Iz) - d0;
        Jzazbz[1] = M2[3] * L + M2[4] * M + M2[5] * S;
        Jzazbz[2] = M2[6] * L + M2[7] * M + M2[8] * S;
    }
}

void Jzazbz_to_XYZ(float * InputJzazbz, float * utputXYZ, uint64_t N)
{
    float * Jzazbz = InputJzazbz;
    float * XYZ = OutputXYZ;

    /* TODO: store inverted matrix here instead of inverting every time */
    double Mid[9];
    double M2d[9];
    invertMatrix(Jzazbz_M, Mid);
    invertMatrix(Jzazbz_M2, M2d);

    /* Converting to float */
    float Mi[9];
    float M2[9];
    for (int i = 0; i < 9; ++i) {
        Mi[i] = Mid[i];
        M2[i] = M2d[i];
    }

    for (uint64_t i = 0; i < N; ++i, Jzazbz += 3, XYZ += 3)
    {
        float Jz_plus_d0 = Jzazbz[0] + d0;
        float az = Jzazbz[1];
        float bz = Jzazbz[2];
        float Iz = Jz_plus_d0 / (1.0f + d - d*Jz_plus_d0);

        float L = M2[0]*Iz + M2[1]*az + M2[2]*bz;
        float M = M2[3]*Iz + M2[4]*az + M2[5]*bz;
        float S = M2[6]*Iz + M2[7]*az + M2[8]*bz;

        L = Jzazbz_inverse_nonlinearf(L);
        M = Jzazbz_inverse_nonlinearf(M);
        S = Jzazbz_inverse_nonlinearf(S);

        float Xp = L*Mi[0] + M*Mi[1] + S*Mi[2];
        float Yp = L*Mi[3] + M*Mi[4] + S*Mi[5];
        float Zp = L*Mi[6] + M*Mi[7] + S*Mi[8];

        float X = (Xp + (b-1.0f)*Zp) / b;
        XYZ[0] = X;
        XYZ[1] = ((Yp + (g-1.0f)*X) / g);
        XYZ[2] = Zp;
    }
}

#undef b
#undef g
#undef c1
#undef c2
#undef c3
#undef n
#undef p
#undef d
#undef d0