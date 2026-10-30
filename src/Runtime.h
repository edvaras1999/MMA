// Native x86 LE backend; no SE address/layout may be used here. GPL-3.0.
#pragma once
#include "Settings.h"
#include "skse/PluginAPI.h"
#include "skse/GameMenus.h"
#include "skse/NiObjects.h"
#include <cstdint>
#ifdef SPII_TEST
#include <xinput.h>
#endif
namespace SPII {
void Log(const char* format,...);
int Fault(const char* where,EXCEPTION_POINTERS* exception);
bool Disabled();
bool WantsWheelInput();
bool Readable(const void* pointer,size_t length,bool object=true);
bool Executable(const void* pointer);
void InitializeRuntime();
bool InstallHooks();
void ResetRuntime(bool ready,bool mayTeardown=false);
void OnFrame(IMenu* menu,unsigned stamp,float interval=1.0f/60);
void OnMenuDestroyed(IMenu* menu,unsigned type);
void RememberPerspective(bool first);
void DrawSettings(void* hwnd,void* device,int width,int height);
bool SettingsOpen();
void SaveFraming();
struct Statistics {unsigned ticks=0,starts=0,stops=0,transforms=0,wrongThreads=0,cameraPasses=0,poseUpdates=0,cameraSkips=0,projectionRepairs=0;};
Statistics GetStatistics();
#ifdef SPII_TEST
unsigned TestFaultCount();
float TestAxis(short value);
bool TestControllerInput(Settings&,const XINPUT_GAMEPAD&,float);
unsigned TestReadChecks();
unsigned TestTimingReads();
unsigned TestTimingSamples(unsigned stage);
unsigned TestUiThread();
unsigned TestQueuedMessages();
void TestFrame(IMenu* menu,unsigned stamp);
void TestOwnerThread(DWORD thread);
void TestResetState();
#endif
}
