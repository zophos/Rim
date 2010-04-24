// Easy Web Camera LIBrary "ewclib.h"  by I.N.
// OS:Windows XP/Vista
// Compiler:Visual C++ 2008 Professional

// 2005/03/26 ver.0.1
// 2005/03/28 ver.0.2 add retry routine
// 2005/03/29 ver.1.0
// 2005/04/01 ver.1.x add skip mode, max=8
// 2005/04/04 ver.1.1 remove skip mode, but check displayname
// 2005/04/05 ver.1.2 debug. use WideCharToMultiByte()
// 2005/04/xx ver.1.3 debug (pMoniker,pEnum,pDevEnum)
// 2006/12/26 ver.1.4 for Visual C++ 2005
// 2006/--/--         IAMVideoProcAmp,IAMCameraControlに挑戦
// 2007/--/--         コールバック関数に挑戦
// 2007/--/--         EWC_IsCaptured(),EWC_GetBuffer(),EWC_SetBuffer()追加．
//                    ewc_time[]．EWC_Open()仕様変更．
//                    EWC_GetValue(),EWC_SetValue(),EWC_SetDefault(),EWC_SetAuto()追加．
// 2007/1/12  ver.1.5 EWC_PropertyPage(), EWC_GetLastMessage()追加
// 2007/01/--         bugfix EWC_SetDefault()
// 2007/10/15 ver.1.5b IAMVideoProcAmp,IAMCameraControl未サポート時の対応
// 2008/01/29 ver.1.6 1.5bの正式公開
// 2008/02/19         #pragma warning(disable:4819)を外す
// 2009/04/23         #pragma warning(disable:4996)を外す
// 2009/04/30 ver.1.7 Visual C++ 2008対応版,OpenCV対応,EWC_Cnv32to24()/EWC_Cnv24to32()追加．
// 2009/05/13 ver.1.8 COM初期化方法の改良（OpenCV対策）

#pragma once

#include <dshow.h>
//
// http://social.msdn.microsoft.com/Forums/en-US/windowssdk/thread/ed097d2c-3d68-4f48-8448-277eaaf68252
//
#pragma include_alias( "dxtrans.h", "qedit.h" )
#define __IDxtCompositor_INTERFACE_DEFINED__
#define __IDxtAlphaSetter_INTERFACE_DEFINED__
#define __IDxtJpeg_INTERFACE_DEFINED__
#define __IDxtKey_INTERFACE_DEFINED__
#include "qedit.h"
#pragma comment(lib,"strmiids.lib")

#include <math.h>	//for floor()

#ifndef EWC_TYPE
	#ifndef _CV_H_
		#define EWC_TYPE MEDIASUBTYPE_RGB32
	#else
		#define EWC_TYPE MEDIASUBTYPE_RGB24
	#endif
#endif

#ifndef EWC_NCAMMAX
#define EWC_NCAMMAX 8
#endif
int ewc_init=0;
int ewc_ncam;
int ewc_wx[EWC_NCAMMAX];
int ewc_wy[EWC_NCAMMAX];
int *ewc_pbuf[EWC_NCAMMAX];
int *ewc_buffer[EWC_NCAMMAX];
volatile long ewc_bufsize[EWC_NCAMMAX];
volatile double ewc_time[EWC_NCAMMAX];
#define EWC_VPAMPMAX	10
#define EWC_CAMCTLMAX	7
#define EWC_ITEMMAX		(EWC_VPAMPMAX+EWC_CAMCTLMAX)
int ewc_valueflag[EWC_NCAMMAX][EWC_ITEMMAX];

IGraphBuilder *ewc_pGraph;
IBaseFilter *ewc_pF[EWC_NCAMMAX];
ISampleGrabber *ewc_pGrab[EWC_NCAMMAX];
ICaptureGraphBuilder2 *ewc_pBuilder[EWC_NCAMMAX];
IBaseFilter *ewc_pCap[EWC_NCAMMAX];
IAMVideoProcAmp *ewc_pVPAmp[EWC_NCAMMAX];
IAMCameraControl *ewc_pCamCtl[EWC_NCAMMAX];

//IAMVideoProcAmp
#define EWC_BRIGHTNESS				0
#define EWC_CONTRAST				1
#define EWC_HUE						2
#define EWC_SATURATION				3
#define EWC_SHARPNESS				4
#define EWC_GAMMA					5
#define EWC_COLORENABLE				6
#define EWC_WHITEBALANCE			7
#define EWC_BACKLIGHTCOMPENSATION	8
#define EWC_GAIN					9
//IAMCameraControl
#define EWC_PAN						10
#define EWC_TILT					11
#define EWC_ROLL					12
#define EWC_ZOOM					13
#define EWC_EXPOSURE				14
#define EWC_IRIS					15
#define EWC_FOCUS					16

HRESULT ewc_hr;
int ewc_cominitflag=0;	//COM初期化フラグ(1なら終了処理を行う)

//カメラ番号のチェック
int numCheck(int num)
{
	if(!ewc_init) return 1;
	if(num<0 || num>=ewc_ncam) return 2;
	return 0;
}

//カメラ台数を返す
int EWC_GetCamera(void)
{
	if(!ewc_init) return 0;
	return ewc_ncam;
}

//カメラ(番号:num)のフレームバッファサイズ(単位:バイト)を返す
int EWC_GetBufferSize(int num)
{
	if(numCheck(num)) return 0;
	return ewc_bufsize[num];
}

//フィルタのピンを取得する
IPin *ewc_GetPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir)
{
	IEnumPins *pEnum;
	IPin *pPin=0;

	ewc_hr= pFilter->EnumPins(&pEnum);
	if(ewc_hr!=S_OK) return NULL;

	while(pEnum->Next(1,&pPin,0)==S_OK){
		PIN_DIRECTION PinDirThis;
		pPin->QueryDirection(&PinDirThis);
		if(PinDir==PinDirThis) break;
		pPin->Release();
	}
	pEnum->Release();
	return pPin;
}

//カメラ(番号:num)の画像取得
int EWC_GetImage(int num, void *buffer)
{
	if(numCheck(num)) return 1;
	memcpy(buffer,ewc_pbuf[num],ewc_bufsize[num]);
	return 0;
}

//バッファアドレスを変更
int EWC_SetBuffer(int num, void *buffer)
{
	if(numCheck(num)) return 1;
	ewc_pbuf[num]=(int *)buffer;
	return 0;
}

//現在のバッファアドレスを取得
int EWC_GetBuffer(int num, void **buffer)
{
	if(numCheck(num)) return 1;
	*buffer=ewc_pbuf[num];
	return 0;
}

//終了処理
int EWC_Close(void)
{
	IMediaControl *ewc_pMediaControl;
	int i;

	if(!ewc_init) return 1;

	//IMediaControlインターフェイス取得
	ewc_hr= ewc_pGraph->QueryInterface(IID_IMediaControl,(void **)&ewc_pMediaControl);
	if(ewc_hr!=S_OK) return 2;

	int t0,t;
	t0= GetTickCount();
	do{
		ewc_hr= ewc_pMediaControl->Stop();
		t= GetTickCount();
		if((t-t0)>3000) break;
	}while(ewc_hr!=S_OK);

	ewc_pMediaControl->Release();

	//メモリ解放
	for(i=ewc_ncam-1;i>=0;i--){
		ewc_pCamCtl[i]->Release();
		ewc_pVPAmp[i]->Release();
		ewc_pGrab[i]->Release();
		ewc_pF[i]->Release();
		ewc_pCap[i]->Release();
		ewc_pBuilder[i]->Release();
		if(ewc_buffer[i]) delete[] ewc_buffer[i];
	}
	ewc_pGraph->Release();

	//COM終了
	if(ewc_cominitflag){
		CoUninitialize();
		ewc_cominitflag=0;
	}
	ewc_init=0;

	return 0;
}

//設定値を読み出す
double EWC_GetValue(int num, int prop, int *mode=NULL)
{
	if(numCheck(num)) return -1.0;
	if(prop<0 || prop>=EWC_ITEMMAX) return -1.0;
	if(!ewc_valueflag[num][prop]) return -1.0;

	long Min,Max,Step,Default,Flags,Val;   

	if(prop<EWC_VPAMPMAX){
		//IAMVideoProcAmp
		ewc_hr= ewc_pVPAmp[num]->GetRange(prop,&Min,&Max,&Step,&Default,&Flags);
		if(ewc_hr==S_OK){
			ewc_hr= ewc_pVPAmp[num]->Get(prop,&Val,&Flags);
			if(ewc_hr!=S_OK){
				return -1.0;
			}
			if(mode) if(Flags & VideoProcAmp_Flags_Auto) *mode=1; else *mode=0;
			double value=(Val-Min)*100.0/(double)(Max-Min);
			return value;
		}else{
			return -1.0;
		}
	}else{
		//IAMCameraControl
		prop-=EWC_VPAMPMAX;
		ewc_hr= ewc_pCamCtl[num]->GetRange(prop,&Min,&Max,&Step,&Default,&Flags);
		if(ewc_hr==S_OK){
			ewc_hr= ewc_pCamCtl[num]->Get(prop,&Val,&Flags);
			if(ewc_hr!=S_OK){
				return -1.0;
			}
			if(mode) if(Flags & CameraControl_Flags_Auto) *mode=1; else *mode=0;
			double value=(Val-Min)*100.0/(double)(Max-Min);
			return value;
		}else{
			return -1.0;
		}
	}
}

//制御を手動モードにし，設定値を変更する
int EWC_SetValue(int num, int prop, double value)
{
	if(numCheck(num)) return 1;
	if(prop<0 || prop>=EWC_ITEMMAX) return 2;
	if(!ewc_valueflag[num][prop]) return 3;

	long Min, Max, Step, Default, Flags, Val;   

	if(prop<EWC_VPAMPMAX){
		//IAMVideoProcAmp
		ewc_hr= ewc_pVPAmp[num]->GetRange(prop,&Min,&Max,&Step,&Default,&Flags);
		if(ewc_hr==S_OK){
			Val=(long) ((value/100.0*(Max-Min))+Min);
			Val=min(max(Val,Min),Max);
			ewc_hr= ewc_pVPAmp[num]->Set(prop,Val,VideoProcAmp_Flags_Manual);
			if(ewc_hr!=S_OK){
				return 5;
			}
		}else{
			return 4;
		}
	}else{
		//IAMCameraControl
		prop-=EWC_VPAMPMAX;
		ewc_hr= ewc_pCamCtl[num]->GetRange(prop,&Min,&Max,&Step,&Default,&Flags);
		if(ewc_hr==S_OK){
			Val=(long) ((value/100.0*(Max-Min))+Min);
			Val=min(max(Val,Min),Max);
			ewc_hr= ewc_pCamCtl[num]->Set(prop,Val,CameraControl_Flags_Manual);
			if(ewc_hr!=S_OK){
				return 5;
			}
		}else{
			return 4;
		}
	}
	return 0;
}

//設定を初期値に戻す 1.6
int EWC_SetDefault(int num, int prop)
{
	if(numCheck(num)) return 1;
	if(prop<0 || prop>=EWC_ITEMMAX) return 2;
	if(!ewc_valueflag[num][prop]) return 3;

	long Min, Max, Step, Default, Flags, Val;   

	if(prop<EWC_VPAMPMAX){
		//IAMVideoProcAmp
		ewc_hr= ewc_pVPAmp[num]->GetRange(prop,&Min,&Max,&Step,&Default,&Flags);
		if(ewc_hr==S_OK){
			ewc_hr= ewc_pVPAmp[num]->Get(prop,&Val,&Flags);
			ewc_hr= ewc_pVPAmp[num]->Set(prop,Default,Flags);
			if(ewc_hr!=S_OK){
				return 5;
			}
		}else{
			return 4;
		}
	}else{
		//IAMCameraControl
		prop-=EWC_VPAMPMAX;
		ewc_hr= ewc_pCamCtl[num]->GetRange(prop,&Min,&Max,&Step,&Default,&Flags);
		if(ewc_hr==S_OK){
			ewc_hr= ewc_pCamCtl[num]->Get(prop,&Val,&Flags);
			ewc_hr= ewc_pCamCtl[num]->Set(prop,Default,Flags);
			if(ewc_hr!=S_OK){
				return 5;
			}
		}else{
			return 4;
		}
	}
	return 0;
}

//制御を自動モードにする
int EWC_SetAuto(int num, int prop)
{
	if(numCheck(num)) return 1;
	if(prop<0 || prop>=EWC_ITEMMAX) return 2;
	if(!ewc_valueflag[num][prop]) return 3;

	long Min, Max, Step, Default, Flags, Val;   

	if(prop<EWC_VPAMPMAX){
		//IAMVideoProcAmp
		ewc_hr= ewc_pVPAmp[num]->GetRange(prop,&Min,&Max,&Step,&Default,&Flags);
		if(ewc_hr==S_OK){
			ewc_hr= ewc_pVPAmp[num]->Get(prop,&Val,&Flags);
			ewc_hr= ewc_pVPAmp[num]->Set(prop,Val,VideoProcAmp_Flags_Auto);
			if(ewc_hr!=S_OK){
				return 5;
			}
		}else{
			return 4;
		}
	}else{
		//IAMCameraControl
		prop-=EWC_VPAMPMAX;
		ewc_hr= ewc_pCamCtl[num]->GetRange(prop,&Min,&Max,&Step,&Default,&Flags);
		if(ewc_hr==S_OK){
			ewc_hr= ewc_pCamCtl[num]->Get(prop,&Val,&Flags);
			ewc_hr= ewc_pCamCtl[num]->Set(prop,Val,CameraControl_Flags_Auto);
			if(ewc_hr!=S_OK){
				return 5;
			}
		}else{
			return 4;
		}
	}
	return 0;
}

#pragma comment(lib,"Quartz.lib")

//最後のエラーメッセージを取得する
//s:文字列格納先  size:領域sのサイズ
void EWC_GetLastMessage(char *s, int size)
{
	wchar_t w[256];
	AMGetErrorText(ewc_hr,w,256);
	WideCharToMultiByte(CP_ACP,0,w,-1,s,size,NULL,NULL);
}

//プロパティページを表示させる 1.6
int EWC_PropertyPage(int num)
{
	if(numCheck(num)) return 1;

	ISpecifyPropertyPages *pProp;

	ewc_hr= ewc_pCap[num]->QueryInterface(IID_ISpecifyPropertyPages, (void **)&pProp);
	if(ewc_hr!=S_OK) return 2;

	FILTER_INFO FilterInfo;
	ewc_hr= ewc_pCap[num]->QueryFilterInfo(&FilterInfo); 
	if(ewc_hr!=S_OK) return 3;
	IUnknown *pFilterUnk;
	ewc_hr= ewc_pCap[num]->QueryInterface(IID_IUnknown,(void **)&pFilterUnk);
	if(ewc_hr!=S_OK) return 4;

	CAUUID caGUID;
	pProp->GetPages(&caGUID);
	pProp->Release();

	OleCreatePropertyFrame(
		NULL,				// Parent window
		0,0,				// Reserved
		FilterInfo.achName,	// Caption for the dialog box
		1,					// Number of objects (just the filter)
		&pFilterUnk,		// Array of object pointers. 
		caGUID.cElems,		// Number of property pages
		caGUID.pElems,		// Array of property page CLSIDs
		0,					// Locale identifier
		0, NULL				// Reserved
	);

	pFilterUnk->Release();
	FilterInfo.pGraph->Release(); 
	CoTaskMemFree(caGUID.pElems);
	return 0;
}

//コールバック関数の定義
class ewc_SampleGrabberCB :public ISampleGrabberCB
{
public:
	STDMETHODIMP_(ULONG) AddRef()
	{
		return 2;
	}
	STDMETHODIMP_(ULONG) Release()
	{
		return 1;
	}
	STDMETHODIMP QueryInterface(REFIID riid, void ** ppv)
	{
		if(riid==IID_ISampleGrabberCB || riid==IID_IUnknown){
			*ppv= (void *)static_cast<ISampleGrabberCB*>(this);
			return NOERROR;
		}
		return E_NOINTERFACE;
	}
	STDMETHODIMP SampleCB(double SampleTime, IMediaSample *pSample)
	{
		return S_OK;
	}
	//フレーム毎に呼ばれる関数
	STDMETHODIMP BufferCB(double dblSampleTime, BYTE *pBuffer, long lBufferSize)
	{
		ewc_bufsize[i]= lBufferSize;
		int wx= ewc_wx[i];
		int wy= ewc_wy[i];
		int byte= lBufferSize/wy;
		//画像の上下を逆にしてコピー
		for(int y=0; y<wy; y++){
			memcpy((unsigned char *)ewc_pbuf[i]+(wy-1-y)*byte, pBuffer+y*byte,byte);
		}
		ewc_time[i]=dblSampleTime;
		return S_OK;
	}
	//コンストラクタ	
	ewc_SampleGrabberCB(int num)
	{
		i=num;
		ewc_pbuf[i]=ewc_buffer[i];
		ewc_bufsize[i]=0;
		tm=ewc_time[i]=0.0;
	}
	//デストラクタ
	~ewc_SampleGrabberCB()
	{
	}
	void TimeSet(double *t)
	{
		*t=tm=ewc_time[i];
	}
	int IsCaptured(void)
	{
		if(tm!=ewc_time[i]) return 1;
		else return 0;
	}
private:
	int i;
	double tm;
};

ewc_SampleGrabberCB *ewc_pSampleGrabberCB[EWC_NCAMMAX];

int EWC_Open(int wx, int wy, double fps)
{
	IAMStreamConfig *ewc_pConfig;
	IMediaControl *ewc_pMediaControl;
	IMoniker *pMoniker=0;
	IEnumMoniker *pEnum=0;
	ICreateDevEnum *pDevEnum=0;
	int i,errcode;
	int retryflag,t0,t;

	if(ewc_init){errcode=1; goto fin;}

	retryflag=0;

cont:
	//各変数の初期化
	errcode=0;
	ewc_pGraph=0;
	ewc_pMediaControl=0;
	ewc_pConfig=0;
	for(i=0;i<EWC_NCAMMAX;i++){
		ewc_pGrab[i]=0;
		ewc_pF[i]=0;
		ewc_pCap[i]=0;
		ewc_pBuilder[i]=0;
		ewc_buffer[i]=0;
		ewc_pVPAmp[i]=0;
		ewc_pCamCtl[i]=0;
		ewc_pSampleGrabberCB[i]=0;
	}

	//COM初期化(v.1.8)
	ewc_hr= CoInitializeEx(NULL,COINIT_MULTITHREADED);
	if(ewc_hr==S_OK) ewc_cominitflag=1;	//成功したらフラグを立てる

	//フィルタグラフマネージャ作成
	ewc_hr= CoCreateInstance(CLSID_FilterGraph,0,CLSCTX_INPROC_SERVER,IID_IGraphBuilder,(void **)&ewc_pGraph);
	if(ewc_hr!=S_OK){errcode=3; goto fin;}

	//システムデバイス列挙子の作成
	ewc_hr= CoCreateInstance(CLSID_SystemDeviceEnum,0,CLSCTX_INPROC_SERVER,IID_ICreateDevEnum,(void **)&pDevEnum);
	if(ewc_hr!=S_OK){errcode=4; goto fin;}
	//列挙子の取得
	ewc_hr= pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,&pEnum,0);
	if(ewc_hr!=S_OK){
		//ESP_Printf("No driver\n");
		errcode=5; goto fin;
	}

	//モニカの取得
	ULONG cFetched;
	wchar_t SrcName[32];
	ewc_ncam=0;
	//char displayname[512];
	for(i=0;i<EWC_NCAMMAX;i++){
		if(pEnum->Next(1,&pMoniker,&cFetched)==S_OK){
			//DisplayNameの取得
			LPOLESTR strMonikerName=0;
			ewc_hr= pMoniker->GetDisplayName(NULL,NULL,&strMonikerName);
			if(ewc_hr!=S_OK){errcode=6; goto fin;}
			//WideCharToMultiByte(CP_ACP,0,strMonikerName,-1,displayname,sizeof(displayname),0,0);
			//ESP_Printf("displayname(%d):%s\n",i,displayname);
			
			//DisplayNameに'@device:pnp'があれば登録
			if(wcsstr(strMonikerName,L"@device:pnp")){
			//if(strstr(displayname,"@device:pnp")){
				//オブジェクト初期化
				pMoniker->BindToObject(0,0,IID_IBaseFilter,(void **)&ewc_pCap[ewc_ncam]);
				pMoniker->Release();
				pMoniker=0;

				//グラフにフィルタを追加
				swprintf_s(SrcName,32,L"Video Capture %d",ewc_ncam);
				ewc_hr= ewc_pGraph->AddFilter(ewc_pCap[ewc_ncam], SrcName);
				if(ewc_hr!=S_OK){errcode=7; goto fin;}
				ewc_ncam++;
			}
		}
	}
	pEnum->Release();
	pEnum=0;
	pDevEnum->Release();
	pDevEnum=0;

	//No camera
	if(!ewc_ncam){errcode=8; goto fin;}

	//ESP_Printf("camera=%d\n",ewc_ncam);

	//キャプチャビルダの作成
	for(i=0;i<ewc_ncam;i++){
		ewc_wx[i]=wx;
		ewc_wy[i]=wy;
		
		CoCreateInstance(CLSID_CaptureGraphBuilder2,0,CLSCTX_INPROC_SERVER,
			IID_ICaptureGraphBuilder2,(void **)&ewc_pBuilder[i]);
		ewc_hr= ewc_pBuilder[i]->SetFiltergraph(ewc_pGraph);
		if(ewc_hr!=S_OK){errcode=9; goto fin;}
		
		//IAMStreamConfigインタフェースの取得
		ewc_hr= ewc_pBuilder[i]->FindInterface(&PIN_CATEGORY_CAPTURE,&MEDIATYPE_Video,
			ewc_pCap[i],IID_IAMStreamConfig,(void**)&ewc_pConfig);
		if(ewc_hr!=S_OK){errcode=10; goto fin;}

		//画像サイズ，フレームレートの設定
		AM_MEDIA_TYPE *ewc_pmt[EWC_NCAMMAX];
		ewc_hr= ewc_pConfig->GetFormat(&ewc_pmt[i]);
		VIDEOINFOHEADER *vh = (VIDEOINFOHEADER*)ewc_pmt[i]->pbFormat;
		vh->bmiHeader.biWidth =ewc_wx[i];
		vh->bmiHeader.biHeight=ewc_wy[i]; 
		vh->AvgTimePerFrame= (REFERENCE_TIME)floor((10000000.0/fps+0.5));
		ewc_hr=ewc_pConfig->SetFormat(ewc_pmt[i]);
		if(ewc_hr!=S_OK){errcode=11; goto fin;}
		ewc_pConfig->Release();
		ewc_pConfig=0;

		//サンプルグラバの生成 ewc_pF[]
		CoCreateInstance(CLSID_SampleGrabber,0,CLSCTX_INPROC_SERVER,IID_IBaseFilter,(LPVOID *)&ewc_pF[i]);
		ewc_hr= ewc_pF[i]->QueryInterface(IID_ISampleGrabber,(void **)&ewc_pGrab[i]);
		if(ewc_hr!=S_OK){errcode=12; goto fin;}

		//メディアタイプの設定
		AM_MEDIA_TYPE ewc_mt[EWC_NCAMMAX];
		ZeroMemory(&ewc_mt[i],sizeof(AM_MEDIA_TYPE));
		ewc_mt[i].majortype=MEDIATYPE_Video;
		//Qcam Pro 4000では次の値が設定可能だった．
		//MEDIASUBTYPE_RGB4
		//MEDIASUBTYPE_RGB8
		//MEDIASUBTYPE_RGB565
		//MEDIASUBTYPE_RGB555
		//MEDIASUBTYPE_RGB24
		//MEDIASUBTYPE_RGB32
		//MEDIASUBTYPE_ARGB32
		ewc_mt[i].subtype=EWC_TYPE;
		ewc_mt[i].formattype=FORMAT_VideoInfo;
		ewc_hr= ewc_pGrab[i]->SetMediaType(&ewc_mt[i]);
		if(ewc_hr!=S_OK){errcode=13; goto fin;}
		//フィルタグラフへの追加
		wchar_t GrabName[32];
		swprintf_s(GrabName,32,L"Grabber %d",i);
		ewc_hr= ewc_pGraph->AddFilter(ewc_pF[i], GrabName);
		if(ewc_hr!=S_OK){errcode=14; goto fin;}

		//サンプルグラバの接続
		IPin *ewc_pSrcOut[EWC_NCAMMAX];
		IPin *ewc_pSGrabIn[EWC_NCAMMAX];
		// ピンの取得
		ewc_pSrcOut[i]=ewc_GetPin(ewc_pCap[i],PINDIR_OUTPUT);
		ewc_pSGrabIn[i]=ewc_GetPin(ewc_pF[i],PINDIR_INPUT);

		// ピンの接続
		ewc_hr= ewc_pGraph->Connect(ewc_pSrcOut[i], ewc_pSGrabIn[i]);
		if(ewc_hr!=S_OK){
			errcode=15;
			goto fin;
		}
		//ESP_Printf("Connected(%p,%p)\n",ewc_pSrcOut[i],ewc_pSGrabIn[i]);
		ewc_pSrcOut[i]->Release();
		ewc_pSrcOut[i]=0;
		ewc_pSGrabIn[i]->Release();
		ewc_pSGrabIn[i]=0;

		//グラバのモード設定
		ewc_hr= ewc_pGrab[i]->SetBufferSamples(FALSE);
		if(ewc_hr!=S_OK){errcode=16; goto fin;}
		ewc_hr= ewc_pGrab[i]->SetOneShot(FALSE);
		if(ewc_hr!=S_OK){errcode=17; goto fin;}

		//バッファの確保，コールバック関数の登録
		ewc_buffer[i]= (int *)new int[wx*wy];
		ewc_pSampleGrabberCB[i]= new ewc_SampleGrabberCB(i);
		ewc_hr= ewc_pGrab[i]->SetCallback(ewc_pSampleGrabberCB[i],1);
		if(ewc_hr!=S_OK){errcode=20; goto fin;}

		//IAMVideoProcAmp
		ewc_hr=ewc_pCap[i]->QueryInterface(IID_IAMVideoProcAmp,(void **)&ewc_pVPAmp[i]);   
		if(ewc_hr!=S_OK){
			//errcode=22;
			//IAMVideoProcAmpが取得できなければ，
			//サポートしてないとみなす．(ver.1.5b)
			for(int j=0;j<EWC_VPAMPMAX;j++){
				//not supported
				ewc_valueflag[i][j]=0;
			}
		}else{
			for(int j=0;j<EWC_VPAMPMAX;j++){
				long Min, Max, Step, Default, Flags;   
				ewc_hr= ewc_pVPAmp[i]->GetRange(j,&Min,&Max,&Step,&Default,&Flags);
				if(ewc_hr==S_OK){
					ewc_valueflag[i][j]=1;
				}else{
					//not supported
					ewc_valueflag[i][j]=0;
				}
			}
		}

		//IAMCameraControl
		ewc_hr=ewc_pCap[i]->QueryInterface(IID_IAMCameraControl,(void **)&ewc_pCamCtl[i]);
		if(ewc_hr!=S_OK){
			//errcode=23;
			//IAMCameraControlが取得できなければ，
			//サポートしてないとみなす．(ver.1.5b)
			for(int j=0;j<EWC_CAMCTLMAX;j++){
				//not supported
				ewc_valueflag[i][j+EWC_VPAMPMAX]=0;
			}
		}else{
			for(int j=0;j<EWC_CAMCTLMAX;j++){
				long Min, Max, Step, Default, Flags;   
				ewc_hr= ewc_pCamCtl[i]->GetRange(j,&Min,&Max,&Step,&Default,&Flags);
				if(ewc_hr==S_OK){
					ewc_valueflag[i][j+EWC_VPAMPMAX]=1;
				}else{
					//not supported
					ewc_valueflag[i][j+EWC_VPAMPMAX]=0;
				}
			}
		}
	}

	//キャプチャ開始
	ewc_hr= ewc_pGraph->QueryInterface(IID_IMediaControl,(void **)&ewc_pMediaControl);
	if(ewc_hr!=S_OK){errcode=18; goto fin;}
	ewc_hr= ewc_pMediaControl->Run();
	if(ewc_hr!=S_OK){errcode=19; retryflag++; goto fin;}
	ewc_pMediaControl->Release();
	ewc_pMediaControl=0;

	//１回以上サンプルされるまで待機
	t0=GetTickCount();
	do{
		t=GetTickCount();
		if((t-t0)>3000){errcode=21; retryflag++; goto fin;}
	}while(ewc_bufsize[0]==0);

fin:
	if(errcode){
		if(!ewc_init){

		if(pDevEnum) pDevEnum->Release();
		if(pEnum) pEnum->Release();
		if(pMoniker) pMoniker->Release();
		if(ewc_pMediaControl) ewc_pMediaControl->Release();
		if(ewc_pConfig) ewc_pConfig->Release();

		for(i=EWC_NCAMMAX-1;i>=0;i--){
			if(ewc_buffer[i]) delete[] ewc_buffer[i];
			if(ewc_pSampleGrabberCB[i]) ewc_pSampleGrabberCB[i]->Release();
			if(ewc_pCamCtl[i]) ewc_pCamCtl[i]->Release();
			if(ewc_pVPAmp[i]) ewc_pVPAmp[i]->Release();
			if(ewc_pGrab[i]) ewc_pGrab[i]->Release();
			if(ewc_pF[i]) ewc_pF[i]->Release();
			if(ewc_pCap[i]) ewc_pCap[i]->Release();
			if(ewc_pBuilder[i]) ewc_pBuilder[i]->Release();
		}
		if(ewc_pGraph) ewc_pGraph->Release();

		//COM終了
		if(ewc_cominitflag){
			CoUninitialize();
			ewc_cominitflag=0;
		}

		if(retryflag){
			//正常に接続されるまでリトライ（３回まで）
			if(retryflag<=2) goto cont;
		}
		}
	}else{
		ewc_init=1;
	}
	return errcode;
}

//新しい画像が到着したかどうか
//num:カメラ番号
//t:取得時刻(秒)
int EWC_IsCaptured(int num, double *t=NULL)
{
	if(numCheck(num)) return 0;

	if(ewc_pSampleGrabberCB[num]->IsCaptured()){
		double tt;
		if(!t) t=&tt;
		ewc_pSampleGrabberCB[num]->TimeSet(t);
		return 1;
	}
	return 0;
}

//画像変換(32ビット->24ビット)
void EWC_Cnv32to24(unsigned char *dst, unsigned int *src, int pxl)
{
	unsigned char R,G,B;
	unsigned int ui;

	for(int n=0; n<pxl; n++){
		ui=*src++;
		B=ui;
		G=(ui>>8);
		R=(ui>>16);
		*(dst+0) = B;
		*(dst+1) = G;
		*(dst+2) = R;
		dst+=3;
	}
}

//画像変換(24ビット->32ビット)
void EWC_Cnv24to32(unsigned int *dst, unsigned char *src, int pxl)
{
	unsigned char R,G,B;

	for(int n=0; n<pxl; n++){
		B = *(src+0);
		G = *(src+1);
		R = *(src+2);
		src+=3;
		*dst++ = (R<<16) | (G<<8) | B;
	}
}
