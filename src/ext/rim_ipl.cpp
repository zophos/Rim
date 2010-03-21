#include <ruby.h>
#include <cv.h>
#include <narray.h>

static IplImage *rim_cv_create_and_copy_image(int width,
                                              int height,
                                              int channels,
                                              int depth,
                                              struct NARRAY *n_na)
{
    IplImage *img=cvCreateImage(cvSize(width,height),
                                depth,
                                channels);

    size_t lineWidth=channels*width*na_sizeof[n_na->type];
    size_t sz=n_na->total*na_sizeof[n_na->type];

    if((img->widthStep==lineWidth)&&(img->imageSize==sz)){
        memcpy(img->imageData,n_na->ptr,sz);
    }
    else{
        char *src,*dst;
        int i;

        for(i=0,src=n_na->ptr,dst=img->imageData;
            i<img->height;
            i++,src+=lineWidth,dst+=img->widthStep)
            memcpy(dst,src,lineWidth);
    }

    return img;
}


int rb_rim_get_image_description(struct NARRAY *n_na,
                                 int *width,
                                 int *height,
                                 int *channels,
                                 int *depth)
{
    switch(n_na->rank){
    case 2:
        *channels=1;
        *width=n_na->shape[0];
        *height=n_na->shape[1];
        break;
    case 3:
        *channels=n_na->shape[0];
        *width=n_na->shape[1];
        *height=n_na->shape[2];
        break;
    default:
        return 0;
    }

    switch(n_na->type){
    case NA_BYTE:
        *depth=IPL_DEPTH_8U;
        break;
    case NA_SINT:
        *depth=IPL_DEPTH_16S;
        break;
    case NA_LINT:
        *depth=IPL_DEPTH_32S;
        break;
    case NA_SFLOAT:
        *depth=IPL_DEPTH_32F;
        break;
    case NA_DFLOAT:
        *depth=IPL_DEPTH_64F;
        break;
    default:
        return 0;
    }

    return 1;
}


VALUE rb_rim_ipl2image(IplImage *img)
{
    struct NARRAY *n_na;
    ID func;

    switch(img->depth){
    case IPL_DEPTH_8U:
    case IPL_DEPTH_8S:
        func=rb_intern("byte");
        break;
    case IPL_DEPTH_16U:
    case IPL_DEPTH_16S:
        func=rb_intern("sint");
        break;
    case IPL_DEPTH_32S:
        func=rb_intern("lint");
        break;
    case IPL_DEPTH_32F:
        func=rb_intern("sfloat");
        break;
    case IPL_DEPTH_64F:
        func=rb_intern("dfloat");
        break;
    default:
        return NULL;
    }

    VALUE oRimImage=
        rb_funcall(rb_const_get(rb_const_get(rb_cObject,rb_intern("Rim")),
                                rb_intern("Image")),
                   func,
                   3,
                   INT2FIX(img->nChannels),
                   INT2FIX(img->width),
                   INT2FIX(img->height));
    
    GetNArray(oRimImage,n_na);

    size_t lineWidth=img->nChannels*img->width*na_sizeof[n_na->type];
    size_t sz=n_na->total*na_sizeof[n_na->type];
    if((img->widthStep==lineWidth)&&(img->imageSize==sz)){
        memcpy(n_na->ptr,img->imageData,sz);
    }
    else{
        char *src,*dst;
        int i;

        for(i=0,src=img->imageData,dst=n_na->ptr;
            i<img->height;
            i++,src+=img->widthStep,dst+=lineWidth)
            memcpy(dst,src,lineWidth);
    }

    return oRimImage;
}


IplImage *rb_rim_image2ipl(VALUE oRimImage)
{
    struct NARRAY *n_na;
    int width,height,channels,depth;

    if(!IsNArray(oRimImage))
        return NULL;

    GetNArray(oRimImage,n_na);


    if(!rb_rim_get_image_description(n_na,
                                     &width,
                                     &height,
                                     &channels,
                                     &depth))
        return NULL;

    return rim_cv_create_and_copy_image(width,
                                        height,
                                        channels,
                                        depth,
                                        n_na);
}


IplImage *rb_rim_image2ipl_ref(VALUE oRimImage)
{
    struct NARRAY *n_na;
    int width,height,channels,depth;

    if(!IsNArray(oRimImage))
        return NULL;

    GetNArray(oRimImage,n_na);

    if(!rb_rim_get_image_description(n_na,
                                     &width,
                                     &height,
                                     &channels,
                                     &depth))
        return NULL;

    IplImage *img=ALLOC(IplImage);
    cvInitImageHeader(img,
                      cvSize(width,height),
                      depth,
                      channels);
    
    size_t lineWidth=img->nChannels*img->width*na_sizeof[n_na->type];
    size_t sz=n_na->total*na_sizeof[n_na->type];
    if((img->widthStep==lineWidth)&&(img->imageSize==sz)){
        img->imageData=n_na->ptr;
        img->imageDataOrigin=NULL;
    }
    else{
        free(img);
        img=rim_cv_create_and_copy_image(width,
                                         height,
                                         channels,
                                         depth,
                                         n_na);
    }
    
    return img;
}


void rb_rim_ipl_free(IplImage *img)
{
    if(!img)
        return;

    if(img->imageDataOrigin)
        cvReleaseImage(&img);
    else
        free(img);
}
