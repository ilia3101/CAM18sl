# CAM18sl
Testing CAM18sl model from this document: https://opg.optica.org/oe/fulltext.cfm?uri=oe-29-18-29257

The document has no mention of hue. However the model appears to have good hue linearity, maybe even better than Jz.

I have implemented CAM18sl in C and Python in this repo.

# Comparisons with other models

| CAM18sl | JzAzBz | CIELAB | CIELUV |
| :--: | :--: | :--: | :--: |
| ![gradient](images/CAM18sl_white-blue.bmp.png) | ![gradient](images/JzAzBz_white-blue.bmp.png) | ![gradient](images/CIELAB_white-blue.bmp.png) | ![gradient](images/Luv_white-blue.bmp.png) |
| ![gradient](images/CAM18sl_white-yellow.bmp.png) | ![gradient](images/JzAzBz_white-yellow.bmp.png) | ![gradient](images/CIELAB_white-yellow.bmp.png) | ![gradient](images/Luv_white-yellow.bmp.png) |
| ![gradient](images/CAM18sl_white-red.bmp.png) | ![gradient](images/JzAzBz_white-red.bmp.png) | ![gradient](images/CIELAB_white-red.bmp.png) | ![gradient](images/Luv_white-red.bmp.png) |
| ![gradient](images/CAM18sl_limegreen-blue.bmp.png) | ![gradient](images/JzAzBz_limegreen-blue.bmp.png) | ![gradient](images/CIELAB_limegreen-blue.bmp.png) | ![gradient](images/Luv_limegreen-blue.bmp.png) |
| ![gradient](images/CAM18sl_blue-red.bmp.png) | ![gradient](images/JzAzBz_blue-red.bmp.png) | ![gradient](images/CIELAB_blue-red.bmp.png) | ![gradient](images/Luv_blue-red.bmp.png) |
| ![gradient](images/CAM18sl_blue-yellow.bmp.png) | ![gradient](images/JzAzBz_blue-yellow.bmp.png) | ![gradient](images/CIELAB_blue-yellow.bmp.png) | ![gradient](images/Luv_blue-yellow.bmp.png) |
| ![gradient](images/CAM18sl_red-yellow.bmp.png) | ![gradient](images/JzAzBz_red-yellow.bmp.png) | ![gradient](images/CIELAB_red-yellow.bmp.png) | ![gradient](images/Luv_red-yellow.bmp.png) |

# Compiling:

To compile:
```
gcc -c gradient.c inverse_test.c cam18sl.c; gcc -lm -o inverse_test cam18sl.o inverse_test.o; gcc -lm -o gradient cam18sl.o gradient.o; rm *.o;
```

To run the gradient generator program:
```
./gradient
```

To run the inverse verification program:
```
./inverse_test
```