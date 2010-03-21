//
//
//
//
#include <ruby.h>
#include <narray.h>
#include <cv.h>
#include "../rim_ipl.h"

VALUE rb_rim_cv_CvtColor(int argc,VALUE *argv,VALUE self,int code)
{
    VALUE v_dst;
    struct NARRAY *src,*dst;
    IplImage *i_src,*i_dst;

    if(!IsNArray(self))
        rb_raise(rb_eTypeError,"Odd reciever was gaven.");

    GetNArray(self,src);

    if((argc==0)||(TYPE(argv[0])==T_NIL))
        v_dst=rb_funcall(self,rb_intern("[]"),0);
    else{
        if(!IsNArray(argv[0])){
            rb_raise(rb_eTypeError,"1st argument must be NArray.");
        }
        GetNArray(argv[0],dst);
        
        if((src->rank!=dst->rank)||(src->type!=dst->type))
            rb_raise(rb_eTypeError,"NArray type or rank miss matched.");

        for(int i=0;i<src->rank;i++){
            if((src->shape[i]!=dst->shape[i]))
                rb_raise(rb_eTypeError,"NArray shape miss matched.");
        }

        v_dst=argv[0];
    }


    i_src=rb_rim_image2ipl_ref(self);
    if(!i_src)
        rb_raise(rb_eTypeError,
                 "Failure in Rim::Image to IplImage convert.");
    i_dst=rb_rim_image2ipl_ref(v_dst);
    if(!i_dst){
        rb_rim_ipl_free(i_src);
        rb_raise(rb_eTypeError,
                 "Failure in Rim::Image to IplImage convert.");
    }

    cvCvtColor(i_src,i_dst,code);

    rb_rim_ipl_free(i_dst);
    rb_rim_ipl_free(i_src);

    return v_dst;
}

//
// Rim::Image#cv_bgr2hls(dst=nil)
//
VALUE rb_rim_cv_bgr2hls(int argc,VALUE *argv,VALUE self)
{
    return rb_rim_cv_CvtColor(argc,argv,self,CV_BGR2HLS);
}

//
// Rim::Image#cv_hls2bgr(dst=nil)
//
VALUE rb_rim_cv_hls2bgr(int argc,VALUE *argv,VALUE self)
{
    return rb_rim_cv_CvtColor(argc,argv,self,CV_HLS2BGR);
}



//
// Rim::Image#cv_binarize_ohtsu(maxval=255)
// Rim::Image#cv_binarize_ohtsu(dst=nil)
// Rim::Image#cv_binarize_ohtsu(dst=nil,maxval=255)
//
VALUE rb_rim_cv_binarize_ohtsu(int argc,VALUE *argv,VALUE self)
{
    int maxval=255;
    IplImage *dst=NULL;
    VALUE v_dst=NULL;

    if(!IsNArray(self))
        rb_raise(rb_eTypeError,"Odd reciever was gaven.");

    IplImage *src=rb_rim_image2ipl_ref(self);
    if((!src)||(src->nChannels!=1)){
        rb_rim_ipl_free(src);
        rb_raise(rb_eTypeError,"Reciever must be single channel.");
    }

    switch(argc){
    case 0:
        break;
    case 1:
        if(IsNArray(argv[0]))
            dst=rb_rim_image2ipl_ref(argv[0]);
        else
            maxval=FIX2INT(argv[0]);
        break;
    default:
        if(IsNArray(argv[0]))
            dst=rb_rim_image2ipl_ref(argv[0]);
        else{
            rb_rim_ipl_free(src);
            rb_raise(rb_eTypeError,"Odd argument gaven.");
        }
        maxval=FIX2INT(argv[1]);
    }

    if(dst){
        if((dst->nChannels!=1)||
           (dst->width!=src->width)||
           (dst->height!=src->height)){
            rb_rim_ipl_free(dst);
            rb_rim_ipl_free(src);
            rb_raise(rb_eTypeError,"Size mismatch.");
        }
        v_dst=argv[0];
    }
    else
        dst=cvCreateImage(cvGetSize(src),IPL_DEPTH_8U,1);


    cvThreshold(src,dst,0,maxval,CV_THRESH_BINARY|CV_THRESH_OTSU);

    if(!v_dst)
        v_dst=rb_rim_ipl2image(dst);

    rb_rim_ipl_free(dst);
    rb_rim_ipl_free(src);

    return v_dst;
}


void rb_cvfilters_init(VALUE mRim,VALUE cRimImage)
{
    rb_define_method(cRimImage,
                     "cv_bgr2hls",
                     (VALUE(*)(...))rb_rim_cv_bgr2hls,
                     -1);

    rb_define_method(cRimImage,
                     "cv_hls2bgr",
                     (VALUE(*)(...))rb_rim_cv_hls2bgr,
                     -1);


    rb_define_method(cRimImage,
                     "cv_binarize_ohtsu",
                     (VALUE(*)(...))rb_rim_cv_binarize_ohtsu,
                     -1);
}
