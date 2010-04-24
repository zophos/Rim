//////////////////////////////////////////////////////////////////////////
//
//
//
#ifndef __SHMEMUTEX_CORE_H__
#define __SHMEMUTEX_CORE_H__

#include <Windows.h>

class ShMemutex
{
public:
    ShMemutex(LPCSTR sharedName,
              DWORD maxSize,
              BOOL asServer,
              DWORD desireAccess=FILE_MAP_ALL_ACCESS);
    ~ShMemutex();
    
    //
    // accessors
    //
    HANDLE map_handle(){return this->map;}
    HANDLE mtx_handle(){return this->mtx;}
    DWORD size(){return this->mapSize;}
    LPCSTR name(){return this->shrName;}
    LPCSTR map_name(){return this->mapName;}
    LPCSTR mtx_name(){return this->mtxName;}
    void *map_addr(){return this->addr;}

    DWORD lock(DWORD timeout=INFINITE);
    BOOL  unlock();

    DWORD write(void *src,DWORD sz,DWORD timeout=INFINITE);
    DWORD read(void *dst,DWORD sz,DWORD timeout=INFINITE);
    void *read(DWORD timeout=INFINITE);

private:
    static const char *NAME_PREFIX;
    static const int NAME_PREFIX_SZ;
    static const char *NAME_MAP_SUFFIX;
    static const char *NAME_MTX_SUFFIX;
    static const int NAME_SUFFIX_SZ;

    HANDLE map;
    HANDLE mtx;
    DWORD mapSize;
    char shrName[MAX_PATH+1];
    char mapName[MAX_PATH+1];
    char mtxName[MAX_PATH+1];
    void *addr;
    LPSTR read_buf;

    void finalize();
};
#endif
