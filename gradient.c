#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

#include "util.c"
#include "jz_and_lab.c"

#include "cam18sl.h"

/********************************** CAM18sl ***********************************/

double lms2006_10_to_xyz2006_10[] = {
    1.93986443, -1.34664359, 0.43044935,
    0.69283932, 0.34967567, 0.00000000,
    0.00000000, 0.00000000, 2.14687945
};

/* To represent 100 nits. */
#define CAM18sl_brightness 100

/* Surround (achromatic) cone value. Perhaps I should be setting this to '100' as I generally use a white background */
#define CAM18sl_BG_cone_value 0

void XYZ_to_CAM18sl_Qab(double X, double Y, double Z, double * QOut, double * aOut, double * bOut)
{
    double to_LMS[9]; invertMatrix(lms2006_10_to_xyz2006_10, to_LMS);
    double LMS[3] = {X,Y,Z}; applyMatrix(LMS, to_LMS);

    /* Match D65 (sRGB(255,255,255)) to 1,1,1 in 2006 10 degree LMS which CAM18 uses,
     * then multiply by the nits factor. */
    double D65_in_LMS[3];
    sRGB_to_XYZ(255,255,255,D65_in_LMS);
    applyMatrix(D65_in_LMS, to_LMS);

    for (int i=0;i<3;++i) LMS[i] *= CAM18sl_brightness / D65_in_LMS[i];
    cam18sl(LMS[0], LMS[1], LMS[2], CAM18sl_BG_cone_value, aOut, bOut, NULL, NULL, QOut);
}

void CAM18sl_Qab_to_XYZ(double Q, double a, double b, double * XOut, double * YOut, double * ZOut)
{
    double LMS[3];
    double to_LMS[9]; invertMatrix(lms2006_10_to_xyz2006_10, to_LMS);

    /* Find D65 in LMS so we can undo the VOn kries adaptation done in the forward transform */
    double D65_in_LMS[3];
    sRGB_to_XYZ(255,255,255,D65_in_LMS);
    applyMatrix(D65_in_LMS, to_LMS);

    cam18sl_inverse(CAM18sl_BG_cone_value, a, b, NULL, &Q, LMS, LMS+1, LMS+2);
    for (int i=0;i<3;++i) LMS[i] /= CAM18sl_brightness / D65_in_LMS[i];
    applyMatrix(LMS, lms2006_10_to_xyz2006_10);

    double * XYZ = LMS;
    *XOut = XYZ[0];
    *YOut = XYZ[1];
    *ZOut = XYZ[2];
}

/*********************************** JzAzBz ***********************************/

#define Jzazbz_brightness 100

void XYZ_to_Jz(double X, double Y, double Z, double * JzOut, double * azOut, double * bzOut)
{
    float XYZ[3] = {X,Y,Z};
    float JzAzBz[3];

    for (int i=0;i<3;++i) XYZ[i] *= Jzazbz_brightness;

    XYZ_to_Jzazbz(XYZ, JzAzBz, 1);

    *JzOut = JzAzBz[0];
    *azOut = JzAzBz[1];
    *bzOut = JzAzBz[2];
}

void Jz_to_XYZ(double Jz, double az, double bz, double * XOut, double * YOut, double * ZOut)
{
    float JzAzBz[3] = {Jz,az,bz};
    float XYZ[3];

    Jzazbz_to_XYZ(JzAzBz, XYZ, 1);

    for (int i=0;i<3;++i) XYZ[i] /= Jzazbz_brightness;

    *XOut = XYZ[0];
    *YOut = XYZ[1];
    *ZOut = XYZ[2];
}

/*********************************** CIELAB ***********************************/

void XYZ_to_Lab(double X, double Y, double Z, double * LOut, double * aOut, double * bOut)
{
    float XYZ[3] = {X,Y,Z};
    float Lab[3];

    /* For lab adaptaion */
    double w[3]; sRGB_to_XYZ(255,255,255,w);
    float D65_in_XYZ[3] = {w[0], w[1], w[2]};

    XYZ_to_CIELAB(XYZ, Lab, D65_in_XYZ, 1);

    *LOut = Lab[0];
    *aOut = Lab[1];
    *bOut = Lab[2];
}

void Lab_to_XYZ(double L, double a, double b, double * XOut, double * YOut, double * ZOut)
{
    float Lab[3] = {L,a,b};
    float XYZ[3];

    /* For lab adaptaion */
    double w[3]; sRGB_to_XYZ(255,255,255,w);
    float D65_in_XYZ[3] = {w[0], w[1], w[2]};

    CIELAB_to_XYZ(Lab, XYZ, D65_in_XYZ, 1);

    *XOut = XYZ[0];
    *YOut = XYZ[1];
    *ZOut = XYZ[2];
}

/************************* Chromaticity Linear (Luv) ***************************/

/* Approximate implementatino of Luv, using cube root instead of the whole
 * curve. It's fine, this would only affect dark values anyway. */

float d65_v = 0.465;
float d65_u = 0.23;

void XYZ_to_Luv(double X, double Y, double Z, double * L, double * u, double * v)
{
    float d = X + (15.0f * Y) + (3.0f * Z);
    float _L = cbrt(Y);
    *L = _L;
    *u = ((4.0f * X) / d - d65_u) * _L;
    *v = ((9.0f * Y) / d - d65_v) * _L;
}

void Luv_to_XYZ(double L, double u, double v, double * x, double * y, double * z)
{
    u = u/L + d65_u;
    v = v/L + d65_v;
    float Y = L*L*L;
    *x = Y * (9.0f*u) / (4.0f*v);
    *y = Y;
    *z = Y * (12.0f - 3.0f*u - 20.0f*v) / (4.0f*v);
}

/********************************** sRGB LOL **********************************/

/* Does nothing */
void cam_xyz2srgb(double x, double y, double z, double * r, double * g, double * b)
{
    uint8_t srgb[3];
    XYZ_to_sRGB(x,y,z,srgb);
    *r = srgb[0];
    *g = srgb[1];
    *b = srgb[2];
}

void cam_srgb2xyz(double r, double g, double b, double * x, double * y, double * z)
{
    double XYZ[3];
    sRGB_to_XYZ(r, g, b, XYZ);
    *x = XYZ[0];
    *y = XYZ[1];
    *z = XYZ[2];
}

/******************************************************************************/

/* structrue for defining a cam to gradient test */
typedef struct {
    void (*to_XYZ)(double, double, double, double*, double*, double*);
    void (*to_CAM)(double, double, double, double*, double*, double*);
    char * name;
} cam_t;

void DoGradientTest(cam_t * model, uint8_t * rgb1, uint8_t * rgb2, char * filename)
{
    double start_XYZ[3];
    double end_XYZ[3];
    sRGB_to_XYZ(rgb1[0], rgb1[1], rgb1[2], start_XYZ);
    sRGB_to_XYZ(rgb2[0], rgb2[1], rgb2[2], end_XYZ);
    double start_CAM[3];
    double end_CAM[3];
    model->to_CAM(start_XYZ[0], start_XYZ[1], start_XYZ[2], &start_CAM[0], &start_CAM[1], &start_CAM[2]);
    model->to_CAM(end_XYZ[0], end_XYZ[1], end_XYZ[2], &end_CAM[0], &end_CAM[1], &end_CAM[2]);

    int width = 300;
    int height = 100;
    uint8_t * data = calloc(width*height,sizeof(uint8_t)*3);
    int border = 3;
    for (int y = border; y < height-border; ++y)
    {
        uint8_t * pix = data + ((y * width + border) * 3);
        for (int x = border; x < width-border; ++x)
        {
            double fac = ((double)x-border) / ((double)width-1.0-2.0*border);
            double ifac = 1.0 - fac;

            double cam[3], xyz[3];
            for (int i = 0; i < 3; ++i) cam[i] = start_CAM[i]*ifac + end_CAM[i]*fac;
            model->to_XYZ(cam[0], cam[1], cam[2], xyz, xyz+1, xyz+2);

            XYZ_to_sRGB(xyz[0], xyz[1], xyz[2], pix);

            pix += 3;
        }
    }

    writebmp(data, width, height, filename);
    free(data);
}

typedef struct {
    uint8_t * rgb_from;
    uint8_t * rgb_to;
    char * name;
} gradient_t;

int main()
{
    uint8_t achromatic[] = {232, 232, 232};
    uint8_t blue[] = {2, 2, 220};
    uint8_t red[] = {240, 32, 2};
    uint8_t yellow[] = {254, 214, 2};
    uint8_t limegreen[] = {1, 235, 34};

    cam_t cams[] = {
        {
            .to_XYZ = CAM18sl_Qab_to_XYZ,
            .to_CAM = XYZ_to_CAM18sl_Qab,
            .name = "CAM18sl"
        },
        {
            .to_XYZ = Jz_to_XYZ,
            .to_CAM = XYZ_to_Jz,
            .name = "JzAzBz"
        },
        {
            .to_XYZ = Lab_to_XYZ,
            .to_CAM = XYZ_to_Lab,
            .name = "CIELAB"
        },
        {
            .to_XYZ = Luv_to_XYZ,
            .to_CAM = XYZ_to_Luv,
            .name = "Luv"
        },
        /* sRGB disabled. */
        /* {
            .to_XYZ = cam_srgb2xyz,
            .to_CAM = cam_xyz2srgb,
            .name = "sRGB"
        }, */
    };

    gradient_t gradients[] = {
        {
            .rgb_from = achromatic,
            .rgb_to = blue,
            .name = "white-blue"
        },
        {
            .rgb_from = achromatic,
            .rgb_to = yellow,
            .name = "white-yellow"
        },
        {
            .rgb_from = achromatic,
            .rgb_to = red,
            .name = "white-red"
        },
        {
            .rgb_from = limegreen,
            .rgb_to = blue,
            .name = "limegreen-blue"
        },
        {
            .rgb_from = blue,
            .rgb_to = red,
            .name = "blue-red"
        },
        {
            .rgb_from = blue,
            .rgb_to = yellow,
            .name = "blue-yellow"
        },
        {
            .rgb_from = red,
            .rgb_to = yellow,
            .name = "red-yellow"
        }
    };

#ifdef __APPLE__
    FILE * comparisons_output = fopen("comparisons.md", "w");
    fprintf(comparisons_output, "|");
    for (int c = 0; c < sizeof(cams)/sizeof(cams[0]); ++c)
        fprintf(comparisons_output, " %s |", cams[c].name);
    fprintf(comparisons_output, "\n|");
    for (int c = 0; c < sizeof(cams)/sizeof(cams[0]); ++c)
        fprintf(comparisons_output, " :--: |");
#endif

    for (int g = 0; g < sizeof(gradients)/sizeof(gradients[0]); ++g)
    {
        gradient_t * gradient = &gradients[g];
        fprintf(comparisons_output, "\n|");
        for (int c = 0; c < sizeof(cams)/sizeof(cams[0]); ++c)
        {
            cam_t * cam = &cams[c];

            char filename[100];
            snprintf(filename, sizeof(filename)-1, "images/%s_%s.bmp", cam->name, gradient->name);
            puts(filename);

            DoGradientTest(cam, gradient->rgb_from, gradient->rgb_to, filename);

#ifdef __APPLE__
            char command[1000];
            snprintf(command, sizeof(command)-1, "sips -s format png %s --out %s.png", filename, filename);
            system(command);
            fprintf(comparisons_output, " ![gradient](%s.png) |", filename);  
#endif
        }
    }

    puts("Done");



#ifdef __APPLE__
    fclose(comparisons_output);
    system("rm images/*.bmp");
#endif

    return 0;
}