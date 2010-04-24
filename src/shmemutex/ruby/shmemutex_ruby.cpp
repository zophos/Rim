#include <new>
#include <ruby.h>
#include <narray.h>
#include "../shmemutex_core.h"

void rb_ShMemutex_free(ShMemutex *ptr)
{
    if(ptr)
        ptr->~ShMemutex();

    ruby_xfree(ptr);
}


VALUE rb_ShMemutex_alloc(VALUE klass)
{
    void *ptr=ruby_xmalloc(sizeof(ShMemutex));
    return Data_Wrap_Struct(klass,0,rb_ShMemutex_free,ptr);
}


//
// ShMemutex::initialize(shName,maxSize,asServer=false)
//
VALUE rb_ShMemutex_initialize(int argc,VALUE *argv,VALUE self)
{
    ShMemutex *ptr;
    VALUE shName,maxSz,asServer;


    if(rb_scan_args(argc,argv,"21",&shName,&maxSz,&asServer)==2)
        asServer=Qnil;

    if(asServer==Qfalse)
        asServer=Qnil;

    Data_Get_Struct(self,ShMemutex,ptr);
    try{
        new(ptr) ShMemutex(StringValuePtr(shName),
                           NUM2LONG(maxSz),
                           asServer==Qnil?0:1);
    }
    catch(const char *e){
        rb_raise(rb_eStandardError,
                 "Could not open %s",
                 StringValuePtr(shName));
    }
    return Qnil;
}

//
// ShMemutex::close
//
VALUE rb_ShMemutex_close(VALUE self)
{
    ShMemutex *ptr;

    Data_Get_Struct(self,ShMemutex,ptr);

    ptr->~ShMemutex();

    return Qnil;
}

//
// ShMemutex::shared_name
//
VALUE rb_ShMemutex_shared_name(VALUE self)
{
    ShMemutex *ptr;

    Data_Get_Struct(self,ShMemutex,ptr);

    return rb_str_new2(ptr->name());
}

//
// ShMemutex::mapping_name
//
VALUE rb_ShMemutex_mapping_name(VALUE self)
{
    ShMemutex *ptr;

    Data_Get_Struct(self,ShMemutex,ptr);

    return rb_str_new2(ptr->map_name());
}

//
// ShMemutex::mutex_name
//
VALUE rb_ShMemutex_mutex_name(VALUE self)
{
    ShMemutex *ptr;

    Data_Get_Struct(self,ShMemutex,ptr);

    return rb_str_new2(ptr->mtx_name());
}

//
// ShMemutex::mapping_size
//
VALUE rb_ShMemutex_mapping_size(VALUE self)
{
    ShMemutex *ptr;

    Data_Get_Struct(self,ShMemutex,ptr);

    return INT2NUM(ptr->size());
}


//
//  ShMemutex::lock(timeout=-1)
//
VALUE rb_ShMemutex_lock(int argc,VALUE *argv,VALUE self)
{
    VALUE timeout;
    ShMemutex *ptr;
    long to=-1;

    if(rb_scan_args(argc,argv,"01",&timeout)==1)
        to=NUM2LONG(timeout);

    Data_Get_Struct(self,ShMemutex,ptr);

    ptr->lock(to);

    return self;
}


//
//  ShMemutex::unlock
//
VALUE rb_ShMemutex_unlock(VALUE self)
{
    ShMemutex *ptr;

    Data_Get_Struct(self,ShMemutex,ptr);

    ptr->unlock();

    return self;
}


//
// ShMemutex::write(src,timeout=-1)
//
VALUE rb_ShMemutex_write(int argc,VALUE *argv,VALUE self)
{
    VALUE src,timeout;
    struct NARRAY *n_na;
    ShMemutex *ptr;
    long to=-1;
    void *buf;
    size_t sz;

    if(rb_scan_args(argc,argv,"11",&src,&timeout)==2)
        to=NUM2LONG(timeout);

    Data_Get_Struct(self,ShMemutex,ptr);

    if(IsNArray(src)){
        GetNArray(src,n_na);

        sz=n_na->total*na_sizeof[n_na->type];
        buf=(void *)n_na->ptr;
    }
    else{
        StringValue(src);
        sz=RSTRING(src)->len+1;
        buf=RSTRING(src)->ptr;
    }

    if((!sz)||(!buf))
        return INT2FIX(0);
    
    return INT2NUM(ptr->write(buf,sz,to));
}

//
// ShMemutex::read(timeout=-1)
//
VALUE rb_ShMemutex_read(int argc,VALUE *argv,VALUE self)
{
    VALUE timeout,buf;
    ShMemutex *ptr;
    long to=-1,len;

    if(rb_scan_args(argc,argv,"01",&timeout)==1)
        to=NUM2LONG(timeout);

    Data_Get_Struct(self,ShMemutex,ptr);
    len=ptr->size();

    buf=rb_str_new(NULL,len);
    ptr->read(RSTRING(buf)->ptr,RSTRING(buf)->len,to);

    return buf;
}


//
// NArray#dump_to_shmemutex(shmemutex,timeout=-1)
//
VALUE rb_NArray_dump_to_shmemutex(int argc,VALUE *argv,VALUE self)
{
    VALUE shm,timeout;
    struct NARRAY *n_na;
    ShMemutex *ptr;
    long to=-1;
    size_t sz;

    if(rb_scan_args(argc,argv,"11",&shm,&timeout)==2)
        to=NUM2LONG(timeout);

    if(rb_obj_is_kind_of(shm,
                         rb_const_get(rb_cObject,
                                      rb_intern("ShMemutex")))!=Qtrue)
        rb_raise(rb_eTypeError,"1st. argument must be ShMemutex object.");
    Data_Get_Struct(shm,ShMemutex,ptr);
    

    GetNArray(self,n_na);
    sz=n_na->total*na_sizeof[n_na->type];
    if(!sz)
        rb_raise(rb_eTypeError,"NArray size is too small");

    ptr->write(n_na->ptr,sz,to);

    return self;
}

//
// NArray#load_from_shmemutex(shmemutex,timeout=-1)
//
VALUE rb_NArray_load_from_shmemutex(int argc,VALUE *argv,VALUE self)
{
    VALUE shm,timeout;
    struct NARRAY *n_na;
    ShMemutex *ptr;
    long to=-1;
    size_t sz;

    if(rb_scan_args(argc,argv,"11",&shm,&timeout)==2)
        to=NUM2LONG(timeout);

    if(rb_obj_is_kind_of(shm,
                         rb_const_get(rb_cObject,
                                      rb_intern("ShMemutex")))!=Qtrue)
        rb_raise(rb_eTypeError,"1st. argument must be ShMemutex object.");
    Data_Get_Struct(shm,ShMemutex,ptr);
    

    GetNArray(self,n_na);
    sz=n_na->total*na_sizeof[n_na->type];
    if(!sz)
        rb_raise(rb_eTypeError,"NArray size is too small");

    ptr->read(n_na->ptr,sz,to);

    return self;
}

extern "C" void Init_shmemutex()
{
    VALUE cShMemutex;
    VALUE cNArray;


    cShMemutex=rb_define_class("ShMemutex",rb_cObject);
    rb_define_alloc_func(cShMemutex,rb_ShMemutex_alloc);


    rb_define_const(cShMemutex,"WAIT_INFINITE",INT2FIX(-1));
    rb_define_const(cShMemutex,"WAIT_NOWAIT",INT2FIX(0));


    rb_define_private_method(cShMemutex,
                             "initialize",
                             (VALUE(*)(...))rb_ShMemutex_initialize,
                             -1);

                             
    rb_define_method(cShMemutex,
                     "close",
                     (VALUE(*)(...))rb_ShMemutex_close,
                     0);

    rb_define_method(cShMemutex,
                     "shared_name",
                     (VALUE(*)(...))rb_ShMemutex_shared_name,
                     0);

    rb_define_method(cShMemutex,
                     "mapping_name",
                     (VALUE(*)(...))rb_ShMemutex_mapping_name,
                     0);

    rb_define_method(cShMemutex,
                     "mutex_name",
                     (VALUE(*)(...))rb_ShMemutex_mutex_name,
                     0);

    rb_define_method(cShMemutex,
                     "mapping_size",
                     (VALUE(*)(...))rb_ShMemutex_mapping_size,
                     0);

    rb_define_method(cShMemutex,
                     "lock",
                     (VALUE(*)(...))rb_ShMemutex_lock,
                     -1);

    rb_define_method(cShMemutex,
                     "unlock",
                     (VALUE(*)(...))rb_ShMemutex_unlock,
                     0);

    rb_define_method(cShMemutex,
                     "write",
                     (VALUE(*)(...))rb_ShMemutex_write,
                     -1);

    rb_define_method(cShMemutex,
                     "read",
                     (VALUE(*)(...))rb_ShMemutex_read,
                     -1);


    cNArray=rb_define_class("NArray",rb_cObject);

    rb_define_method(cNArray,
                     "dump_to_shmemutex",
                     (VALUE(*)(...))rb_NArray_dump_to_shmemutex,
                     -1);

    rb_define_method(cNArray,
                     "load_from_shmemutex",
                     (VALUE(*)(...))rb_NArray_load_from_shmemutex,
                     -1);

}

