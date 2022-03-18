#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

#include "cam18sl.h"
#include "matrix/matrix.h"

void writebmp(unsigned char * data, int width, int height, char * filename) {
    int rowbytes = width*3+(4-(width*3%4))%4, imagesize = rowbytes*height, y;
    uint16_t header[] = {0x4D42,0,0,0,0,26,0,12,0,width,height,1,24};
    *((uint32_t *)(header+1)) = 26 + imagesize-((4-(width*3%4))%4);
    FILE * file = fopen(filename, "wb"); fwrite(header, 1, 26, file);
    if (file) {
        for (int i = 0; i < width*height; ++i) {int t = data[i*3]; data[i*3] = data[i*3+2]; data[i*3+2] = t;}
        for (y = height-1; y >= 0; --y) fwrite(data+(y*width*3), rowbytes, 1, file);
        fwrite(data, width*3, 1, file); fclose(file);
    }
}

/* to srgb */
double toR[] = {3.2404542, -1.5371385, -0.4985314, -0.9692660,  1.8760108,  0.0415560, 0.0556434, -0.2040259,  1.0572252};
/* to XYZ */
double toX[] = {0.4124, 0.3576, 0.1805, 0.2126, 0.7152, 0.0722, 0.0193, 0.1192, 0.9505};

double sRGB_2part(double u) {
    if(u<0)return 0;else if(u<0.0031308)return(323.0*u)/25.0; else if(u<1)return 1.055*pow(u,1.0/2.4)-0.055;else return 1.0;
}

double sRGB_2part_tolinear(double x) {
    if(x<0)return 0;else if(x<0.04045)return(25.0*x)/323.0; else if(x<1)return pow((x+0.055)/1.055, 2.4);else return 1.0;
}

uint8_t sRGB_encode(double u) {
    return (uint8_t)(255.0 * sRGB_2part(u));
}

double sRGB_decode(uint8_t x) {
    return sRGB_2part_tolinear(((double)x) / 255.0);
}

void XYZ_to_sRGB(double X, double Y, double Z, uint8_t * sRGBOut)
{
    double R = toR[0] * X + toR[1] * Y + toR[2] * Z;
    double G = toR[3] * X + toR[4] * Y + toR[5] * Z;
    double B = toR[6] * X + toR[7] * Y + toR[8] * Z;
    sRGBOut[0] = sRGB_encode(R);
    sRGBOut[1] = sRGB_encode(G);
    sRGBOut[2] = sRGB_encode(B);
}

void sRGB_to_XYZ(uint8_t r, uint8_t g, uint8_t b, double * XYZOut)
{
    double R = sRGB_decode(r);
    double G = sRGB_decode(g);
    double B = sRGB_decode(b);
    XYZOut[0] = toX[0] * R + toX[1] * G + toX[2] * B;
    XYZOut[1] = toX[3] * R + toX[4] * G + toX[5] * B;
    XYZOut[2] = toX[6] * R + toX[7] * G + toX[8] * B;
}

/********************************** CAM18sl ***********************************/

double lms2006_10_to_xyz[] = {
    1.93986443, -1.34664359, 0.43044935,
    0.69283932, 0.34967567, 0.00000000,
    0.00000000, 0.00000000, 2.14687945
};

void XYZ_to_CAM18sl_Qab(double X, double Y, double Z, double * QOut, double * aOut, double * bOut)
{
    double to_LMS[9]; invertMatrix(lms2006_10_to_xyz, to_LMS);
    double LMS[] = {X,Y,Z}; applyMatrix(LMS, to_LMS);
    cam18sl(LMS[0], LMS[1], LMS[2], 0, aOut, bOut, NULL, NULL, QOut);
}

void CAM18sl_Qab_to_XYZ(double Q, double a, double b, double * XOut, double * YOut, double * ZOut)
{
    double LMS[3];
    cam18sl_inverse(0, a, b, NULL, &Q, LMS, LMS+1, LMS+2);
    applyMatrix(LMS, lms2006_10_to_xyz);
    double * XYZ = LMS;
    *XOut = XYZ[0];
    *YOut = XYZ[1];
    *ZOut = XYZ[2];
}

/**************************** Chromaticity Linear *****************************/

/* Does nothing */
void nothing(double a, double b, double c, double * x, double * y, double * z)
{
    *x = a;
    *y = b;
    *z = c;
}

/******************************************************************************/

void DoGradientTest(void * CAMToXYZFunc, void * XYZToCAMFunc, uint8_t * rgb1, uint8_t * rgb2, char * filename)
{
    void (*to_XYZ)(double, double, double, double*, double*, double*) = CAMToXYZFunc;
    void (*to_CAM)(double, double, double, double*, double*, double*) = XYZToCAMFunc;

    double start_XYZ[3];
    double end_XYZ[3];
    sRGB_to_XYZ(rgb1[0], rgb1[1], rgb1[2], start_XYZ);
    sRGB_to_XYZ(rgb2[0], rgb2[1], rgb2[2], end_XYZ);
    double start_CAM[3];
    double end_CAM[3];
    to_CAM(start_XYZ[0], start_XYZ[1], start_XYZ[2], &start_CAM[0], &start_CAM[1], &start_CAM[2]);
    to_CAM(end_XYZ[0], end_XYZ[1], end_XYZ[2], &end_CAM[0], &end_CAM[1], &end_CAM[2]);

    int width = 300;
    int height = 100;
    uint8_t * data = malloc(width*height*sizeof(uint8_t)*3);
    for (int y = 0; y < height; ++y)
    {
        uint8_t * pix = data + (y * width * 3);
        for (int x = 0; x < width; ++x)
        {
            double fac = ((double)x) / ((double)width-1.0);
            double ifac = 1.0 - fac;

            double cam[3], xyz[3];
            for (int i = 0; i < 3; ++i) cam[i] = start_CAM[i]*ifac + end_CAM[i]*fac;
            to_XYZ(cam[0], cam[1], cam[2], xyz, xyz+1, xyz+2);

            XYZ_to_sRGB(xyz[0], xyz[1], xyz[2], pix);

            pix += 3;
        }
    }

    writebmp(data, width, height, filename);
}

int main()
{
    uint8_t achromatic[] = {232, 232, 232};

    uint8_t rgb_blue[] = {2, 2, 220};
    uint8_t rgb_red[] = {240, 2, 2};
    uint8_t rgb_yellow[] = {235, 235, 2};

    DoGradientTest(CAM18sl_Qab_to_XYZ, XYZ_to_CAM18sl_Qab, achromatic, rgb_blue, "CAM18sl_blue.bmp");
    DoGradientTest(CAM18sl_Qab_to_XYZ, XYZ_to_CAM18sl_Qab, achromatic, rgb_red, "CAM18sl_red.bmp");
    DoGradientTest(CAM18sl_Qab_to_XYZ, XYZ_to_CAM18sl_Qab, achromatic, rgb_yellow, "CAM18sl_yellow.bmp");

    DoGradientTest(nothing, nothing, achromatic, rgb_blue, "linear_blue.bmp");
    DoGradientTest(nothing, nothing, achromatic, rgb_red, "linear_red.bmp");
    DoGradientTest(nothing, nothing, achromatic, rgb_yellow, "linear_yellow.bmp");

    return 0;
}