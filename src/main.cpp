#include "Runtime.h"
#include "skse/GameInput.h"
#include "skse/GameAPI.h"
#include "skse/skse_version.h"
#include <shlobj.h>
#include <cstdarg>
#include <new>
IDebugLog gLog;
const _LookupREFRByHandle LookupREFRByHandle=reinterpret_cast<_LookupREFRByHandle>(0x004A9180);
namespace SPII {
namespace {
HANDLE logFile=INVALID_HANDLE_VALUE;SRWLOCK logLock=SRWLOCK_INIT;
volatile LONG faults=0;PluginHandle pluginHandle=kPluginHandle_Invalid;
SKSEMessagingInterface* messaging=nullptr;DWORD loadThread=0;
}
void Log(const char* format,...){
 if(logFile==INVALID_HANDLE_VALUE||!LoggingEnabled())return;
 char text[2048];va_list args;va_start(args,format);int used=_vsnprintf_s(text,sizeof(text)-3,_TRUNCATE,format,args);va_end(args);
 if(used<0)used=static_cast<int>(strlen(text));text[used++]='\r';text[used++]='\n';
 AcquireSRWLockExclusive(&logLock);DWORD wrote=0;WriteFile(logFile,text,static_cast<DWORD>(used),&wrote,nullptr);ReleaseSRWLockExclusive(&logLock);
}
#ifdef SPII_TEST
unsigned TestFaultCount(){return static_cast<unsigned>(InterlockedCompareExchange(&faults,0,0));}
#endif
bool Disabled(){return InterlockedCompareExchange(&faults,0,0)>=3;}
int Fault(const char* where,EXCEPTION_POINTERS* exception){
 LONG count=InterlockedIncrement(&faults);auto* r=exception->ExceptionRecord;
 Log("FAULT %s code=%08lX at=%p thread=%lu count=%ld disabled=%u",where,r->ExceptionCode,r->ExceptionAddress,GetCurrentThreadId(),count,count>=3);return EXCEPTION_EXECUTE_HANDLER;
}
extern volatile LONG wheelDelta;
void InitializeSettingsUI();bool VerifySites();
void PrepareForMenu(bool opening,const char* name);
namespace {
class WheelSink final:public BSTEventSink<InputEvent>{
 EventResult ReceiveEvent(InputEvent** events,InputEventDispatcher*) override{
  __try{if(!WantsWheelInput())return kEvent_Continue;unsigned visited=0;if(events)for(InputEvent* event=*events;event&&visited++<256&&Readable(event,0x10);event=event->next){
   if(event->deviceType!=kDeviceType_Mouse||event->eventType!=InputEvent::kEventType_Button||!Readable(event,0x20))continue;
   auto* bytes=reinterpret_cast<char*>(event);unsigned key=*reinterpret_cast<unsigned*>(bytes+0x14);unsigned value=*reinterpret_cast<unsigned*>(bytes+0x18);
   if(value!=0&&(key==8||key==9))InterlockedExchangeAdd(&wheelDelta,key==8?1:-1);
  }}__except(Fault("input",GetExceptionInformation())){ResetRuntime(false);}
  return kEvent_Continue;
 }
};
class MenuSink final:public BSTEventSink<MenuOpenCloseEvent>{
 EventResult ReceiveEvent(MenuOpenCloseEvent* event,EventDispatcher<MenuOpenCloseEvent>*)override{
  __try{if(event&&event->menuName.data)PrepareForMenu(event->opening,event->menuName.data);}__except(Fault("menu event",GetExceptionInformation())){ResetRuntime(false);}
  return kEvent_Continue;
 }
};
alignas(WheelSink) unsigned char wheelStorage[sizeof(WheelSink)];
alignas(MenuSink) unsigned char menuStorage[sizeof(MenuSink)];
bool initialized=false;
void MessageBody(SKSEMessagingInterface::Message* message){
 if(!message)return;
 switch(message->type){
 case SKSEMessagingInterface::kMessage_DataLoaded:
  if(initialized||Disabled())return;
  if(!VerifySites()){InterlockedExchange(&faults,3);return;}
  InitializeRuntime();InitializeSettingsUI();
  if(!InstallHooks()){Log("Hook install failed; rollback completed");InterlockedExchange(&faults,3);return;}
  if(auto* menus=MenuManager::GetSingleton())menus->MenuOpenCloseEventDispatcher()->AddEventSink(new(menuStorage)MenuSink);
  if(auto* dispatcher=*g_inputEventDispatcher)dispatcher->AddEventSink(new(wheelStorage)WheelSink);
  initialized=true;Log("DataLoaded thread=%lu load thread=%lu; LE camera ready",GetCurrentThreadId(),loadThread);break;
 case SKSEMessagingInterface::kMessage_SaveGame:ResetRuntime(true);break;
 case SKSEMessagingInterface::kMessage_PreLoadGame:ResetRuntime(false,true);break;
 case SKSEMessagingInterface::kMessage_PostLoadGame:ResetRuntime(message->data!=nullptr);break;
 case SKSEMessagingInterface::kMessage_NewGame:ResetRuntime(true);break;
 }
}
void Message(SKSEMessagingInterface::Message* message){__try{MessageBody(message);}__except(Fault("SKSE message",GetExceptionInformation())){ResetRuntime(false);}}
bool LoadBody(const SKSEInterface* skse){
 loadThread=GetCurrentThreadId();InitializeSettings();
 #ifndef SPII_TEST
 char path[MAX_PATH];if(SUCCEEDED(SHGetFolderPathA(nullptr,CSIDL_MYDOCUMENTS,nullptr,SHGFP_TYPE_CURRENT,path))){
  strcat_s(path,"\\My Games\\Skyrim\\SKSE");SHCreateDirectoryExA(nullptr,path,nullptr);strcat_s(path,"\\ShowPlayerInInventoryLE.log");
  logFile=CreateFileA(path,GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
 }
 #endif
 pluginHandle=skse->GetPluginHandle();messaging=static_cast<SKSEMessagingInterface*>(skse->QueryInterface(kInterface_Messaging));
 if(!messaging||messaging->interfaceVersion<SKSEMessagingInterface::kInterfaceVersion)return false;
 bool result=messaging->RegisterListener(pluginHandle,"SKSE",Message);Log("Show Player In Inventory LE 2.4-test7 loaded; thread=%lu listener=%u",loadThread,result);return result;
}
}
}
extern "C" bool SKSEPlugin_Query(const SKSEInterface* skse,PluginInfo* info){
 if(!skse||!info)return false;info->infoVersion=PluginInfo::kInfoVersion;info->name="Show Player In Inventory LE";info->version=0x02040007;
 return !skse->isEditor&&skse->runtimeVersion==RUNTIME_VERSION_1_9_32_0&&skse->skseVersion>=PACKED_SKSE_VERSION;
}
extern "C" bool SKSEPlugin_Load(const SKSEInterface* skse){__try{return SPII::LoadBody(skse);}__except(SPII::Fault("load",GetExceptionInformation())){return false;}}
