#include "Runtime.h"
#define KHAOS_ABI extern "C"
#include "KhaosAPI.h"
#include "imgui.h"
#include "backends/imgui_impl_dx9.h"
#include "backends/imgui_impl_win32.h"
#include <d3d9.h>
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND,UINT,WPARAM,LPARAM);
namespace SPII {
void CloseSettings();
namespace {
volatile LONG hosted=0,opened=0,uiReady=0,inputReset=1;
ImGuiContext* context=nullptr;HWND currentWindow=nullptr;IDirect3DDevice9* currentDevice=nullptr;bool backends=false;
DWORD renderThread=0;CRITICAL_SECTION renderLock;
struct Message{HWND window;UINT id;WPARAM w;LPARAM l;};Message messages[64];unsigned messageCount=0;SRWLOCK messageLock=SRWLOCK_INIT;
void Panel(){
 Settings s=GetSettings();bool changed=false,visible=true;
 ImGui::SetNextWindowSize(ImVec2(630,610),ImGuiCond_FirstUseEver);
 if(ImGui::Begin("Show Player In Inventory LE",&visible)){
  ImGui::TextUnformatted("Camera settings update live while Inventory or Magic is open.");
  changed|=ImGui::Checkbox("Enable",&s.enabled);ImGui::SameLine();changed|=ImGui::Checkbox("Magic menu",&s.magic);
  changed|=ImGui::Checkbox("Barter menu",&s.barter);ImGui::SameLine();changed|=ImGui::Checkbox("Container menu",&s.container);
  changed|=ImGui::Checkbox("Hide item previews",&s.hidePreviews);ImGui::SameLine();changed|=ImGui::Checkbox("Remove menu blur",&s.clearBlur);
  changed|=ImGui::Checkbox("Camera controls",&s.cameraControls);ImGui::SameLine();changed|=ImGui::Checkbox("Hold movement / looking",&s.lockControls);
  changed|=ImGui::Checkbox("Remember framing",&s.remember);ImGui::SameLine();changed|=ImGui::Checkbox("Automatic body framing",&s.autoFrame);
  changed|=ImGui::Checkbox("Face the camera",&s.turnToFront);ImGui::SameLine();changed|=ImGui::Checkbox("Reset orbit on opening",&s.resetOrbitOnOpen);
  changed|=ImGui::Checkbox("Animate player in paused menus",&s.animatePaused);
  changed|=ImGui::Checkbox("Neutral face in paused menus",&s.neutralFace);
  const ImGuiSliderFlags flags=ImGuiSliderFlags_NoInput;
  changed|=ImGui::SliderFloat("Horizontal position",&s.screenX,0.1f,0.9f,"%.2f",flags);
  changed|=ImGui::SliderFloat("Vertical position",&s.screenY,0.1f,0.9f,"%.2f",flags);
  changed|=ImGui::SliderFloat("Distance / zoom",&s.distance,70,600,"%.0f",flags);
  changed|=ImGui::SliderFloat("Body target height",&s.height,10,150,"%.0f",flags);
  changed|=ImGui::SliderFloat("Pitch",&s.pitch,-75,75,"%.0f",flags);
  changed|=ImGui::SliderFloat("Orbit",&s.yaw,-180,180,"%.0f",flags);
  changed|=ImGui::SliderFloat("Field of view",&s.fov,20,120,"%.0f",flags);
  if(ImGui::Button("Reset framing")){Settings d;s.screenX=d.screenX;s.screenY=d.screenY;s.distance=d.distance;s.height=d.height;s.pitch=d.pitch;s.yaw=d.yaw;s.fov=d.fov;changed=true;}
  ImGui::SameLine();if(ImGui::Button("Reload INI")){ReloadSettings();s=GetSettings();}
  ImGui::Separator();
  ImGui::TextWrapped("Outside the item list: right drag orbits; middle drag pans; Ctrl+middle adjusts pitch. Ctrl+wheel zooms; Ctrl+Shift+wheel changes FOV. Hold RB and use sticks/triggers on a controller. F10 reloads the INI.");
  ImGui::TextWrapped("LE backend: optional KHA-OS settings, no FLICK or Address Library. Physics follows the installed LE provider.");
  ImGui::TextUnformatted("Original: Ivy / ItzIvy05. LE adaptation: Mhacks79. GPLv3.");
 }
 ImGui::End();if(changed){PutSettings(s,false);RequestSettingsSave();}FlushSettingsSave(false);if(!visible)CloseSettings();
}
void ReleaseObjects(){if(backends)ImGui_ImplDX9_InvalidateDeviceObjects();}
void SafeReleaseObjects(){__try{ReleaseObjects();}__except(EXCEPTION_EXECUTE_HANDLER){}}
void Body(HWND window,IDirect3DDevice9* device,int width,int height){
 if(!window||!device||width<=0||height<=0)return;
 DWORD thread=GetCurrentThreadId();if(renderThread&&renderThread!=thread){Log("Settings render thread changed; frame declined");return;}renderThread=thread;
 if(!context){context=ImGui::CreateContext();ImGui::SetCurrentContext(context);auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.LogFilename=nullptr;ImGui::StyleColorsDark();}
 ImGui::SetCurrentContext(context);
 if(InterlockedExchange(&inputReset,0)){auto& io=ImGui::GetIO();io.ClearEventsQueue();io.ClearInputKeys();io.ClearInputMouse();}
 if(backends&&(currentWindow!=window||currentDevice!=device)){ImGui_ImplDX9_Shutdown();ImGui_ImplWin32_Shutdown();backends=false;}
 if(!backends){if(!ImGui_ImplWin32_Init(window))return;if(!ImGui_ImplDX9_Init(device)){ImGui_ImplWin32_Shutdown();return;}currentWindow=window;currentDevice=device;backends=true;}
 Message batch[64];unsigned count=0;AcquireSRWLockExclusive(&messageLock);count=messageCount;memcpy(batch,messages,count*sizeof(Message));messageCount=0;ReleaseSRWLockExclusive(&messageLock);
 for(unsigned i=0;i<count;++i)ImGui_ImplWin32_WndProcHandler(batch[i].window,batch[i].id,batch[i].w,batch[i].l);
 ImGui_ImplDX9_NewFrame();ImGui_ImplWin32_NewFrame();auto& io=ImGui::GetIO();io.DisplaySize=ImVec2(static_cast<float>(width),static_cast<float>(height));io.MouseDrawCursor=true;
 ImGui::NewFrame();Panel();ImGui::Render();ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}
}
void InitializeSettingsUI(){InitializeCriticalSection(&renderLock);InterlockedExchange(&uiReady,1);}
bool SettingsOpen(){return InterlockedCompareExchange(&hosted,0,0)&&InterlockedCompareExchange(&opened,0,0);}
void DrawSettings(void* window,void* device,int width,int height){
 if(!InterlockedCompareExchange(&uiReady,0,0)||!SettingsOpen()||Disabled())return;
 EnterCriticalSection(&renderLock);
 ImGuiContext* previous=ImGui::GetCurrentContext();
 __try{__try{Body(static_cast<HWND>(window),static_cast<IDirect3DDevice9*>(device),width,height);}
 __except(Fault("settings render",GetExceptionInformation())){InterlockedExchange(&opened,0);}}
 __finally{SafeReleaseObjects();ImGui::SetCurrentContext(previous);LeaveCriticalSection(&renderLock);}
}
}
extern "C" bool KHAOS_Query(KHAOS_PluginInfo* info){
 if(!info||info->structSize<sizeof(*info)||info->apiVersion<KHAOS_API_VERSION_MIN)return false;
 info->apiVersion=KHAOS_API_VERSION;info->structSize=sizeof(*info);info->flags=0;info->pluginId="showplayerininventory.le";
 info->displayName="Show Player In Inventory LE";info->category="Camera";info->dllName="ShowPlayerInInventoryLE.dll";
 info->originalHotkey="KHA-OS settings / F10 reload INI";info->description="Live inventory camera framing, orbit, zoom and FOV.";return true;
}
namespace SPII {
void CloseSettings(){
 InterlockedExchange(&opened,0);InterlockedExchange(&inputReset,1);AcquireSRWLockExclusive(&messageLock);messageCount=0;ReleaseSRWLockExclusive(&messageLock);
 __try{FlushSettingsSave(true);}__except(Fault("settings save",GetExceptionInformation())){}
}
#ifdef SPII_TEST
unsigned TestQueuedMessages(){AcquireSRWLockShared(&messageLock);unsigned n=messageCount;ReleaseSRWLockShared(&messageLock);return n;}
#endif
}
extern "C" void KHAOS_SetHosted(bool value){InterlockedExchange(&SPII::hosted,value?1:0);if(!value)SPII::CloseSettings();}
extern "C" void KHAOS_ToggleMenu(){if(InterlockedCompareExchange(&SPII::hosted,0,0)){if(SPII::SettingsOpen())SPII::CloseSettings();else InterlockedExchange(&SPII::opened,1);}}
extern "C" bool KHAOS_IsMenuOpen(){return SPII::SettingsOpen();}
extern "C" void KHAOS_RenderFrame(void* window,void* device,int width,int height){SPII::DrawSettings(window,device,width,height);}
extern "C" bool KHAOS_ProcessWndProc(void* window,unsigned message,uintptr_t w,intptr_t l){
 if(!SPII::SettingsOpen())return false;
 if((message>=WM_MOUSEFIRST&&message<=WM_MOUSELAST)||(message>=WM_KEYFIRST&&message<=WM_KEYLAST)||message==WM_CHAR){
  AcquireSRWLockExclusive(&SPII::messageLock);if(SPII::messageCount<64)SPII::messages[SPII::messageCount++]={static_cast<HWND>(window),message,w,l};ReleaseSRWLockExclusive(&SPII::messageLock);return true;
 }return false;
}
