//
//
//
//
#include <ruby.h>
#include <narray.h>
#include "imura-naist-labeling.h"

typedef struct{
    int id;
    int n_pixels;
    int min_x;
    int min_y;
    int max_x;
    int max_y;
    float gx;
    float gy;
} RimImageLabelInfo;


//////////////////////////////////////////////////////////////////////////
//
// Rim::Image::LabelInfo
//
VALUE rb_rim_image_labelinfo_alloc(VALUE klass)
{
    RimImageLabelInfo *ptr=ALLOC(RimImageLabelInfo);

    return Data_Wrap_Struct(klass,0,-1,ptr);
}


VALUE rb_rim_image_labelinfo_new(Labeling<unsigned char,
                                 long>::RegionInfo *ri)
{
    RimImageLabelInfo *ptr;

    VALUE self=
        rb_funcall(rb_const_get(
                       rb_const_get(rb_const_get(rb_cObject,
                                                 rb_intern("Rim")),
                                    rb_intern("Image")),
                       rb_intern("LabelInfo")),
                   rb_intern("new"),0);

    Data_Get_Struct(self,RimImageLabelInfo,ptr);

    ptr->id=(int)(ri->GetResult());
    ptr->n_pixels=ri->GetNumOfPixels();

    ri->GetMin(ptr->min_x,ptr->min_y);
    ri->GetMax(ptr->max_x,ptr->max_y);

    ri->GetCenterOfGravity(ptr->gx,ptr->gy);

    return self;
}

//
// Rim::Image::LabelInfo#id
//
VALUE rb_rim_image_labelinfo_id(VALUE self)
{
    RimImageLabelInfo *ptr;

    Data_Get_Struct(self,RimImageLabelInfo,ptr);
    
    return INT2FIX(ptr->id);
}

//
// Rim::Image::LabelInfo#area
//
VALUE rb_rim_image_labelinfo_area(VALUE *self)
{
    RimImageLabelInfo *ptr;

    Data_Get_Struct(self,RimImageLabelInfo,ptr);
    
    return INT2FIX(ptr->n_pixels);
}

//
// Rim::Image::LabelInfo#center_of_gravity
//
VALUE rb_rim_image_labelinfo_cog(VALUE *self)
{
    RimImageLabelInfo *ptr;
    VALUE rimpoint2d;

    Data_Get_Struct(self,RimImageLabelInfo,ptr);

    rimpoint2d=
        rb_funcall(rb_const_get(rb_const_get(rb_cObject,rb_intern("Rim")),
                                rb_intern("Point2D")),
                   rb_intern("new"),
                   2,
                   rb_float_new(ptr->gx),
                   rb_float_new(ptr->gy));

    return rimpoint2d;
}

//
// Rim::Image::LabelInfo#rectangle
//
VALUE rb_rim_image_labelinfo_rect(VALUE *self)
{
    RimImageLabelInfo *ptr;
    VALUE mRim,cP2d,lt,rb,rect;

    Data_Get_Struct(self,RimImageLabelInfo,ptr);

    mRim=rb_const_get(rb_cObject,rb_intern("Rim"));
    cP2d=rb_const_get(mRim,rb_intern("Point2D"));

    lt=rb_funcall(cP2d,
                  rb_intern("new"),
                  2,
                  rb_float_new(ptr->min_x),
                  rb_float_new(ptr->min_y));

    rb=rb_funcall(cP2d,
                  rb_intern("new"),
                  2,
                  rb_float_new(ptr->max_x),
                  rb_float_new(ptr->max_y));

    rect=rb_funcall(rb_const_get(mRim,
                                 rb_intern("Rectangle")),
                    rb_intern("new"),
                    2,
                    lt,
                    rb);
    
    return rect;
}


//
// Rim::Image#labelinfo
//
VALUE rb_rim_image_labelinfo(VALUE self)
{
    VALUE ar=rb_iv_get(self,"@labelinfo");
    Check_Type(ar,T_ARRAY);
    return rb_ary_dup(ar);
}


//
// Rim::Image#labeling(min_area=0)
//
VALUE rb_rim_image_labeling(int argc,VALUE *argv,VALUE self)
{
    struct NARRAY *na_src,*na_dst;
    int width,height,channels,depth,min_area=0,i,n;
    VALUE opt,dst,ar;
    Labeling<unsigned char,long> labeling;
    
    
    if(!IsNArray(self))
        rb_raise(rb_eTypeError,"Odd reciever was gaven.");
    
    GetNArray(self,na_src);
    
    switch(na_src->rank){
    case 2:
        channels=1;
        width=na_src->shape[0];
        height=na_src->shape[1];
        break;
    case 3:
        channels=na_src->shape[0];
        width=na_src->shape[1];
        height=na_src->shape[2];
        break;
    default:
        rb_raise(rb_eTypeError,"Odd reciever was gaven.");
    }

    if((na_src->type!=NA_BYTE)||(channels!=1))
        rb_raise(rb_eTypeError,"Odd reciever was gaven.");
    
    
    if(rb_scan_args(argc,argv,"01",&opt)==1)
        min_area=NUM2LONG(opt);


    dst=rb_funcall(rb_const_get(rb_const_get(rb_cObject,rb_intern("Rim")),
                                rb_intern("Image")),
                   rb_intern("lint"),
                   3,
                   INT2FIX(1),
                   INT2FIX(width),
                   INT2FIX(height));
    GetNArray(dst,na_dst);
    
    labeling.Exec((unsigned char *)na_src->ptr,
                  (long *)na_dst->ptr,
                  width,
                  height,
                  (min_area?true:false),
                  min_area);
    
    ar=rb_ary_new();

    n=labeling.GetNumOfResultRegions();
    for(i=0;i<n;i++)
        rb_ary_push(ar,
                    rb_rim_image_labelinfo_new(
                        labeling.GetResultRegionInfo(i)));

    rb_iv_set(dst,"@labelinfo",ar);
    rb_define_singleton_method(dst,
                               "labelinfo",
                               (VALUE(*)(...))rb_rim_image_labelinfo,
                               0);
    return dst;
}


void rb_labeling_init(VALUE mRim,VALUE cRimImage)
{
    VALUE cRimLabelInfo=rb_define_class_under(cRimImage,
                                              "LabelInfo",
                                              rb_cObject);

    rb_define_alloc_func(cRimLabelInfo,
                         rb_rim_image_labelinfo_alloc);

    
    rb_define_method(cRimLabelInfo,
                     "id",
                     (VALUE(*)(...))rb_rim_image_labelinfo_id,
                     0);
    rb_define_method(cRimLabelInfo,
                     "area",
                     (VALUE(*)(...))rb_rim_image_labelinfo_area,
                     0);
    rb_define_method(cRimLabelInfo,
                     "center_of_gravity",
                     (VALUE(*)(...))rb_rim_image_labelinfo_cog,
                     0);
    rb_define_method(cRimLabelInfo,
                     "rectangle",
                     (VALUE(*)(...))rb_rim_image_labelinfo_rect,
                     0);


    rb_define_method(cRimImage,
                     "labeling",
                     (VALUE(*)(...))rb_rim_image_labeling,
                     -1);
}
