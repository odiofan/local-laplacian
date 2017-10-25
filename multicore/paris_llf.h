#ifndef LOCAL_LAPLACIAN_H
#define LOCAL_LAPLACIAN_H
#include "Image.h"

double alpha_, beta_;

typedef struct{
    int start;
    int end;
} range;
Image im;
pixel_t *pim;
pixel_t *c_Buff;
range row_range,col_range;
pixel_t * paris_llf(int img_w,int img_h,pixel_t *global_pointer, double alpha, double beta,double sigma);

#endif


