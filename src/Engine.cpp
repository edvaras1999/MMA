// Show Player In Inventory LE. GPL-3.0; Ivy / Mhacks79.
// Every engine site is LE-only; reproducible decompilation lives in ../evidence/.
#include "Runtime.h"
#include "skse/NiNodes.h"
#include "skse/NiExtraData.h"
#include <cmath>
#include <new>
#include <xinput.h>
#include <atomic>
namespace SPII {
void ApplyReset(bool value,bool discardScene);
extern volatile LONG wheelDelta;
namespace {
constexpr float Pi=3.14159265358979323846f;
constexpr unsigned WatchCount=4, NameCount=14;
const char* const names[NameCount]={"InventoryMenu","MagicMenu","BarterMenu","ContainerMenu","TweenMenu","Main Menu","Loading Menu","RaceSex Menu","Console","MessageBoxMenu","Quantity Menu","Book Menu","Journal Menu","Dialogue Menu"};
alignas(BSFixedString) unsigned char stringStorage[NameCount][sizeof(BSFixedString)];
BSFixedString* menuNames[NameCount]={};
using Advance=void (__thiscall*)(IMenu*,unsigned,unsigned);
using Dtor=void* (__thiscall*)(IMenu*,unsigned);
using Perspective=void (__thiscall*)(void*,void*);
Advance oldAdvance[5]={};Dtor oldDtor[5]={};Perspective oldPerspective[2]={};
using ProcessMessage=unsigned(__thiscall*)(IMenu*,void*);ProcessMessage oldTweenProcess=nullptr;
using ProjectionUpdate=void(__thiscall*)(NiCamera*,void*);ProjectionUpdate oldProjectionUpdate=nullptr;volatile LONG projectionThreadLogged=0;
const uintptr_t tables[5]={0x010E5B90,0x010E6594,0x010E3860,0x010E4098,0x010E537C};
struct Patch {void** slot;void* before;void* after;};Patch patches[14]={};unsigned patchCount=0;
struct OwnedBlur {IMenu* menu=nullptr;void* manager=nullptr;};OwnedBlur blur[5];
struct Ref {
 NiAVObject* object=nullptr;
 void Release(){auto* old=object;object=nullptr;if(old&&InterlockedDecrement(&old->m_uiRefCount)==0){using Fn=void(__thiscall*)(NiAVObject*);reinterpret_cast<Fn>((*reinterpret_cast<void***>(old))[1])(old);}}
 void Set(NiAVObject* value){if(value==object)return;if(value)InterlockedIncrement(&value->m_uiRefCount);Release();object=value;}
};
Ref projectionRef;
struct OwnedPreview{Ref ref;bool restoreVisible=false;};OwnedPreview previews[16];unsigned previewCount=0;
void* sessionCamera=nullptr;float savedFOV=0;unsigned char savedVanity=0;
NiFrustum savedFrustum{};
std::atomic<bool> active{false},pendingPOV{false},cachedFirst{true},cacheFrozen{false};
bool framingDirty=false;volatile LONG abandonRestore=0;
volatile LONG eventWatch=0,eventTween=0;
volatile LONG ownerThread=0,ready=0,resetRequest=0;
DWORD uiThread=0;unsigned sessionMenu=0;bool applyingCamera=false;unsigned frameStamp=0;bool stampValid=false;
void* controlOwner=nullptr;unsigned heldControls=0;
POINT lastMouse{};bool mouseSeeded=false,mouseHeld=false,mouseLocked=false,reloadHeld=false;
ULONGLONG lastTime=0;Statistics statistics;
// Scalar snapshots only; the live RTTI-checked, pinned projection is still resolved.
bool cameraCommitted=false;unsigned char committedCameraData[sizeof(NiCamera)-sizeof(NiAVObject)]={};NiTransform committedLocal{},committedWorld{};
enum TimingStage{OwnedWork,FaceWork,PoseWork,PreviewWork,FramingWork,NativeCameraWork,ReapplyWork,NativeMenuWork,TimingCount};
struct Timing{ULONGLONG sum=0,peak=0;unsigned count=0;};Timing timings[TimingCount];
#ifdef SPII_TEST
unsigned timingReads=0;
#endif
LARGE_INTEGER timingFrequency{};unsigned sessionFrames=0,cameraSamples=0,menuSamples=0,sessionCommits=0,sessionSkips=0,sessionProjectionRepairs=0;
ULONGLONG SampleStart(bool selected){LARGE_INTEGER now{};if(selected&&timingFrequency.QuadPart>0&&LoggingEnabled()&&QueryPerformanceCounter(&now)){
 #ifdef SPII_TEST
 ++timingReads;
 #endif
 return now.QuadPart;}return 0;}
void SampleEnd(TimingStage stage,ULONGLONG start){LARGE_INTEGER now{};if(!start||!QueryPerformanceCounter(&now)||static_cast<ULONGLONG>(now.QuadPart)<start)return;
 auto& t=timings[stage];ULONGLONG elapsed=now.QuadPart-start;t.sum+=elapsed;if(elapsed>t.peak)t.peak=elapsed;++t.count;}
void ReportPerformance(){
 if(timingFrequency.QuadPart<=0)return;
 Log("Performance frames=%u camera-callbacks=%u; sampled every64; inclusive costs overlap; samples can miss stalls",sessionFrames,cameraSamples);
 Log("Camera work commits=%u unchanged-skips=%u",statistics.transforms-sessionCommits,statistics.cameraSkips-sessionSkips);
 Log("Final projection repairs=%u",statistics.projectionRepairs-sessionProjectionRepairs);
 const char* labels[TimingCount]={"owned-work","face","actor-and-scene","item-previews","camera-frame","native-camera","camera-reapply","native-menu-advance"};
 for(unsigned i=0;i<TimingCount;++i){const auto& t=timings[i];if(t.count)Log("Perf %s samples=%u mean-us=%llu peak-us=%llu",labels[i],t.count,t.sum*1000000/t.count/timingFrequency.QuadPart,t.peak*1000000/timingFrequency.QuadPart);}
}
#ifdef SPII_TEST
unsigned readChecks=0;
#endif
using XInputFn=DWORD(WINAPI*)(DWORD,XINPUT_STATE*);XInputFn xinput=nullptr;

template<class T>T* At(uintptr_t address){return Readable(reinterpret_cast<void*>(address),sizeof(T*))?*reinterpret_cast<T**>(address):nullptr;}
template<class T>T& Field(void* p,unsigned offset){return *reinterpret_cast<T*>(static_cast<char*>(p)+offset);}
void* Camera(){auto* p=At<void>(0x012E7288);return Readable(p,0xD8)?p:nullptr;}
void* Player(){auto* p=At<void>(0x01B2E8E4);return Readable(p,0x728)?p:nullptr;}
MenuManager* Menus(){auto* p=At<MenuManager>(0x012E3548);return Readable(p,sizeof(MenuManager))?p:nullptr;}
bool Open(MenuManager* m,unsigned i){return m&&menuNames[i]&&m->IsMenuOpen(menuNames[i]);}
IMenu* Menu(MenuManager* m,unsigned i){auto* p=m&&menuNames[i]?m->GetMenu(menuNames[i]):nullptr;return Readable(p,0x1C)?p:nullptr;}
bool Blocked(MenuManager* m){for(unsigned i=5;i<9;++i)if(Open(m,i))return true;return Open(m,12)||Open(m,13);}
bool Prompt(MenuManager* m){return Open(m,9)||Open(m,10)||Open(m,11)||SettingsOpen();}
NiNode* Root(void* camera){auto* p=Field<NiNode*>(camera,0x1C);return Readable(p,sizeof(NiNode))?p:nullptr;}
NiCamera* Projection(NiNode* root){
 if(!root||!root->m_children.m_data||!root->m_children.m_emptyRunStart||root->m_children.m_emptyRunStart>16)return nullptr;
 const unsigned count=root->m_children.m_emptyRunStart;
 if(!Readable(root->m_children.m_data,count*sizeof(void*)))return nullptr;
 for(unsigned i=0;i<count;++i){
  auto* child=root->m_children.m_data[i];if(!Readable(child,sizeof(NiCamera)))continue;
  auto** table=*reinterpret_cast<void***>(child);if(!Readable(table,3*sizeof(void*))||!Executable(table[2]))continue;
  using Type=const NiRTTI*(__thiscall*)(NiAVObject*);const NiRTTI* type=reinterpret_cast<Type>(table[2])(child);
  for(unsigned depth=0;type&&depth<16;++depth){if(!Readable(type,sizeof(NiRTTI)))break;
   if(Readable(type->name,9,false)&&memcmp(type->name,"NiCamera",9)==0)return static_cast<NiCamera*>(child);type=type->parent;}
 }
 return nullptr;
}
void NodeUpdate(NiAVObject* root){alignas(16) NiAVObject::ControllerUpdateContext ctx{};using Fn=void(__thiscall*)(NiAVObject*,void*);reinterpret_cast<Fn>(0x00AAF320)(root,&ctx);}
void ReleasePreview(){
 for(auto& own:previews){if(own.ref.object&&own.restoreVisible)own.ref.object->m_flags&=~1u;own.restoreVisible=false;own.ref.Release();}
 previewCount=0;
}
void Preview(bool hide){
 if(!hide){ReleasePreview();return;}
 NiAVObject* nodes[16]={};unsigned nodeCount=0;
 auto* manager=At<void>(0x01B2E99C);
 if(Readable(manager,0xE4)){
  const unsigned count=Field<unsigned>(manager,0xD0),flags=Field<unsigned>(manager,0x40);
  const bool local=(flags&0x80000000u)!=0;
  if(count&&count<=16&&(!local||count<=7)){
   auto* data=local?static_cast<char*>(manager)+0x44:Field<char*>(manager,0x44);
   if(Readable(data,count*0x14))for(unsigned i=0;i<count;++i){auto* node=*reinterpret_cast<NiAVObject**>(data+i*0x14+8);if(!Readable(node,sizeof(NiAVObject)))continue;
    bool duplicate=false;for(unsigned j=0;j<nodeCount;++j)duplicate|=nodes[j]==node;if(!duplicate)nodes[nodeCount++]=node;}
  }
 }
 for(auto& own:previews){if(!own.ref.object)continue;bool present=false;for(unsigned i=0;i<nodeCount;++i)present|=nodes[i]==own.ref.object;
  if(!present){if(own.restoreVisible)own.ref.object->m_flags&=~1u;own.restoreVisible=false;own.ref.Release();}}
 previewCount=0;
 for(unsigned i=0;i<nodeCount;++i){OwnedPreview* found=nullptr;for(auto& own:previews)if(own.ref.object==nodes[i]){found=&own;break;}
  if(!found)for(auto& own:previews)if(!own.ref.object){own.ref.Set(nodes[i]);own.restoreVisible=(nodes[i]->m_flags&1)==0;found=&own;break;}
  if(found){nodes[i]->m_flags|=1u;++previewCount;}
 }
}
void RestoreBlur(unsigned i){
 auto own=blur[i];blur[i]={};
 if(!own.menu)return;
 auto* current=At<void>(0x01B2E4C0);
 if(own.menu&&current==own.manager&&Readable(current,0x10)&&Field<unsigned short>(current,0x0C)<0x7FFF){using Fn=void(__thiscall*)(void*);reinterpret_cast<Fn>(0x00897130)(current);}
}
void Blur(IMenu* menu,unsigned i,bool suppress){
 if(blur[i].menu&&blur[i].menu!=menu)RestoreBlur(i);
 if(!suppress){RestoreBlur(i);return;}
 if(!menu||blur[i].menu)return;
 auto* manager=At<void>(0x01B2E4C0);
 if(Readable(manager,0x10)&&Field<unsigned short>(manager,0x0C)>0){
  blur[i]={menu,manager};using Fn=void(__thiscall*)(void*);reinterpret_cast<Fn>(0x00897160)(manager);
 }
}
void ReleaseControls(){
 unsigned bits=heldControls;void* before=controlOwner;heldControls=0;controlOwner=nullptr;
 if(!bits)return;
 auto* current=At<void>(0x012E7458);
 if(bits&&current==before&&Readable(current,0x9C)){using Fn=void(__thiscall*)(void*,unsigned,bool,bool);reinterpret_cast<Fn>(0x00A67A30)(current,bits,true,false);}
}
void HoldControls(bool hold){
 if(!hold){ReleaseControls();return;}
 auto* map=At<void>(0x012E7458);if(!Readable(map,0x9C))return;
 if(controlOwner&&controlOwner!=map)ReleaseControls();
 constexpr unsigned flags=(1u<<0)|(1u<<1)|(1u<<5)|(1u<<9);
 unsigned ours=Field<unsigned>(map,0x90)&flags;
 if(ours){heldControls|=ours;controlOwner=map;using Fn=void(__thiscall*)(void*,unsigned,bool,bool);reinterpret_cast<Fn>(0x00A67A30)(map,ours,false,false);}
}
unsigned State(void* camera){auto* state=Field<void*>(camera,0x20);return Readable(state,0x10)?Field<unsigned>(state,0x0C):~0u;}
bool Eligible(void* player,void* camera){
 if(!player||!camera)return false;auto* root=Root(camera);if(!root||!Projection(root))return false;
 if(!Readable(Field<void*>(player,0x40),4))return false;
 const auto& pos=Field<NiPoint3>(player,0x34);if(!std::isfinite(pos.x)||!std::isfinite(pos.y)||!std::isfinite(pos.z))return false;
 const unsigned state=State(camera);if(state!=0&&state!=7&&state!=8&&state!=9)return false;
 unsigned flags=Field<unsigned>(player,0x68);
 if((flags&(15u<<14))||(flags&0x0FE00000u)||(Field<unsigned char>(player,0x727)&0x20))return false;
 auto* loaded=Field<void*>(player,0x44);return Readable(loaded,0x24)&&Readable(Field<void*>(loaded,0x20),sizeof(NiAVObject));
}
bool ContainerAllowed(){
 unsigned handle=0;if(!Readable(reinterpret_cast<void*>(0x01B3E764),4))return false;handle=*reinterpret_cast<unsigned*>(0x01B3E764);
 if(!handle||handle==0xFFFFFFFFu)return false;
 void* reference=nullptr;using Lookup=bool(__cdecl*)(unsigned*,void**);
 if(!reinterpret_cast<Lookup>(0x004A9180)(&handle,&reference)||!reference)return false;
 bool result=false;
 __try{if(Readable(reference,0x1C))result=Field<unsigned char>(reference,0x12)!=0x3E;}
 __finally{if(Readable(reference,0x1C)){auto* base=static_cast<char*>(reference)+0x14;
  if((InterlockedDecrement(reinterpret_cast<volatile LONG*>(base+4))&0x3FF)==0){using Release=void(__thiscall*)(void*);reinterpret_cast<Release>((*reinterpret_cast<void***>(base))[1])(base);}}}
 return result;
}
void NativeCameraUpdate(void* camera){using Fn=void(__thiscall*)(void*);reinterpret_cast<Fn>(0x00653480)(camera);}
void ForceFirst(void* camera){using Fn=void(__thiscall*)(void*);reinterpret_cast<Fn>(0x0083CE90)(camera);}
float Third(void* camera){
 auto* state=Field<void*>(camera,0x90);if(!Readable(state,0xBC)||Field<unsigned>(state,0x0C)!=9)return Field<float>(camera,0xAC);
 using Fn=void(__thiscall*)(void*,void*);reinterpret_cast<Fn>(0x006533D0)(camera,state);
 // Ending native Tween restores its original FOV before our first framing pass.
 const float restoredFOV=Field<float>(camera,0xAC);NativeCameraUpdate(camera);return restoredFOV;
}
bool ResumeParentTween(void* camera){
 auto* state=Field<void*>(camera,0x88);if(!Readable(state,0x40)||Field<unsigned>(state,0x0C)!=7||Field<void*>(state,8)!=camera)return false;
 // Generic state change preserves the previous gameplay state saved by native Tab.
 using Fn=void(__thiscall*)(void*,void*);reinterpret_cast<Fn>(0x006533D0)(camera,state);NativeCameraUpdate(camera);return true;
}
// Reuses the LE face chain and recursive lock proved in the Mfg Fix LE audit.
struct FaceSnapshot{BSFaceGenAnimationData* data=nullptr;alignas(16) float values[67]={};unsigned char overrideFlag=0;};FaceSnapshot faceSnapshot;
BSFaceGenAnimationData* Face(void* player){
 if(!Readable(player,0x8C))return nullptr;auto* process=Field<void*>(player,0x88);if(!Readable(process,8))return nullptr;
 auto* middle=Field<void*>(process,4);if(!Readable(middle,0xE4))return nullptr;
 auto* head=Field<void*>(middle,0xE0);if(!Readable(head,0xE0))return nullptr;
 auto* data=Field<BSFaceGenAnimationData*>(head,0xDC);return Readable(data,sizeof(*data))?data:nullptr;
}
bool FaceReady(BSFaceGenAnimationData* data){
 const unsigned counts[4]={17,17,17,16};for(unsigned i=0;i<4;++i){const auto& key=data->keyFrames[i];if(key.count!=counts[i]||!Readable(key.values,key.count*sizeof(float)))return false;}return true;
}
void UpdateFace(BSFaceGenAnimationData* data,float dt){
 data->unk1A3=1;data->unk1A4=0;
 using Fn=bool(__thiscall*)(BSFaceGenAnimationData*,float,bool);reinterpret_cast<Fn>(0x0059E5C0)(data,dt>0?dt:1.0f/60,false);
}
void RestoreFace(void* player,bool abandon){
 auto* saved=faceSnapshot.data;faceSnapshot.data=nullptr;if(!saved||abandon)return;
 auto* data=Face(player);if(data!=saved)return;
 auto* lock=reinterpret_cast<SimpleLock*>(reinterpret_cast<char*>(data)+0x1A8);lock->Lock();
 __try{if(FaceReady(data)){unsigned offset=0;for(unsigned i=0;i<4;++i){auto& key=data->keyFrames[i];memcpy(key.values,faceSnapshot.values+offset,key.count*sizeof(float));offset+=key.count;key.isUpdated=0;}data->overrideFlag=faceSnapshot.overrideFlag;UpdateFace(data,1.0f/60);}else data->overrideFlag=faceSnapshot.overrideFlag;}
 __finally{lock->Release();}
}
void NeutralFace(void* player,bool enabled,float dt){
 if(!enabled){RestoreFace(player,false);return;}auto* data=Face(player);if(!data)return;
 if(faceSnapshot.data&&faceSnapshot.data!=data)faceSnapshot.data=nullptr;
 auto* lock=reinterpret_cast<SimpleLock*>(reinterpret_cast<char*>(data)+0x1A8);lock->Lock();
 __try{if(FaceReady(data)){
  if(!faceSnapshot.data){unsigned offset=0;for(unsigned i=0;i<4;++i){const auto& key=data->keyFrames[i];memcpy(faceSnapshot.values+offset,key.values,key.count*sizeof(float));offset+=key.count;}faceSnapshot.overrideFlag=data->overrideFlag;faceSnapshot.data=data;}
  if(data->unk0C){using Release=void(__cdecl*)(void**,void*);reinterpret_cast<Release>(0x0059D870)(reinterpret_cast<void**>(&data->unk0C),reinterpret_cast<void*>(0x01B1A470));}
  for(unsigned i=0;i<4;++i){auto& key=data->keyFrames[i];memset(key.values,0,key.count*sizeof(float));key.isUpdated=0;}data->overrideFlag=1;UpdateFace(data,dt);
 }}__finally{lock->Release();}
}
void Stop(bool leaveForeign=false){
 const bool abandon=InterlockedExchange(&abandonRestore,0)!=0;
 ReleaseControls();ReleasePreview();for(unsigned i=0;i<5;++i){if(abandon)blur[i]={};else RestoreBlur(i);}
 if(faceSnapshot.data)RestoreFace(Player(),abandon);
 if(!active)return;
 active=false;cameraCommitted=false;auto* camera=Camera();
 if(camera==sessionCamera&&!abandon){
  const unsigned state=State(camera);const bool ordinary=state==0||state==7||state==8||state==9;
  Field<unsigned char>(camera,0xD0)=savedVanity;
  pendingPOV=cachedFirst.load();
  const bool parent=Open(Menus(),4);
  if(ordinary){Field<float>(camera,0xAC)=savedFOV;auto* projection=Projection(Root(camera));
   if(projection&&projection==projectionRef.object){projection->m_frustum.m_fLeft=savedFrustum.m_fLeft;projection->m_frustum.m_fRight=savedFrustum.m_fRight;projection->m_frustum.m_fTop=savedFrustum.m_fTop;projection->m_frustum.m_fBottom=savedFrustum.m_fBottom;}}
  if(!leaveForeign&&ordinary){
   if(parent)ResumeParentTween(camera);
   else{if(pendingPOV)ForceFirst(camera);pendingPOV=false;NativeCameraUpdate(camera);Field<float>(camera,0xAC)=savedFOV;
    auto* projection=Projection(Root(camera));if(projection&&projection==projectionRef.object){projection->m_frustum.m_fLeft=savedFrustum.m_fLeft;projection->m_frustum.m_fRight=savedFrustum.m_fRight;projection->m_frustum.m_fTop=savedFrustum.m_fTop;projection->m_frustum.m_fBottom=savedFrustum.m_fBottom;}}
  }
 }
 projectionRef.Release();sessionCamera=nullptr;mouseSeeded=mouseHeld=mouseLocked=false;lastTime=0;
 ReportPerformance();++statistics.stops;if(framingDirty){if(GetSettings().remember)PutSettings(GetSettings(),true);framingDirty=false;}
 Log("Camera restored thread=%lu; native camera reapplies=%u paused scene updates=%u",GetCurrentThreadId(),statistics.cameraPasses,statistics.poseUpdates);
}
bool Start(void* player,void* camera){
 Settings opening=GetSettings();if(opening.turnToFront&&opening.resetOrbitOnOpen){opening.yaw=0;PutSettings(opening,false);}
 sessionCamera=camera;savedFOV=Field<float>(camera,0xAC);savedVanity=Field<unsigned char>(camera,0xD0);
 auto* projection=Projection(Root(camera));if(projection){savedFrustum=projection->m_frustum;projectionRef.Set(projection);}
 const unsigned stateBefore=State(camera);pendingPOV=false;Field<unsigned char>(camera,0xD0)=0;
 if(stateBefore!=9){const float restoredFOV=Third(camera);if(stateBefore==7&&std::isfinite(restoredFOV)&&restoredFOV>0&&restoredFOV<180){savedFOV=restoredFOV;
  if(auto* after=Projection(Root(camera))){projectionRef.Set(after);savedFrustum=after->m_frustum;}}}
 if(State(camera)!=8&&State(camera)!=9){Field<unsigned char>(camera,0xD0)=savedVanity;projectionRef.Release();sessionCamera=nullptr;return false;}
 cameraCommitted=false;for(auto& t:timings)t={};sessionFrames=cameraSamples=menuSamples=0;sessionCommits=statistics.transforms;sessionSkips=statistics.cameraSkips;sessionProjectionRepairs=statistics.projectionRepairs;active=true;
 ++statistics.starts;mouseSeeded=false;Log("Camera started thread=%lu perspective=%s",GetCurrentThreadId(),cachedFirst?"first":"third");
 Log("Opening front=%u reset-orbit=%u orbit-tenths=%ld",opening.turnToFront,opening.resetOrbitOnOpen,static_cast<long>(opening.yaw*10));
 Log("Opening child=%s tween-open=%u state-before=%u restore-fov-tenths=%ld",names[sessionMenu],Open(Menus(),4),stateBefore,static_cast<long>(savedFOV*10));
 Log("Opening placement screen-x-milli=%ld screen-y-milli=%ld",static_cast<long>(opening.screenX*1000),static_cast<long>(opening.screenY*1000));
 (void)player;return true;
}
NiPoint3 Vec(float x,float y,float z){NiPoint3 v;v.x=x;v.y=y;v.z=z;return v;}
NiPoint3 Add(NiPoint3 a,NiPoint3 b){return Vec(a.x+b.x,a.y+b.y,a.z+b.z);}
NiPoint3 Mul(NiPoint3 a,float s){return Vec(a.x*s,a.y*s,a.z*s);}
float Dot(NiPoint3 a,NiPoint3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
bool FiniteTransform(const NiTransform& value){
 if(!std::isfinite(value.pos.x)||!std::isfinite(value.pos.y)||!std::isfinite(value.pos.z)||!std::isfinite(value.scale))return false;
 for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)if(!std::isfinite(value.rot.data[i][j]))return false;return true;
}
float BodyHeading(void* player,NiAVObject* body){
 const float fallback=Field<NiPoint3>(player,0x28).z;
 if(body&&FiniteTransform(body->m_worldTransform)){
  const float x=body->m_worldTransform.rot.data[0][1],y=body->m_worldTransform.rot.data[1][1],length=x*x+y*y;
  if(length>0.25f&&length<2.25f)return std::atan2(x,y);
 }
 return fallback;
}
void FrameCameraBody(void* player,void* camera,const Settings& s){
 auto* root=Root(camera);auto* projection=Projection(root);if(!root||!projection)return;
 if(projection!=projectionRef.object){cameraCommitted=false;projectionRef.Set(projection);savedFrustum=projection->m_frustum;}
 float aspect=(projection->m_frustum.m_fRight-projection->m_frustum.m_fLeft)/(projection->m_frustum.m_fTop-projection->m_frustum.m_fBottom);
 if(!std::isfinite(aspect)||aspect<0.5f||aspect>6)aspect=16.0f/9.0f;
 float htan=std::tan(s.fov*Pi/360),vtan=htan/aspect;

 auto* loaded=Field<void*>(player,0x44);auto* body=Readable(loaded,0x24)?Field<NiAVObject*>(loaded,0x20):nullptr;
 float scale=Readable(body,sizeof(NiAVObject))?Clamp(body->m_worldTransform.scale,0.25f,4):1;
 float distance=s.autoFrame?130.0f*scale/(2*vtan*0.90f)*(s.distance/215.0f):s.distance;
 distance=Clamp(distance,70,1500);
 float heading=BodyHeading(player,body)+(s.turnToFront?Pi:0)+s.yaw*Pi/180;
 if(!std::isfinite(heading))return;
 float pitch=s.pitch*Pi/180;float ch=std::cos(heading),sh=std::sin(heading),cp=std::cos(pitch),sp=std::sin(pitch);
 NiPoint3 right=Vec(ch,-sh,0),forward=Vec(sh*cp,ch*cp,sp),up=Vec(-sh*sp,-ch*sp,cp);
 NiPoint3 position=Field<NiPoint3>(player,0x34);position.z+=s.height*scale;
 position=Add(position,Mul(forward,-distance));
 // Shift the projection rather than moving the eye sideways: the model stays frontal.
 NiTransform transform;transform.scale=1;
 for(unsigned row=0;row<3;++row){transform.rot.data[row][0]=(&right.x)[row];transform.rot.data[row][1]=(&forward.x)[row];transform.rot.data[row][2]=(&up.x)[row];}
 transform.pos=position;NiTransform result=transform;
 if(root->m_parent){
  if(!Readable(root->m_parent,sizeof(NiAVObject)))return;
  const auto& parent=root->m_parent->m_worldTransform;if(!FiniteTransform(parent))return;float scaleParent=parent.scale;if(!std::isfinite(scaleParent)||std::fabs(scaleParent)<0.001f)return;
  NiPoint3 d=Add(position,Mul(parent.pos,-1));NiTransform local;local.scale=1/scaleParent;
  for(unsigned row=0;row<3;++row){NiPoint3 axis=Vec(parent.rot.data[0][row],parent.rot.data[1][row],parent.rot.data[2][row]);(&local.pos.x)[row]=Dot(axis,d)/scaleParent;
   for(unsigned col=0;col<3;++col)local.rot.data[row][col]=Dot(axis,Vec(transform.rot.data[0][col],transform.rot.data[1][col],transform.rot.data[2][col]));}
  result=local;
 }
 if(!FiniteTransform(result))return;
 const float centerX=htan*(1-2*s.screenX),centerY=vtan*(2*s.screenY-1);
 // Check live values rather than caching engine readiness or pointers. Native
 // overwrites, lens/clip changes and projection child transforms invalidate this.
 if(cameraCommitted&&Field<float>(camera,0xAC)==s.fov&&
  projection->m_frustum.m_fLeft==centerX-htan&&projection->m_frustum.m_fRight==centerX+htan&&
  projection->m_frustum.m_fTop==centerY+vtan&&projection->m_frustum.m_fBottom==centerY-vtan&&
  memcmp(reinterpret_cast<char*>(projection)+sizeof(NiAVObject),committedCameraData,sizeof(committedCameraData))==0&&
  memcmp(&root->m_localTransform,&result,sizeof(NiTransform))==0&&memcmp(&root->m_worldTransform,&transform,sizeof(NiTransform))==0&&
  memcmp(&projection->m_localTransform,&committedLocal,sizeof(NiTransform))==0&&memcmp(&projection->m_worldTransform,&committedWorld,sizeof(NiTransform))==0){++statistics.cameraSkips;return;}
 projection->m_frustum.m_fLeft=centerX-htan;projection->m_frustum.m_fRight=centerX+htan;projection->m_frustum.m_fTop=centerY+vtan;projection->m_frustum.m_fBottom=centerY-vtan;
 Field<float>(camera,0xAC)=s.fov;root->m_localTransform=result;
 NodeUpdate(root);memcpy(committedCameraData,reinterpret_cast<char*>(projection)+sizeof(NiAVObject),sizeof(committedCameraData));committedLocal=projection->m_localTransform;committedWorld=projection->m_worldTransform;cameraCommitted=true;++statistics.transforms;
}
void FrameCamera(void* player,void* camera,const Settings& s){
 if(applyingCamera)return;applyingCamera=true;
 __try{FrameCameraBody(player,camera,s);}__finally{applyingCamera=false;}
}
void KeepCameraAfterNativeUpdate(void* state,void* next){
 if(!active||Disabled()||applyingCamera||uiThread!=GetCurrentThreadId()||!InterlockedCompareExchange(&ready,0,0)||InterlockedCompareExchange(&resetRequest,0,0))return;
 auto* menus=Menus();if(!menus||!Open(menus,sessionMenu)||Blocked(menus))return;
 auto* camera=Camera();auto* player=Player();if(camera!=sessionCamera||!camera||Field<void*>(camera,0x20)!=state||(State(camera)!=8&&State(camera)!=9)||!Eligible(player,camera))return;
 if(!Readable(next,sizeof(void*),false)||*static_cast<void**>(next))return;
 Settings s=GetSettings();if(s.enabled){FrameCamera(player,camera,s);++statistics.cameraPasses;}
}
float Axis(short v){const int magnitude=v<0?-static_cast<int>(v):v;if(magnitude<=7000)return 0;return (v<0?-1.0f:1.0f)*static_cast<float>(magnitude-7000)/(v<0?25768.0f:25767.0f);}
bool ControllerInput(Settings& s,const XINPUT_GAMEPAD& g,float dt){
 const float rx=Axis(g.sThumbRX),ry=Axis(g.sThumbRY),lx=Axis(g.sThumbLX),ly=Axis(g.sThumbLY);
 const float lt=g.bLeftTrigger<=30?0.0f:static_cast<float>(g.bLeftTrigger-30)/225;
 const float rt=g.bRightTrigger<=30?0.0f:static_cast<float>(g.bRightTrigger-30)/225;
 s.yaw+=rx*dt*100;s.distance-=ry*dt*100;s.screenX+=lx*dt*0.4f;s.screenY-=ly*dt*0.4f;s.fov+=(rt-lt)*dt*40;
 return dt>0&&(rx!=0||ry!=0||lx!=0||ly!=0||rt!=lt);
}
void Input(Settings& s,bool prompt,float dt){
 
 POINT cursor{};bool got=GetCursorPos(&cursor)!=FALSE;bool held=(GetAsyncKeyState(static_cast<int>(s.rotateButton))&0x8000)!=0;
 bool middle=(GetAsyncKeyState(VK_MBUTTON)&0x8000)!=0;bool any=held||middle;
 RECT client{};HWND window=GetForegroundWindow();DWORD pid=0;GetWindowThreadProcessId(window,&pid);bool focused=pid==GetCurrentProcessId()&&GetClientRect(window,&client);
 POINT local=cursor;if(focused)ScreenToClient(window,&local);
 if(any&&!mouseHeld)mouseLocked=!focused||local.x<static_cast<LONG>((client.right-client.left)*0.45f);
 bool change=false;
 if(got&&mouseSeeded&&focused&&any&&mouseHeld&&!mouseLocked&&!prompt&&s.cameraControls){
  float dx=Clamp(static_cast<float>(cursor.x-lastMouse.x),-100,100),dy=Clamp(static_cast<float>(cursor.y-lastMouse.y),-100,100);
  if(held)s.yaw+=dx*s.sensitivity*180/Pi;
  if(middle){if(GetAsyncKeyState(VK_CONTROL)&0x8000)s.pitch+=dy*s.sensitivity*25;else{s.screenX-=dx*0.001f;s.screenY-=dy*0.001f;}}
  change=dx!=0||dy!=0;
 }
 if(got){lastMouse=cursor;mouseSeeded=true;}mouseHeld=any;
 if(xinput&&focused&&!prompt&&s.cameraControls){XINPUT_STATE state{};
  if(xinput(0,&state)==ERROR_SUCCESS&&(state.Gamepad.wButtons&s.controllerButton)){
   change|=ControllerInput(s,state.Gamepad,dt);
  }
 }
 // Wheel is supplied by the native input sink; no game window procedure is replaced.
 LONG wheel=InterlockedExchange(&wheelDelta,0);
 if(wheel&&!prompt&&s.cameraControls&&(GetAsyncKeyState(VK_CONTROL)&0x8000)){
  if(GetAsyncKeyState(VK_SHIFT)&0x8000)s.fov-=static_cast<float>(wheel)*3;else s.distance-=static_cast<float>(wheel)*12;change=true;
 }
 if(change){Normalize(s);PutSettings(s,false);framingDirty=true;}
}
void Body(IMenu* source,unsigned stamp,float interval){
 ++statistics.ticks;
 DWORD thread=GetCurrentThreadId();DWORD owner=static_cast<DWORD>(InterlockedCompareExchange(&ownerThread,0,0));
 if(!owner||owner!=thread){++statistics.wrongThreads;if(statistics.wrongThreads==1)Log("Camera deferred: UI thread=%lu owner=%lu",thread,owner);return;}
 if(uiThread!=thread){uiThread=thread;Log("Accepted UI/camera owner thread=%lu",thread);}
 LONG reset=InterlockedExchange(&resetRequest,0);if(reset)ApplyReset((reset&2)!=0,(reset&4)!=0);
 if(InterlockedCompareExchange(&resetRequest,0,0))return;
 FlushSettingsSave(false);
 auto* menus=Menus();if(!menus)return;
 const bool reload=(GetAsyncKeyState(VK_F10)&0x8000)!=0;if(reload&&!reloadHeld)ReloadSettings();reloadHeld=reload;
 bool watching[4]={};Settings s=GetSettings();bool wanted=false;
 for(unsigned i=0;i<4;++i){watching[i]=Open(menus,i)&&(i==0||(i==1&&s.magic)||(i==2&&s.barter)||(i==3&&s.container&&ContainerAllowed()));wanted|=watching[i];}
 cacheFrozen=wanted||Open(menus,4);
 // No frame-lifetime pointers or page cache: inactive HUD only probes menu state.
 if(!wanted&&!active&&!pendingPOV&&!heldControls&&!projectionRef.object&&!previewCount&&!faceSnapshot.data&&!blur[0].menu&&!blur[1].menu&&!blur[2].menu&&!blur[3].menu&&!blur[4].menu)return;
 if(wanted){IMenu* primary=nullptr;for(unsigned i=0;i<4;++i)if(watching[i]){primary=Menu(menus,i);sessionMenu=i;break;}if(source!=primary)return;}
 if(stampValid&&frameStamp==stamp)return;stampValid=true;frameStamp=stamp;
 if(Disabled()||!InterlockedCompareExchange(&ready,0,0)||!s.enabled||!wanted||Blocked(menus)){
  Stop(Blocked(menus));
  if(pendingPOV&&!Open(menus,4)&&!Blocked(menus)){auto* camera=Camera();if(camera&&(State(camera)==0||State(camera)==8||State(camera)==9)){if(cachedFirst){ForceFirst(camera);NativeCameraUpdate(camera);Field<float>(camera,0xAC)=savedFOV;}pendingPOV=false;}}
  return;
 }
 auto* camera=Camera();auto* player=Player();if(!Eligible(player,camera)){Stop(true);return;}
 if(!active){if(!s.remember){Settings live=s;ReloadSettings();Settings disk=GetSettings();
  live.screenX=disk.screenX;live.screenY=disk.screenY;live.distance=disk.distance;live.height=disk.height;live.pitch=disk.pitch;live.yaw=disk.yaw;live.fov=disk.fov;s=live;PutSettings(s,false);}
  if(!Start(player,camera))return;s=GetSettings();}
 if(camera!=sessionCamera){Stop(true);return;}
 // A late Tween completion can restore first person after the menu has started.
 if(State(camera)==0||State(camera)==7){Third(camera);if(State(camera)!=9&&State(camera)!=8){Stop(true);return;}}
 const bool sample=(sessionFrames++&63)==0;ULONGLONG workTime=SampleStart(sample);
 float dt=Clamp(interval,0,0.1f);
 bool prompt=Prompt(menus);HoldControls(s.lockControls&&!prompt);
 for(unsigned i=0;i<4;++i)Blur(watching[i]?Menu(menus,i):nullptr,i,s.clearBlur&&watching[i]);
 Blur(Open(menus,4)?Menu(menus,4):nullptr,4,s.clearBlur&&Open(menus,4));
 Input(s,prompt,dt);
 bool paused=false;for(unsigned i=0;i<4;++i)if(watching[i]){auto* view=Menu(menus,i);paused|=view&&(view->flags&IMenu::kType_PauseGame)!=0;}
 const bool souls=GetModuleHandleA("SkyrimSouls.dll")||GetModuleHandleA("SkyrimSoulsLE.dll");
 ULONGLONG stepTime=SampleStart(sample);NeutralFace(player,s.neutralFace&&paused&&!souls,dt);SampleEnd(FaceWork,stepTime);
 stepTime=SampleStart(sample);
 // Native Actor animation only: graph owns its private locks; no physics query,
 // no world-unpause switch or third-party Havok entry point is called.
 if(s.animatePaused&&paused&&!prompt&&dt>0&&!souls){
  auto* process=Field<void*>(player,0x88);void* middle=Readable(process,8)?Field<void*>(process,4):nullptr;
  if(Readable(middle,0x160)){using Animate=void(__thiscall*)(void*,float);reinterpret_cast<Animate>(0x00734E80)(player,dt);}
 }
 if(s.animatePaused&&paused&&!prompt&&!souls){
  auto* loaded=Field<void*>(player,0x44);auto* body=Readable(loaded,0x24)?Field<NiAVObject*>(loaded,0x20):nullptr;
  if(Readable(body,sizeof(NiAVObject))){NodeUpdate(body);++statistics.poseUpdates;}
 }
 SampleEnd(PoseWork,stepTime);stepTime=SampleStart(sample);Preview(s.hidePreviews);SampleEnd(PreviewWork,stepTime);
 stepTime=SampleStart(sample);FrameCamera(player,camera,s);SampleEnd(FramingWork,stepTime);SampleEnd(OwnedWork,workTime);
}
template<unsigned I>void __fastcall AdvanceHook(IMenu* menu,void*,unsigned dt,unsigned stamp){
 __try{const bool sample=active&&uiThread==GetCurrentThreadId()&&(menuSamples++&63)==0;ULONGLONG time=SampleStart(sample);oldAdvance[I](menu,dt,stamp);SampleEnd(NativeMenuWork,time);float interval;memcpy(&interval,&dt,4);OnFrame(menu,stamp,interval);}__except(Fault("advance chain",GetExceptionInformation())){ResetRuntime(false);}
}
template<unsigned I>void* __fastcall DtorHook(IMenu* menu,void*,unsigned flags){
 __try{OnMenuDestroyed(menu,I);return oldDtor[I](menu,flags);}__except(Fault("destructor chain",GetExceptionInformation())){ResetRuntime(false);return menu;}
}
unsigned __fastcall TweenProcessHook(IMenu* menu,void*,void* message){
 __try{
  // Native update message0 restores missing Tween blur when its counter is zero.
  // Repay our parent loan around that callback so it never creates a second debt.
  const bool borrowed=blur[4].menu==menu&&Readable(message,12)&&Field<unsigned>(message,4)==0;
  if(borrowed)RestoreBlur(4);
  const unsigned result=oldTweenProcess(menu,message);
  if(borrowed&&active){auto* menus=Menus();if(menus&&Open(menus,sessionMenu)&&Menu(menus,4)==menu)Blur(menu,4,GetSettings().clearBlur);}
  return result;
 }__except(Fault("Tween message chain",GetExceptionInformation())){ResetRuntime(false);return 2;}
}
void KeepFinalProjection(NiCamera* projection){
 // NiCamera::UpdateWorldData is reached by the native FOV setter AFTER menu/
 // camera-state callbacks. Repair only our pinned portrait camera, before its
 // original matrix builder reads the planes. Never recurse into node updates.
 if(!active||projection!=projectionRef.object||Disabled()||uiThread!=GetCurrentThreadId()||!InterlockedCompareExchange(&ready,0,0)||InterlockedCompareExchange(&resetRequest,0,0))return;
 auto* camera=Camera();auto* menus=Menus();
 if(!camera||camera!=sessionCamera||(State(camera)!=8&&State(camera)!=9)||!menus||!Open(menus,sessionMenu)||Blocked(menus)||!Readable(projection,sizeof(NiCamera)))return;
 const Settings s=GetSettings();if(!s.enabled)return;
 auto& f=projection->m_frustum;const float width=f.m_fRight-f.m_fLeft,height=f.m_fTop-f.m_fBottom;
 if(f.m_bOrtho||!std::isfinite(width)||!std::isfinite(height)||width<=0||height<=0||!std::isfinite(f.m_fLeft)||!std::isfinite(f.m_fBottom))return;
 const float x=0.5f*(1-(f.m_fLeft+f.m_fRight)/width),y=0.5f*(1+(f.m_fTop+f.m_fBottom)/height);
 if(std::fabs(x-s.screenX)<0.00001f&&std::fabs(y-s.screenY)<0.00001f)return;
 const float cx=width*(0.5f-s.screenX),cy=height*(s.screenY-0.5f);
 f.m_fLeft=cx-width/2;f.m_fRight=cx+width/2;f.m_fTop=cy+height/2;f.m_fBottom=cy-height/2;++statistics.projectionRepairs;
}
void __fastcall ProjectionUpdateHook(NiCamera* projection,void*,void* context){
 __try{if(active&&projection==projectionRef.object&&InterlockedCompareExchange(&projectionThreadLogged,1,0)==0)Log("Projection callback thread=%lu owner=%lu",GetCurrentThreadId(),uiThread);KeepFinalProjection(projection);}__except(Fault("projection framing",GetExceptionInformation())){ResetRuntime(false);}
 // Always retain the original receiver/context call, even if repair declined/faulted.
 __try{oldProjectionUpdate(projection,context);}__except(Fault("projection chain",GetExceptionInformation())){ResetRuntime(false);}
}
template<unsigned I>void __fastcall PerspectiveHook(void* state,void*,void* next){
 __try{const bool sample=I==1&&active&&uiThread==GetCurrentThreadId()&&(cameraSamples++&63)==0;
  ULONGLONG time=SampleStart(sample);oldPerspective[I](state,next);SampleEnd(NativeCameraWork,time);
  RememberPerspective(I==0);time=SampleStart(sample);if(I==1)KeepCameraAfterNativeUpdate(state,next);SampleEnd(ReapplyWork,time);}__except(Fault("camera chain",GetExceptionInformation())){ResetRuntime(false);}
}
bool PatchSlot(uintptr_t table,unsigned slot,void* after,void** saved){
 auto** address=reinterpret_cast<void**>(table+slot*4);if(!Readable(address,4))return false;
 void* before=*address;if(!Executable(before)||before==after)return false;
 DWORD protection=0;if(!VirtualProtect(address,4,PAGE_READWRITE,&protection))return false;
 void* observed=InterlockedCompareExchangePointer(address,after,before);DWORD ignored=0;VirtualProtect(address,4,protection,&ignored);
 if(observed!=before)return false;
 *saved=before;patches[patchCount++]={address,before,after};return true;
}
}
volatile LONG wheelDelta=0;
bool Readable(const void* pointer,size_t length,bool object){
#ifdef SPII_TEST
 ++readChecks;
#endif
 uintptr_t a=reinterpret_cast<uintptr_t>(pointer);if(!a||!length||(object&&(a&3))||a>=0xFFFF0000u||length>0xFFFF0000u-a)return false;
 uintptr_t end=a+length;while(a<end){MEMORY_BASIC_INFORMATION info{};if(!VirtualQuery(reinterpret_cast<void*>(a),&info,sizeof(info))||info.State!=MEM_COMMIT||(info.Protect&(PAGE_GUARD|PAGE_NOACCESS)))return false;
  uintptr_t next=reinterpret_cast<uintptr_t>(info.BaseAddress)+info.RegionSize;if(next<=a)return false;a=next;}return true;
}
bool Executable(const void* pointer){MEMORY_BASIC_INFORMATION info{};if(!pointer||!VirtualQuery(pointer,&info,sizeof(info))||info.State!=MEM_COMMIT||(info.Protect&(PAGE_GUARD|PAGE_NOACCESS)))return false;return (info.Protect&0xF0)!=0;}
void InitializeRuntime(){
 QueryPerformanceFrequency(&timingFrequency);
 for(unsigned i=0;i<NameCount;++i)menuNames[i]=new(stringStorage[i])BSFixedString(names[i]);
 const char* dlls[]={"xinput1_4.dll","xinput1_3.dll","xinput9_1_0.dll"};
 for(const char* dll:dlls){HMODULE module=GetModuleHandleA(dll);if(!module)module=LoadLibraryA(dll);if(module){xinput=reinterpret_cast<XInputFn>(GetProcAddress(module,"XInputGetState"));if(xinput)break;}}
 InterlockedExchange(&ready,1);
}
void PrepareForMenu(bool opening,const char* name){
 for(unsigned i=0;i<4;++i)if(strcmp(name,names[i])==0){if(opening)InterlockedOr(&eventWatch,1L<<i);else InterlockedAnd(&eventWatch,~(1L<<i));return;}
 if(strcmp(name,"TweenMenu")==0)InterlockedExchange(&eventTween,opening?1:0);
}
void RememberPerspective(bool first){
 InterlockedExchange(&ownerThread,static_cast<LONG>(GetCurrentThreadId()));
 if(!active&&!cacheFrozen&&!InterlockedCompareExchange(&eventWatch,0,0)&&!InterlockedCompareExchange(&eventTween,0,0))cachedFirst=first;
}
void OnFrame(IMenu* menu,unsigned stamp,float interval){__try{Body(menu,stamp,interval);}__except(Fault("menu frame",GetExceptionInformation())){__try{Stop(true);}__except(EXCEPTION_EXECUTE_HANDLER){ReleaseControls();}}}
void OnMenuDestroyed(IMenu* menu,unsigned type){__try{if(blur[type].menu==menu)RestoreBlur(type);}__except(Fault("menu destructor",GetExceptionInformation())){blur[type]={};}}
void ApplyReset(bool value,bool discardScene){
 if(discardScene){InterlockedExchange(&abandonRestore,1);pendingPOV=false;}
 Stop(false);auto* camera=discardScene?nullptr:Camera();if(pendingPOV&&cachedFirst&&camera&&(State(camera)==0||State(camera)==8||State(camera)==9)){ForceFirst(camera);NativeCameraUpdate(camera);Field<float>(camera,0xAC)=savedFOV;}
 cacheFrozen=false;cachedFirst=true;pendingPOV=false;stampValid=false;InterlockedExchange(&eventWatch,0);InterlockedExchange(&eventTween,0);InterlockedExchange(&wheelDelta,0);
 if(value&&!InterlockedCompareExchange(&resetRequest,0,0))InterlockedExchange(&ready,1);
}
void ResetRuntime(bool value,bool mayTeardown){
 InterlockedExchange(&ready,0);
 const DWORD owner=static_cast<DWORD>(InterlockedCompareExchange(&ownerThread,0,0));
 if(owner&&owner==GetCurrentThreadId()){
  LONG pending=InterlockedExchange(&resetRequest,0);__try{ApplyReset(value,(pending&4)!=0);}__except(Fault("reset",GetExceptionInformation())){}
 }else{
  LONG before=0,after=0;do{before=InterlockedCompareExchange(&resetRequest,0,0);after=(value?3:1)|(before&4)|(mayTeardown?4:0);}while(InterlockedCompareExchange(&resetRequest,after,before)!=before);
  Log("Reset queued for camera owner thread=%lu discard-scene=%u",owner,(after&4)!=0);
 }
}
void SaveFraming(){if(framingDirty){RequestSettingsSave();framingDirty=false;}}
Statistics GetStatistics(){return statistics;}
bool WantsWheelInput(){return active&&!Disabled()&&InterlockedCompareExchange(&ready,0,0)!=0;}
bool InstallHooks(){
 bool ok=true;
 #define ADV(i) ok=ok&&PatchSlot(tables[i],5,reinterpret_cast<void*>(&AdvanceHook<i>),reinterpret_cast<void**>(&oldAdvance[i]))
 ADV(0);ADV(1);ADV(2);ADV(3);ADV(4);
 #undef ADV
 #define DTOR(i) ok=ok&&PatchSlot(tables[i],0,reinterpret_cast<void*>(&DtorHook<i>),reinterpret_cast<void**>(&oldDtor[i]))
 DTOR(0);DTOR(1);DTOR(2);DTOR(3);
 ok=ok&&PatchSlot(0x010E8140,0,reinterpret_cast<void*>(&DtorHook<4>),reinterpret_cast<void**>(&oldDtor[4]));
 ok=ok&&PatchSlot(0x010E8140,4,reinterpret_cast<void*>(&TweenProcessHook),reinterpret_cast<void**>(&oldTweenProcess));
 #undef DTOR
 ok=ok&&PatchSlot(0x010E30C8,3,reinterpret_cast<void*>(&PerspectiveHook<0>),reinterpret_cast<void**>(&oldPerspective[0]));
 ok=ok&&PatchSlot(0x010E29EC,3,reinterpret_cast<void*>(&PerspectiveHook<1>),reinterpret_cast<void**>(&oldPerspective[1]));
 ok=ok&&PatchSlot(0x01117CFC,0x2D,reinterpret_cast<void*>(&ProjectionUpdateHook),reinterpret_cast<void**>(&oldProjectionUpdate));
 if(!ok){for(unsigned i=patchCount;i>0;--i){auto& patch=patches[i-1];DWORD old=0;if(VirtualProtect(patch.slot,4,PAGE_READWRITE,&old)){InterlockedCompareExchangePointer(patch.slot,patch.before,patch.after);DWORD ignored=0;VirtualProtect(patch.slot,4,old,&ignored);}}patchCount=0;}
 return ok;
}
#ifdef SPII_TEST
float TestAxis(short value){return Axis(value);}
bool TestControllerInput(Settings& s,const XINPUT_GAMEPAD& g,float dt){return ControllerInput(s,g,dt);}
unsigned TestReadChecks(){return readChecks;}
unsigned TestTimingReads(){return timingReads;}
unsigned TestTimingSamples(unsigned stage){return stage<TimingCount?timings[stage].count:0;}
unsigned TestUiThread(){return uiThread;}
void TestFrame(IMenu* menu,unsigned stamp){OnFrame(menu,stamp);}
void TestOwnerThread(DWORD thread){InterlockedExchange(&ownerThread,static_cast<LONG>(thread));}
void TestResetState(){active=false;pendingPOV=false;cacheFrozen=false;cachedFirst=true;stampValid=false;statistics={};uiThread=0;InterlockedExchange(&ready,1);}
#endif
}
