//
//
//
#include <sys/cygwin.h>
#include <highgui.h>
#include "../rim_ipl.h"

#ifndef MAX_PATH
#define MAX_PATH (260)
#endif

VALUE rb_rim_file_load(VALUE self,VALUE path)
{
    VALUE oRimImage;
    struct NARRAY *n_na;
    size_t sz;
    char win32path[MAX_PATH];

    cygwin_conv_path(CCP_POSIX_TO_WIN_A| CCP_RELATIVE,
                     StringValueCStr(path),
                     win32path,
                     MAX_PATH);
    
    IplImage *img=cvLoadImage(win32path,CV_LOAD_IMAGE_COLOR);
    if(!img)
        rb_raise(rb_eIOError,"cvLoadImage IO Error");
 
    oRimImage=rb_rim_ipl2image(img);
    cvReleaseImage(&img);

    return oRimImage;
}


VALUE rb_rim_file_save(VALUE self,VALUE path)
{
    struct NARRAY *n_na;
    int width,height,channels,depth;
    size_t sz;
    char win32path[MAX_PATH];

    IplImage *img=rb_rim_image2ipl(self);
    if(!img)
        rb_raise(rb_eTypeError,"Odd reciever was gaven.");

    if((img->nChannels!=1 && img->nChannels!=3)||
       img->depth!=IPL_DEPTH_8U){
        rb_rim_ipl_free(img);
        rb_raise(rb_eTypeError,"Odd reciever was gaven.");
    }


    cygwin_conv_path(CCP_POSIX_TO_WIN_A| CCP_RELATIVE,
                     StringValueCStr(path),
                     win32path,
                     MAX_PATH);
    if(!cvSaveImage(win32path,img)){
        rb_rim_ipl_free(img);
        rb_raise(rb_eIOError,"cvSaveImage IO Error");
    }
    
    rb_rim_ipl_free(img);

    return path;
}

extern "C" void Init_rim_file()
{
    VALUE mRim,cRimImage;

    mRim=rb_define_module("Rim");

    rb_define_module_function(mRim,
                              "load",
                              (VALUE(*)(...))rb_rim_file_load,
                              1);

    cRimImage=rb_define_class_under(mRim,
                                    "Image",
                                    rb_const_get(rb_cObject,
                                                 rb_intern("NVector")));
    rb_define_method(cRimImage,
                     "save",
                     (VALUE(*)(...))rb_rim_file_save,
                     1);

}
