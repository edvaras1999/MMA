#include "Settings.h"
#include <cmath>
#include <cstring>
#include <cstdio>
namespace SPII {
namespace { SRWLOCK lock=SRWLOCK_INIT; Settings settings; volatile LONG revision=0,loggingEnabled=1; char path[MAX_PATH]={}; volatile LONG pendingSave=0,lastEdit=0;
#ifdef SPII_TEST
unsigned settingsWrites=0;
#endif
}
float Clamp(float v,float lo,float hi) { if(!std::isfinite(v))return lo;return v<lo?lo:(v>hi?hi:v); }
float ParseDecimal(const char* text,float fallback) {
 if(!text)return fallback;
 while(*text==' '||*text=='\t')++text;
 bool neg=false;if(*text=='-'||*text=='+'){neg=*text=='-';++text;}
 double value=0,scale=1;bool dot=false,seen=false;
 for(;*text;++text){if(*text>='0'&&*text<='9'){seen=true;if(dot){scale*=0.1;value+=(*text-'0')*scale;}else value=value*10+(*text-'0');if(value>1e9)return fallback;}
 else if((*text=='.'||*text==',')&&!dot)dot=true;
 else if(*text==' '||*text=='\t'){while(*text==' '||*text=='\t')++text;if(*text)return fallback;break;}
 else return fallback;}
 return seen?static_cast<float>(neg?-value:value):fallback;
}
void Normalize(Settings& s) {
 s.screenX=Clamp(s.screenX,0.1f,0.9f);s.screenY=Clamp(s.screenY,0.1f,0.9f);
 s.distance=Clamp(s.distance,70,1000);s.height=Clamp(s.height,10,150);
 s.pitch=Clamp(s.pitch,-75,75);s.yaw=Clamp(s.yaw,-36000,36000);
 s.fov=Clamp(s.fov,20,120);s.sensitivity=Clamp(s.sensitivity,0.001f,0.05f);
 if(s.rotateButton!=1&&s.rotateButton!=2&&s.rotateButton!=4)s.rotateButton=2;
 if(!s.controllerButton||s.controllerButton>0x8000||(s.controllerButton&(s.controllerButton-1)))s.controllerButton=0x0200;
}
Settings GetSettings(){Settings copy;AcquireSRWLockShared(&lock);__try{copy=settings;}__finally{ReleaseSRWLockShared(&lock);}return copy;}
bool LoggingEnabled(){return InterlockedCompareExchange(&loggingEnabled,0,0)!=0;}
const char* IniPath(){return path;}
unsigned SettingsRevision(){return static_cast<unsigned>(InterlockedCompareExchange(&revision,0,0));}
namespace {
void Number(const char* key,float v){
 char out[48];const bool neg=v<0;unsigned scaled=static_cast<unsigned>(std::fabs(v)*10000.0f+0.5f);
 _snprintf_s(out,sizeof(out),_TRUNCATE,"%s%u.%04u",neg?"-":"",scaled/10000,scaled%10000);
 WritePrivateProfileStringA("Camera",key,out,path);
}
void Boolean(const char* key,bool v){WritePrivateProfileStringA("General",key,v?"1":"0",path);}
void Save(const Settings& s){if(!*path)return;
#ifdef SPII_TEST
++settingsWrites;
#endif
 Boolean("bEnable",s.enabled);Boolean("bMagicMenu",s.magic);Boolean("bBarterMenu",s.barter);Boolean("bContainerMenu",s.container);
 Boolean("bHideItemPreviews",s.hidePreviews);Boolean("bCameraControls",s.cameraControls);Boolean("bLockControls",s.lockControls);
 Boolean("bRememberCamera",s.remember);Boolean("bClearBlur",s.clearBlur);Boolean("bAutoFrame",s.autoFrame);Boolean("bFaceCamera",s.turnToFront);Boolean("bResetOrbitOnOpen",s.resetOrbitOnOpen);Boolean("bEnableLogging",s.logging);Boolean("bAnimateWhilePaused",s.animatePaused);Boolean("bNeutralFace",s.neutralFace);
 WritePrivateProfileStringA("General","iSettingsVersion","4",path);
 Number("fScreenX",s.screenX);Number("fScreenY",s.screenY);Number("fDistance",s.distance);Number("fHeight",s.height);
 Number("fPitch",s.pitch);Number("fYaw",s.yaw);Number("fFOV",s.fov);Number("fSensitivity",s.sensitivity);
 char out[24];_snprintf_s(out,sizeof(out),_TRUNCATE,"%u",s.rotateButton);WritePrivateProfileStringA("Controls","iRotateButton",out,path);
 _snprintf_s(out,sizeof(out),_TRUNCATE,"%u",s.controllerButton);WritePrivateProfileStringA("Controls","iControllerButton",out,path);
}
}
void RequestSettingsSave(){InterlockedExchange(&lastEdit,static_cast<LONG>(GetTickCount()));InterlockedExchange(&pendingSave,1);}
void FlushSettingsSave(bool force){
 if(!InterlockedCompareExchange(&pendingSave,0,0))return;
 const DWORD last=static_cast<DWORD>(InterlockedCompareExchange(&lastEdit,0,0));
 if(!force&&GetTickCount()-last<400)return;
 AcquireSRWLockExclusive(&lock);__try{if(InterlockedExchange(&pendingSave,0))Save(settings);}__finally{ReleaseSRWLockExclusive(&lock);}
}
#ifdef SPII_TEST
unsigned TestSettingsWrites(){return settingsWrites;}
void TestSaveDelayElapsed(){InterlockedExchange(&lastEdit,static_cast<LONG>(GetTickCount()-401));}
#endif
void PutSettings(Settings value,bool persist){Normalize(value);AcquireSRWLockExclusive(&lock);__try{settings=value;InterlockedExchange(&loggingEnabled,value.logging?1:0);InterlockedIncrement(&revision);if(persist){Save(value);InterlockedExchange(&pendingSave,0);}}__finally{ReleaseSRWLockExclusive(&lock);}}
void InitializeSettings(const char* explicitPath){
 if(explicitPath){strncpy_s(path,explicitPath,_TRUNCATE);}else{
 GetModuleFileNameA(nullptr,path,MAX_PATH);path[MAX_PATH-1]=0;char* slash=strrchr(path,'\\');if(slash)slash[1]=0;
 const char* suffix="Data\\SKSE\\Plugins\\ShowPlayerInInventoryLE.ini";if(strlen(path)+strlen(suffix)<sizeof(path))strcat_s(path,suffix);else path[0]=0;}
 ReloadSettings();
}
void ReloadSettings(){Settings s;char value[64];
 #define BOOL(key,field) s.field=GetPrivateProfileIntA("General",key,s.field?1:0,path)!=0
 BOOL("bEnable",enabled);BOOL("bMagicMenu",magic);BOOL("bBarterMenu",barter);BOOL("bContainerMenu",container);
 BOOL("bHideItemPreviews",hidePreviews);const unsigned settingsVersion=GetPrivateProfileIntA("General","iSettingsVersion",0,path);if(settingsVersion<3)s.hidePreviews=true;BOOL("bCameraControls",cameraControls);BOOL("bLockControls",lockControls);
 BOOL("bRememberCamera",remember);BOOL("bClearBlur",clearBlur);BOOL("bAutoFrame",autoFrame);BOOL("bFaceCamera",turnToFront);BOOL("bResetOrbitOnOpen",resetOrbitOnOpen);BOOL("bEnableLogging",logging);BOOL("bAnimateWhilePaused",animatePaused);BOOL("bNeutralFace",neutralFace);
 #undef BOOL
 #define NUM(key,field) GetPrivateProfileStringA("Camera",key,"",value,sizeof(value),path);s.field=ParseDecimal(value,s.field)
 NUM("fScreenX",screenX);NUM("fScreenY",screenY);NUM("fDistance",distance);NUM("fHeight",height);NUM("fPitch",pitch);NUM("fYaw",yaw);NUM("fFOV",fov);NUM("fSensitivity",sensitivity);
 #undef NUM
 s.rotateButton=GetPrivateProfileIntA("Controls","iRotateButton",s.rotateButton,path);s.controllerButton=GetPrivateProfileIntA("Controls","iControllerButton",s.controllerButton,path);
 PutSettings(s,false);
}
}
