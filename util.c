#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void writebmp(unsigned char * data, int width, int height, char * filename) {
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
static double toR[] = {3.2404542, -1.5371385, -0.4985314, -0.9692660,  1.8760108,  0.0415560, 0.0556434, -0.2040259,  1.0572252};
/* to XYZ */
static double toX[] = {0.4124, 0.3576, 0.1805, 0.2126, 0.7152, 0.0722, 0.0193, 0.1192, 0.9505};

static double sRGB_2part(double u) {
    if(u<0)return 0;else if(u<0.0031308)return(323.0*u)/25.0; else if(u<1)return 1.055*pow(u,1.0/2.4)-0.055;else return 1.0;
}

static double sRGB_2part_tolinear(double x) {
    if(x<0)return 0;else if(x<0.04045)return(25.0*x)/323.0; else if(x<1)return pow((x+0.055)/1.055, 2.4);else return 1.0;
}

static uint8_t sRGB_encode(double u) {
    return (uint8_t)(255.0 * sRGB_2part(u));
}

static double sRGB_decode(uint8_t x) {
    return sRGB_2part_tolinear(((double)x) / 255.0);
}

static void XYZ_to_sRGB(double X, double Y, double Z, uint8_t * sRGBOut)
{
    double R = toR[0] * X + toR[1] * Y + toR[2] * Z;
    double G = toR[3] * X + toR[4] * Y + toR[5] * Z;
    double B = toR[6] * X + toR[7] * Y + toR[8] * Z;
    sRGBOut[0] = sRGB_encode(R);
    sRGBOut[1] = sRGB_encode(G);
    sRGBOut[2] = sRGB_encode(B);
}

static void sRGB_to_XYZ(uint8_t r, uint8_t g, uint8_t b, double * XYZOut)
{
    double R = sRGB_decode(r);
    double G = sRGB_decode(g);
    double B = sRGB_decode(b);
    XYZOut[0] = toX[0] * R + toX[1] * G + toX[2] * B;
    XYZOut[1] = toX[3] * R + toX[4] * G + toX[5] * B;
    XYZOut[2] = toX[6] * R + toX[7] * G + toX[8] * B;
}

static uint8_t diagonal_flip[9] = { 0, 3, 6, 1, 4, 7, 2, 5, 8 };
#define diag_flip(X) diagonal_flip[(X)]

/* Multiplies matrices A and B to outputMatrix */
static void multiplyMatrices(double * A, double * B, double * outputMatrix)
{
    outputMatrix[0] = A[0] * B[0] + A[1] * B[3] + A[2] * B[6];
    outputMatrix[1] = A[0] * B[1] + A[1] * B[4] + A[2] * B[7];
    outputMatrix[2] = A[0] * B[2] + A[1] * B[5] + A[2] * B[8];
    outputMatrix[3] = A[3] * B[0] + A[4] * B[3] + A[5] * B[6];
    outputMatrix[4] = A[3] * B[1] + A[4] * B[4] + A[5] * B[7];
    outputMatrix[5] = A[3] * B[2] + A[4] * B[5] + A[5] * B[8];
    outputMatrix[6] = A[6] * B[0] + A[7] * B[3] + A[8] * B[6];
    outputMatrix[7] = A[6] * B[1] + A[7] * B[4] + A[8] * B[7];
    outputMatrix[8] = A[6] * B[2] + A[7] * B[5] + A[8] * B[8];
}

static void invertMatrix(double * inputMatrix, double * outputMatrix)
{
    for (int y = 0; y < 3; ++y)
    {
        for (int x = 0; x < 3; ++x)
        {
            /* Determenant locations for 2 x 2 */
            int dX[2] = { (x + 1) % 3, (x + 2) % 3 };
            int dY[2] = { 3 * ((y + 1) % 3), 3 * ((y + 2) % 3) };

            outputMatrix[ diag_flip(y*3 + x) ] = 
            (   /* Determinant caluclation 2 x 2 */
                  inputMatrix[dY[0] + dX[0]] 
                * inputMatrix[dY[1] + dX[1]]
                - inputMatrix[dY[0] + dX[1]] 
                * inputMatrix[dY[1] + dX[0]]
            );
        }
    }

    /* Calculate whole matrix determinant */
    double determinant = 1.0 / (
          inputMatrix[0] * ( inputMatrix[8] * inputMatrix[4] - inputMatrix[7] * inputMatrix[5] )
        - inputMatrix[3] * ( inputMatrix[8] * inputMatrix[1] - inputMatrix[7] * inputMatrix[2] )
        + inputMatrix[6] * ( inputMatrix[5] * inputMatrix[1] - inputMatrix[4] * inputMatrix[2] )
    );

    /* Multiply all elements by the determinant */
    for (int i = 0; i < 9; ++i) outputMatrix[i] *= determinant;
}

static void printMatrix(double * matrix)
{
    for (int i = 0; i < 9; i += 3)
        printf("[ %.4f, %.4f, %.4f ]\n", matrix[i], matrix[i+1], matrix[i+2]);
}

/* SLOW, V is vector, M is matrix */
static void applyMatrix(double * V, double * M)
{
    double V0 = M[0] * V[0] + M[1] * V[1] + M[2] * V[2];
    double V1 = M[3] * V[0] + M[4] * V[1] + M[5] * V[2];
    double V2 = M[6] * V[0] + M[7] * V[1] + M[8] * V[2];
    V[0] = V0;
    V[1] = V1;
    V[2] = V2;
}

