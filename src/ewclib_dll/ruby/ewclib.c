#include <ruby.h>
#include <narray.h>
#include "../ewcdll.h"

//
// EwcLib.open(width,height,fps)
//
VALUE rb_ewc_open(VALUE self,VALUE width,VALUE height,VALUE fps)
    
{
    VALUE mEwcLib;
    int w,h;
    double f;

    w=NUM2INT(width);
    h=NUM2INT(height);
    f=NUM2DBL(fps);

    if(!EWCDLL_Open(w,h,f)){
        mEwcLib=rb_const_get(rb_cObject,rb_intern("EwcLib"));
        rb_cv_set(mEwcLib,"@@capture_image_width",width);
        rb_cv_set(mEwcLib,"@@capture_image_height",height);
        rb_cv_set(mEwcLib,"@@capture_frame_rate",fps);

        return self;
    }
    return Qnil;
}

//
// EwcLib.close
//
VALUE rb_ewc_close(VALUE self)
{
    VALUE mEwcLib;

    if(!EWCDLL_Close()){
        mEwcLib=rb_const_get(rb_cObject,rb_intern("EwcLib"));
        rb_cv_set(mEwcLib,"@@capture_image_width",Qnil);
        rb_cv_set(mEwcLib,"@@capture_image_height",Qnil);
        rb_cv_set(mEwcLib,"@@capture_frame_rate",Qnil);

        return self;
    }
    
    return Qnil;
}


//
// EwcLib#open_camera(id,width=640,height=480,fps=15.0)
//
VALUE rb_ewc_open_camera(int argc,VALUE *argv,VALUE self)
    
{
    VALUE mEwcLib;
    VALUE camera_id,c_width,c_height,c_fps,width,height,fps;
    int id,w,h,need_init=1;
    double f;
    
    mEwcLib=rb_const_get(rb_cObject,rb_intern("EwcLib"));

    c_width=rb_cv_get(mEwcLib,"@@capture_image_width");
    c_height=rb_cv_get(mEwcLib,"@@capture_image_height");
    c_fps=rb_cv_get(mEwcLib,"@@capture_frame_rate");
    
    rb_scan_args(argc,argv,"13",
                 &camera_id,
                 &width,
                 &height,
                 &fps);

    if(TYPE(c_width)!=T_NIL && 
       TYPE(c_height)!=T_NIL &&
       TYPE(c_fps)!=T_NIL){

        width=c_width;
        height=c_height;
        fps=c_fps;
        need_init=0;
    }

    id=NUM2INT(camera_id);

    if(TYPE(width)==T_NIL)
        w=640;
    else
        w=NUM2INT(width);

    if(TYPE(height)==T_NIL)
        h=480;
    else
        h=NUM2INT(height);

    if(TYPE(fps)==T_NIL)
        f=15.0;
    else
        f=NUM2DBL(fps);
    
    if(need_init)
        need_init=EWCDLL_Open(w,h,f);

    if(!need_init && id>=0 && id<EWCDLL_GetCamera()){
        rb_iv_set(self,"@camera_id",camera_id);
        
        rb_cv_set(mEwcLib,"@@capture_image_width",width);
        rb_cv_set(mEwcLib,"@@capture_image_height",height);
        rb_cv_set(mEwcLib,"@@capture_frame_rate",fps);
    
        return self;
    }
    else
        return Qnil;
}

//
// EwcLib#close_camera
//
VALUE rb_ewc_close_camera(VALUE self)
{
    VALUE mEwcLib;
    int ret;

    ret=EWCDLL_Close();
    
    if(!ret){
        mEwcLib=rb_const_get(rb_cObject,rb_intern("EwcLib"));
        rb_cv_set(mEwcLib,"@@capture_image_width",Qnil);
        rb_cv_set(mEwcLib,"@@capture_image_height",Qnil);
        rb_cv_set(mEwcLib,"@@capture_frame_rate",Qnil);
        
        rb_iv_set(self,"@camera_id",Qnil);
    }
    return INT2FIX(ret);
}

//
// EwcLib#capture
//
VALUE rb_ewc_capture(VALUE self)
{
    int id,width,height;
    size_t buf_sz;
    struct NARRAY *n_na;
    VALUE buf;


    id=NUM2INT(rb_iv_get(self,"@camera_id"));
    buf_sz=EWCDLL_GetBufferSize(id);

    if(IsNArray(self)){
        GetNArray(self,n_na);
        if(n_na->total*na_sizeof[n_na->type]>=buf_sz){
            EWCDLL_GetImage(id,n_na->ptr);
            return self;
        }
    }

    buf=rb_str_new(NULL,buf_sz);
    EWCDLL_GetImage(id,RSTRING(buf)->ptr);

    return buf;
}

void Init_ewclib()
{
    VALUE mEwcLib;

    mEwcLib=rb_define_module("EwcLib");
    rb_cv_set(mEwcLib,"@@capture_image_width",Qnil);
    rb_cv_set(mEwcLib,"@@capture_image_height",Qnil);
    rb_cv_set(mEwcLib,"@@capture_frame_rate",Qnil);

    rb_define_module_function(mEwcLib,"open",rb_ewc_open,3);
    rb_define_module_function(mEwcLib,"close",rb_ewc_close,0);

    rb_define_method(mEwcLib,"open_camera",rb_ewc_open_camera,-1);
    rb_define_method(mEwcLib,"close_camera",rb_ewc_close_camera,0);
    rb_define_method(mEwcLib,"capture",rb_ewc_capture,0);
}
