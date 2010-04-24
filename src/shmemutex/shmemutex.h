//////////////////////////////////////////////////////////////////////////
//
//
//
#ifndef __SHMEMUTEX_H__
#define __SHMEMUTEX_H__

#ifdef __SHMEMUTEX_H_STATIC__
#define EXTERN extern
#define CALLCONV __cdecl
#else
#define EXTERN __declspec(dllimport)
#define CALLCONV __stdcall
#endif


typedef void * ShMemutexHandle;

#ifdef __cplusplus
extern "C" {
#endif

EXTERN ShMemutexHandle CALLCONV shmemutex_open(char *sharedName,
                                               long maxSize,
                                               int asServer);

EXTERN void CALLCONV shmemutex_close(ShMemutexHandle handle);


EXTERN void * CALLCONV shmemutex_map_handle(ShMemutexHandle handle);

EXTERN void * CALLCONV shmemutex_mtx_handle(ShMemutexHandle handle);

EXTERN long CALLCONV shmemutex_map_size(ShMemutexHandle handle);

EXTERN char * CALLCONV shmemutex_name(ShMemutexHandle handle);

EXTERN char * CALLCONV shmemutex_map_name(ShMemutexHandle handle);

EXTERN char * CALLCONV shmemutex_mtx_name(ShMemutexHandle handle);

EXTERN void * CALLCONV shmemutex_map_addr(ShMemutexHandle handle);



EXTERN long CALLCONV shmemutex_lock(ShMemutexHandle handle,
                                    long timeout);

EXTERN int CALLCONV shmemutex_unlock(ShMemutexHandle handle);



EXTERN long CALLCONV shmemutex_write(ShMemutexHandle handle,
                                     void * src,
                                     long sz,
                                     long timeout);

EXTERN void * CALLCONV shmemutex_read(ShMemutexHandle handle,
                                      void * dst,
                                      long sz,
                                      long timeout);
    
#ifdef __cplusplus
}
#endif

#endif
