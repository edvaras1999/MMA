// Show Player In Inventory LE. GPL-3.0; upstream Ivy, LE adaptation Mhacks79.
#pragma once
#include <windows.h>
namespace SPII {
struct Settings {
 bool enabled=true, magic=true, barter=false, container=false, hidePreviews=true;
 bool cameraControls=true, lockControls=true, remember=true, clearBlur=true;
 bool autoFrame=true, turnToFront=true, resetOrbitOnOpen=true, logging=true, animatePaused=true, neutralFace=true;
 float screenX=0.82f, screenY=0.52f, distance=215.0f, height=65.0f;
 float pitch=0.0f, yaw=0.0f, fov=75.0f, sensitivity=0.008f;
 unsigned rotateButton=2, controllerButton=0x0200;
};
float Clamp(float value,float lo,float hi);
float ParseDecimal(const char* text,float fallback);
void Normalize(Settings& value);
Settings GetSettings();
bool LoggingEnabled();
void PutSettings(Settings value,bool persist);
void InitializeSettings(const char* path=nullptr);
void ReloadSettings();
void RequestSettingsSave();
void FlushSettingsSave(bool force);
const char* IniPath();
unsigned SettingsRevision();
}

#ifdef SPII_TEST
namespace SPII {unsigned TestSettingsWrites();void TestSaveDelayElapsed();}
#endif
