//////////////////////////////////////////////////////////////////////////
//
//
//

#include "shmemutex_core.h"
#include <stdio.h>

const char *ShMemutex::NAME_PREFIX="shmemutex_";
const int ShMemutex::NAME_PREFIX_SZ=10;
const char *ShMemutex::NAME_MAP_SUFFIX="_map";
const char *ShMemutex::NAME_MTX_SUFFIX="_mtx";
const int ShMemutex::NAME_SUFFIX_SZ=4;


ShMemutex::ShMemutex(LPCSTR sharedName,
                     DWORD maxSize,
                     BOOL asServer,
                     DWORD desireAccess)
    : map(NULL),
      mtx(NULL),
      mapSize(0),
      addr(NULL),
      read_buf(NULL)
{
    size_t len=NAME_PREFIX_SZ+strlen(sharedName);

#ifdef _MSC_VER
    _snprintf_s(this->shrName,
                MAX_PATH,
                len,
                "%s%s",
                NAME_PREFIX,
                sharedName);

    len+=NAME_SUFFIX_SZ;
    _snprintf_s(this->mapName,
                MAX_PATH,
                len,
                "%s%s",
                this->shrName,
                NAME_MAP_SUFFIX);
    _snprintf_s(this->mtxName,
                MAX_PATH,
                len,
                "%s%s",
                this->shrName,
                NAME_MTX_SUFFIX);
#else
    snprintf(this->shrName,
             MAX_PATH+1,
             "%s%s",
             NAME_PREFIX,
             sharedName);

    len+=NAME_SUFFIX_SZ;
    snprintf(this->mapName,
             MAX_PATH+1,
             "%s%s",
             this->shrName,
             NAME_MAP_SUFFIX);
    snprintf(this->mtxName,
             MAX_PATH+1,
             "%s%s",
             this->shrName,
             NAME_MTX_SUFFIX);
#endif

    this->mapSize=maxSize;


    this->mtx=CreateMutexA(NULL,FALSE,this->mtxName);
    if((!this->mtx)||
       (asServer && GetLastError()==ERROR_ALREADY_EXISTS)){
        this->finalize();
        throw "Given Name was already used for CreateFileMutexA()";
    }
    
    this->map=CreateFileMappingA(INVALID_HANDLE_VALUE,
                          NULL,
                          PAGE_READWRITE,
                          0,
                          this->mapSize,
                          this->mapName);
    if((!this->map)||
       (asServer && GetLastError()==ERROR_ALREADY_EXISTS)){
        this->finalize();
        throw "Given Name was already used for CreateFileMappingA()";
    }

    this->addr=(void *)MapViewOfFile(this->map,
                                     desireAccess,
                                     0,0,0);
    if(!this->addr){
        this->finalize();
        throw "Could not Mapping at MapViewOfFile()";
    }

}


ShMemutex::~ShMemutex()
{
    this->finalize();
}


void ShMemutex::finalize()
{
    if(this->addr){
        UnmapViewOfFile(this->addr);
        this->addr=NULL;
    }
    if(this->map){
        CloseHandle(this->map);
        this->map=NULL;
    }
    if(this->mtx){
        CloseHandle(this->mtx);
        this->mtx=NULL;
    }
    if(this->read_buf){
        delete [] (this->read_buf);
        this->read_buf=NULL;
    }
}


DWORD ShMemutex::lock(DWORD timeout)
{
    return WaitForSingleObject(this->mtx,timeout);
}
BOOL  ShMemutex::unlock()
{
    return ReleaseMutex(this->mtx);
}


DWORD ShMemutex::write(void *src,DWORD sz,DWORD timeout)
{
    if(WaitForSingleObject(this->mtx,timeout)==WAIT_OBJECT_0){
        if(sz>this->mapSize)
            sz=this->mapSize;
        memcpy(this->addr,src,sz);
        ReleaseMutex(this->mtx);
        return sz;
    }
    else
        return 0;
}


DWORD ShMemutex::read(void *dst,DWORD sz,DWORD timeout)
{
    if(WaitForSingleObject(this->mtx,timeout)==WAIT_OBJECT_0){
        if(sz>this->mapSize)
            sz=this->mapSize;
        memcpy(dst,this->addr,sz);
        ReleaseMutex(this->mtx);
        return sz;
    }
    else
        return 0;
}

void *ShMemutex::read(DWORD timeout)
{
    if(WaitForSingleObject(this->mtx,timeout)==WAIT_OBJECT_0){
        if(!read_buf)
            this->read_buf=new char[this->mapSize];
        if(!this->read_buf)
            throw "Could not allocate memory for the read buffer";

        memcpy(this->read_buf,this->addr,this->mapSize);
        ReleaseMutex(this->mtx);
        return (void *)(this->read_buf);
    }
    else
        return NULL;
}
