////////////////////////////////////////////////////////////////////////
//
// ewcdll.cpp  -- wrapper DLL of ewclib
//
#define EWC_TYPE MEDIASUBTYPE_RGB24
#include "ewclib.h"

BOOL WINAPI DllMain(HINSTANCE hDLL,DWORD dwReason,LPVOID lpReserved)
{
    return TRUE;
}

extern "C"{
int __stdcall EWCDLL_Open(int wx, int wy, double fps)
{
    return EWC_Open(wx,wy,fps);
}

int __stdcall EWCDLL_Close(void)
{
    return EWC_Close();
}

int __stdcall EWCDLL_GetCamera(void)
{
    return EWC_GetCamera();
}

int __stdcall EWCDLL_GetBufferSize(int num)
{
    return EWC_GetBufferSize(num);
}

int __stdcall EWCDLL_GetImage(int num, void *buffer)
{
    return EWC_GetImage(num,buffer);
}

int __stdcall EWCDLL_IsCaptured(int num, double *t)
{
    if(t)
        return EWC_IsCaptured(num,t);
    else
        return EWC_IsCaptured(num);
}

double __stdcall EWCDLL_time(int num)
{
    return ewc_time[num];
}

int __stdcall EWCDLL_SetBuffer(int num, void *buffer)
{
    return EWC_SetBuffer(num,buffer);
}

int __stdcall EWCDLL_GetBuffer(int num, void **buffer)
{
    return EWC_GetBuffer(num,buffer);
}

double __stdcall EWCDLL_GetValue(int num, int prop, int *mode)
{
    if(mode)
        return EWC_GetValue(num,prop,mode);
    else
        return EWC_GetValue(num,prop);
}

int __stdcall EWCDLL_SetValue(int num, int prop, double value)
{
    return EWC_SetValue(num,prop,value);
}

int __stdcall EWCDLL_SetDefault(int num, int prop)
{
    return EWC_SetDefault(num,prop);
}

int __stdcall EWCDLL_SetAuto(int num, int prop)
{
    return EWC_SetAuto(num,prop);
}

void __stdcall EWCDLL_GetLastMessage(char *s, int size)
{
     EWC_GetLastMessage(s,size);
}

int __stdcall EWCDLL_PropertyPage(int num)
{
    return EWC_PropertyPage(num);
}

}
