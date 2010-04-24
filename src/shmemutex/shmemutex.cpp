//////////////////////////////////////////////////////////////////////////
//
//
//
#include "shmemutex_core.h"
#include <iostream>

#ifdef _USRDLL
#define CALLCONV __stdcall
#else
#define CALLCONV __cdecl
#endif

extern "C" {

#ifdef _USRDLL
BOOL WINAPI DllMain(HINSTANCE hDLL,DWORD dwReason,LPVOID lpReserved)
{
    return TRUE;
}
#endif

void * CALLCONV shmemutex_open(LPCSTR sharedName,
                               DWORD maxSize,
                               BOOL asServer)
{
    try{
        ShMemutex *self=new ShMemutex(sharedName,
                                      maxSize,
                                      asServer);
        return (void *)self;
    }
    catch(const char *e){
        return NULL;
    }
    catch(...){
        return NULL;
    }
}

void CALLCONV shmemutex_close(void *handle)
{
    ShMemutex *self=(ShMemutex *)handle;
    delete self;
}


HANDLE CALLCONV shmemutex_map_handle(void *handle)
{
    ShMemutex *self=(ShMemutex *)handle;
    return self->map_handle();
}

HANDLE CALLCONV shmemutex_mtx_handle(void *handle)
{
    ShMemutex *self=(ShMemutex *)handle;
    return self->mtx_handle();
}

DWORD CALLCONV shmemutex_map_size(void *handle)
{
    ShMemutex *self=(ShMemutex *)handle;
    return self->size();
}

LPCSTR CALLCONV shmemutex_name(void *handle)
{
    ShMemutex *self=(ShMemutex *)handle;
    return self->name();
}

LPCSTR CALLCONV shmemutex_map_name(void *handle)
{
    ShMemutex *self=(ShMemutex *)handle;
    return self->map_name();
}

LPCSTR CALLCONV shmemutex_mtx_name(void *handle)
{
    ShMemutex *self=(ShMemutex *)handle;
    return self->mtx_name();
}
void * CALLCONV shmemutex_map_addr(void *handle)
{
    ShMemutex *self=(ShMemutex *)handle;
    return self->map_addr();
}


DWORD CALLCONV shmemutex_lock(void *handle,DWORD timeout)
{
    ShMemutex *self=(ShMemutex *)handle;
    return self->lock(timeout);
}

BOOL CALLCONV shmemutex_unlock(void *handle)
{
    ShMemutex *self=(ShMemutex *)handle;
    return self->unlock();
}
    
DWORD CALLCONV shmemutex_write(void *handle,
                               void *src,
                               DWORD sz,
                               DWORD timeout)
{
    ShMemutex *self=(ShMemutex *)handle;
    return self->write(src,sz,timeout);
}

void * CALLCONV shmemutex_read(void *handle,
                               void *dst,
                               DWORD sz,
                               DWORD timeout)
{
    ShMemutex *self=(ShMemutex *)handle;
    try{
        if(dst){
            if(self->read(dst,sz,timeout))
                return dst;
            else
                return NULL;
        }
        else
            return self->read(timeout);
    }
    catch(const char *e){
        return NULL;
    }
    catch(...){
        return NULL;
    }
}
    
}
