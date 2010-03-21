//
//
//
#ifndef __RB_RIM_IPL_H__
#define __RB_RIM_IPL_H__
#include <ruby.h>
#include <cv.h>
#include <narray.h>

int rb_rim_get_image_description(struct NARRAY *n_na,
                                 int *width,
                                 int *height,
                                 int *channels,
                                 int *depth);

VALUE rb_rim_ipl2image(IplImage *img);
IplImage *rb_rim_image2ipl(VALUE oRimImage);
IplImage *rb_rim_image2ipl_ref(VALUE oRimImage);

void rb_rim_ipl_free(IplImage *img);
#endif
