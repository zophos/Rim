//
//
//
//
#include <ruby.h>
#include <narray.h>
#include <cv.h>
#include "../rim_ipl.h"
#include <iostream>

//////////////////////////////////////////////////////////////////////////
//
// Rim::Box2D
//
VALUE rb_rim_box2d_alloc(VALUE klass)
{
    CvBox2D *ptr=ALLOC(CvBox2D);

    return Data_Wrap_Struct(klass,0,-1,ptr);
}


VALUE rb_rim_box2d_new(CvBox2D *box)
{
    CvBox2D *ptr;

    VALUE self=
        rb_funcall(rb_const_get(rb_const_get(rb_cObject,rb_intern("Rim")),
                                rb_intern("Box2D")),
                   rb_intern("new"),0);

    Data_Get_Struct(self,CvBox2D,ptr);
    *ptr=*box;

    return self;
}

VALUE rb_rim_box2d_center(VALUE self)
{
    CvBox2D *ptr;
    VALUE rimpoint2d;

    Data_Get_Struct(self,CvBox2D,ptr);
    rimpoint2d=
        rb_funcall(rb_const_get(rb_const_get(rb_cObject,rb_intern("Rim")),
                                rb_intern("Point2D")),
                   rb_intern("new"),
                   2,
                   rb_float_new(ptr->center.x),
                   rb_float_new(ptr->center.y));

    return rimpoint2d;
}


VALUE rb_rim_box2d_width(VALUE self)
{
    CvBox2D *ptr;
    Data_Get_Struct(self,CvBox2D,ptr);

    return rb_float_new(ptr->size.width);
}
VALUE rb_rim_box2d_height(VALUE self)
{
    CvBox2D *ptr;
    Data_Get_Struct(self,CvBox2D,ptr);

    return rb_float_new(ptr->size.height);
}
VALUE rb_rim_box2d_angle(VALUE self)
{
    CvBox2D *ptr;
    Data_Get_Struct(self,CvBox2D,ptr);

    return rb_float_new(ptr->angle);
}


VALUE rb_rim_box2d_to_point(VALUE self)
{
    CvBox2D *ptr;
    CvPoint2D32f pt[4];

    Data_Get_Struct(self,CvBox2D,ptr);

    cvBoxPoints(*ptr,pt);

    VALUE ary=rb_ary_new();

    //rb_gc_mark(ar);

    VALUE cRimPoint2D=rb_const_get(rb_const_get(rb_cObject,
                                                rb_intern("Rim")),
                                   rb_intern("Point2D"));
    ID idRimPoint2D_new=rb_intern("new");
    for(int i=0;i<4;i++){
        rb_ary_push(ary,
                    rb_funcall(cRimPoint2D,
                               idRimPoint2D_new,
                               2,
                               rb_float_new(pt[i].x),
                               rb_float_new(pt[i].y)));
    }

    return ary;
}


VALUE rb_rim_cv_min_area_rect(VALUE self)
{
    if(!IsNArray(self))
        rb_raise(rb_eTypeError,"Odd reciever was gaven.");
    
    IplImage *i_src=rb_rim_image2ipl(self);

    CvMemStorage *storage=cvCreateMemStorage(0);
    CvSeq *contours=0;

    cvFindContours(i_src,
                   storage,
                   &contours,
                   sizeof(CvContour),
                   CV_RETR_EXTERNAL,
                   CV_CHAIN_APPROX_NONE);

    CvBox2D b=cvMinAreaRect2(contours);
    VALUE box=rb_rim_box2d_new(&b);

    cvReleaseMemStorage(&storage);
    rb_rim_ipl_free(i_src);

    return box;
}


VALUE rb_rim_cv_find_courner_harris(VALUE self,
                                    VALUE tmp1,
                                    VALUE tmp2,
                                    VALUE msk)
{
    int n_corners=50;
    CvPoint2D32f corners[n_corners];

    
    if((!IsNArray(self))||(!IsNArray(tmp1))||!(IsNArray(tmp2)))
        rb_raise(rb_eTypeError,"Odd reciever was gaven.");

    IplImage *src=rb_rim_image2ipl_ref(self);
    IplImage *eig=rb_rim_image2ipl_ref(tmp1);
    IplImage *tmp=rb_rim_image2ipl_ref(tmp2);
    
    IplImage *mask=NULL;
    if(IsNArray(msk))
        mask=rb_rim_image2ipl_ref(msk);

    cvGoodFeaturesToTrack(src,
                          eig,
                          tmp,
                          corners,
                          &n_corners,
                          0.2,   // quality level
                          9,     // minimum distance
                          mask,  // mask
                          5,     // Smoothing size
                          1,     // use Harris
                          0.01); // Harris Option

    cvFindCornerSubPix(src,
                       corners,
                       n_corners,
                       cvSize(5,5), // window size
                       cvSize(-1,-1), // zero zone
                       cvTermCriteria(CV_TERMCRIT_ITER|
                                      CV_TERMCRIT_EPS,
                                      20,
                                      0.03));
    
    VALUE ary=rb_ary_new();
    VALUE cRimPoint2D=rb_const_get(rb_const_get(rb_cObject,
                                                rb_intern("Rim")),
                                   rb_intern("Point2D"));
    ID idNew=rb_intern("new");

    for(int i=0;i<n_corners;i++){
        rb_ary_push(ary,
                    rb_funcall(cRimPoint2D,
                               idNew,
                               2,
                               rb_float_new(corners[i].x),
                               rb_float_new(corners[i].y)));

    }

    if(mask)
        rb_rim_ipl_free(mask);
        
    rb_rim_ipl_free(tmp);
    rb_rim_ipl_free(eig);
    rb_rim_ipl_free(src);

    return ary;
}


VALUE rb_rim_find_squire_vertex(VALUE self,
                                VALUE v_min_complexity,
                                VALUE v_max_complexity,
                                VALUE v_area_ratio,
                                VALUE v_poly_approx)
{

    if(!IsNArray(self))
        rb_raise(rb_eTypeError,"Odd reciever was gaven.");

    double min_complexity=NUM2DBL(v_min_complexity);
    double max_complexity=NUM2DBL(v_max_complexity);
    double area_ratio=NUM2DBL(v_area_ratio);
    double poly_approx=NUM2DBL(v_poly_approx);


    IplImage *src=rb_rim_image2ipl(self);
    
    CvMemStorage *storage=cvCreateMemStorage(0);
    CvSeq *contours=0;

    cvFindContours(src,
                   storage,
                   &contours,
                   sizeof(CvContour),
                   CV_RETR_EXTERNAL,
                   CV_CHAIN_APPROX_NONE);
    
    double area=fabs(cvContourArea(contours));
    double arclen=cvArcLength(contours,CV_WHOLE_SEQ,1);
    double val=arclen*arclen/area;
    if(val<min_complexity||val>max_complexity){
        cvReleaseMemStorage(&storage);
        rb_rim_ipl_free(src);
        return Qnil;
    }

    CvBox2D minrect=cvMinAreaRect2(contours);
    double minrect_area=minrect.size.width*minrect.size.height;
    if(area/minrect_area<area_ratio){
        cvReleaseMemStorage(&storage);
        rb_rim_ipl_free(src);
        return Qnil;
    }

    /*
    CvSeq *seq=cvConvexityDefects(contours,cvConvexHull2(contours));
    CvSeqReader reader;
    cvStartReadSeq(seq,&reader,0);
    VALUE ret=Qtrue;
    for(int i=0;i<seq->total;i++){
        CvConvexityDefect cd;
        CV_READ_SEQ_ELEM(cd,reader);
        if(cd.depth>3.0){
            ret=Qnil;
            break;
        }
    }
    */

    contours=cvApproxPoly(contours,
                          sizeof(CvContour),NULL,
                          CV_POLY_APPROX_DP,
                          poly_approx);
    if(contours->total!=4){
        cvReleaseMemStorage(&storage);
        rb_rim_ipl_free(src);
        return Qnil;
    }

    VALUE cRimPoint2D=rb_const_get(rb_const_get(rb_cObject,
                                                rb_intern("Rim")),
                                   rb_intern("Point2D"));
    ID idRimPoint2D_new=rb_intern("new");

    VALUE ary=rb_ary_new();

    CvSeqReader reader;
    cvStartReadSeq(contours,&reader,0);
    for(int i=0;i<contours->total;i++){
        CvPoint pt;
        CV_READ_SEQ_ELEM(pt,reader);

        rb_ary_push(ary,
                    rb_funcall(cRimPoint2D,
                               idRimPoint2D_new,
                               2,
                               rb_float_new((float)pt.x),
                               rb_float_new((float)pt.y)));
    }

    cvReleaseMemStorage(&storage);
    rb_rim_ipl_free(src);

    return ary;
}

VALUE rb_rim_fill_hole(VALUE self)
{
    if(!IsNArray(self))
        rb_raise(rb_eTypeError,"Odd reciever was gaven.");

    IplImage *src=rb_rim_image2ipl(self);

    CvMemStorage *storage=cvCreateMemStorage(0);
    CvSeq *contours=0;

    cvFindContours(src,
                   storage,
                   &contours,
                   sizeof(CvContour),
                   CV_RETR_EXTERNAL,
                   CV_CHAIN_APPROX_NONE);

    cvDrawContours(src,
                   contours,
                   CV_RGB(255,255,255),
                   CV_RGB(255,255,255),
                   0,
                   CV_FILLED);

    VALUE ret=rb_rim_ipl2image(src);

    cvReleaseMemStorage(&storage);
    rb_rim_ipl_free(src);

    return ret;
}



VALUE rb_rim_cv_binarize_adaptive(int argc,VALUE *argv,VALUE self)
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


    cvAdaptiveThreshold(src,
                        dst,
                        (double)maxval,
                        CV_ADAPTIVE_THRESH_MEAN_C,
                        CV_THRESH_BINARY,
                        7);

    if(!v_dst)
        v_dst=rb_rim_ipl2image(dst);

    rb_rim_ipl_free(dst);
    rb_rim_ipl_free(src);

    return v_dst;
}


VALUE rb_rim_cv_warp_perspective(int argc,VALUE *argv,VALUE self)
{
    VALUE mat,dst=NULL;
    struct NARRAY *na_mat;
    int mat_type=0;

    if(!IsNArray(self))
        rb_raise(rb_eTypeError,"Odd reciever was gaven.");
    
    if(rb_scan_args(argc,argv,"11",&mat,&dst)==2){
        if(!IsNArray(dst))
            rb_raise(rb_eTypeError,"2nd argument must be NArray");
    }
    else
        dst=NULL;
    
     //
     // create rotation matrix
     //
    if(!IsNArray(mat))
        rb_raise(rb_eTypeError,"1st argument must be NArray");
    
    GetNArray(mat,na_mat);
    if(na_mat->rank!=2 || na_mat->shape[0]!=3 || na_mat->shape[1]!=3)
         rb_raise(rb_eTypeError,"argument shapes must be 3x3");
    
    switch(na_mat->type){
    case NA_SFLOAT:
        mat_type=CV_32FC1;
        break;
    case NA_DFLOAT:
        mat_type=CV_64FC1;
        break;
    default:
        rb_raise(rb_eTypeError,"argument type must be FLOAT");
    }
    CvMat *rot3x3=cvCreateMatHeader(3,3,mat_type);
    cvSetData(rot3x3,
              na_mat->ptr,
              na_sizeof[na_mat->type]*na_mat->shape[0]);
     
    //
    // create perspective warp image
    //
    IplImage *src_img=rb_rim_image2ipl_ref(self);
    IplImage *dst_img=NULL;    
    if(dst)
        dst_img=rb_rim_image2ipl_ref(dst);
    else
        dst_img=cvCloneImage(src_img);

    cvWarpPerspective(src_img,dst_img,rot3x3);


    if(!dst)
        dst=rb_rim_ipl2image(dst_img);


    rb_rim_ipl_free(dst_img);
    rb_rim_ipl_free(src_img);
    cvReleaseMatHeader(&rot3x3);
    
    return dst;
}


extern "C" void Init_rim_demo()
{
    VALUE mRim=rb_define_module("Rim");
    VALUE cRimImage=
        rb_define_class_under(mRim,
                              "Image",
                              rb_const_get(rb_cObject,rb_intern("NVector")));

    
    VALUE cRimBox2D=rb_define_class_under(mRim,
                                          "Box2D",
                                          rb_cObject);

    rb_define_alloc_func(cRimBox2D,rb_rim_box2d_alloc);

    rb_define_method(cRimBox2D,
                     "center",
                     (VALUE(*)(...))rb_rim_box2d_center,
                     0);
    rb_define_method(cRimBox2D,
                     "width",
                     (VALUE(*)(...))rb_rim_box2d_width,
                     0);
    rb_define_method(cRimBox2D,
                     "height",
                     (VALUE(*)(...))rb_rim_box2d_height,
                     0);
    rb_define_method(cRimBox2D,
                     "angle",
                     (VALUE(*)(...))rb_rim_box2d_angle,
                     0);
    rb_define_method(cRimBox2D,
                     "to_point",
                     (VALUE(*)(...))rb_rim_box2d_to_point,
                     0);


    rb_define_method(cRimImage,
                     "cv_min_area_rect",
                     (VALUE(*)(...))rb_rim_cv_min_area_rect,
                     0);


    rb_define_method(cRimImage,
                     "cv_find_courner_harris",
                     (VALUE(*)(...))rb_rim_cv_find_courner_harris,
                     3);

    rb_define_method(cRimImage,
                     "find_squire_vertex",
                     (VALUE(*)(...))rb_rim_find_squire_vertex,
                     4);

    rb_define_method(cRimImage,
                     "fill_hole",
                     (VALUE(*)(...))rb_rim_fill_hole,
                     0);

    rb_define_method(cRimImage,
                     "cv_binarize_adaptive",
                     (VALUE(*)(...))rb_rim_cv_binarize_adaptive,
                     -1);
    
    rb_define_method(cRimImage,
                     "cv_warp_perspective",
                     (VALUE(*)(...))rb_rim_cv_warp_perspective,
                     -1);
}
