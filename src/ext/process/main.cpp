//
//
//
//
#include <ruby.h>
#include "labeling.h"
#include "cvfilters.h"

extern "C" void Init_rim_image_process()
{
    VALUE mRim=rb_define_module("Rim");
    VALUE cRimImage=
        rb_define_class_under(mRim,
                              "Image",
                              rb_const_get(rb_cObject,rb_intern("NVector")));
    
    rb_labeling_init(mRim,cRimImage);
    rb_cvfilters_init(mRim,cRimImage);
}
