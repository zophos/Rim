////////////////////////////////////////////////////////////////////////
//
// ewcdll.h  -- wrapper DLL of ewclib
//
#ifndef __EWCDLL_H__
#define __EWCDLL_H__

#ifdef __cplusplus
extern "C" {
#endif

__declspec(dllimport) int __stdcall EWCDLL_Open(int wx,
                                                int wy,
                                                double fps);
__declspec(dllimport) int __stdcall EWCDLL_Close(void);
__declspec(dllimport) int __stdcall EWCDLL_GetCamera(void);
__declspec(dllimport) int __stdcall EWCDLL_GetBufferSize(int num);
__declspec(dllimport) int __stdcall EWCDLL_GetImage(int num,
                                                    void *buffer);
__declspec(dllimport) int __stdcall EWCDLL_IsCaptured(int num,
                                                      double *t);
__declspec(dllimport) double __stdcall EWCDLL_time(int num);
__declspec(dllimport) int __stdcall EWCDLL_SetBuffer(int num,
                                                     void *buffer);
__declspec(dllimport) int __stdcall EWCDLL_GetBuffer(int num,
                                                     void **buffer);
__declspec(dllimport) double __stdcall EWCDLL_GetValue(int num,
                                                       int prop,
                                                       int *mode);
__declspec(dllimport) int __stdcall EWCDLL_SetValue(int num,
                                                    int prop,
                                                    double value);
__declspec(dllimport) int __stdcall EWCDLL_SetDefault(int num,
                                                      int prop);
__declspec(dllimport) int __stdcall EWCDLL_SetAuto(int num,
                                                   int prop);
__declspec(dllimport) void __stdcall EWCDLL_GetLastMessage(char *s,
                                                           int size);
__declspec(dllimport) int __stdcall EWCDLL_PropertyPage(int num);


#ifdef __cplusplus
}
#endif

#endif
