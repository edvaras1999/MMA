"""Compile isolated defects against the actual-source native harness."""
from pathlib import Path
import argparse,subprocess,json,hashlib
root=Path(__file__).resolve().parent
p=argparse.ArgumentParser();p.add_argument('--sdk',required=True);p.add_argument('--msbuild',required=True);a=p.parse_args()
out=root/'build/mutations-test7';out.mkdir(parents=True,exist_ok=True)
engine=(root/'src/Engine.cpp').read_text(encoding='utf8');settings=(root/'src/Settings.cpp').read_text(encoding='utf8')
cases=[
('controls-release','Engine','ReleaseControls();ReleasePreview();','ReleasePreview();'),
('fov-restore','Engine','NativeCameraUpdate(camera);Field<float>(camera,0xAC)=savedFOV;','NativeCameraUpdate(camera);'),
('frustum-restore','Engine','projection->m_frustum.m_fRight=savedFrustum.m_fRight;',''),
('unowned-clip','Engine','projection->m_frustum.m_fLeft=savedFrustum.m_fLeft;','projection->m_frustum=savedFrustum;projection->m_frustum.m_fLeft=savedFrustum.m_fLeft;'),
('frame-dedup','Engine','if(stampValid&&frameStamp==stamp)return;',''),
('destructor-blur','Engine','if(blur[type].menu==menu)RestoreBlur(type);',''),
('animation-clamp','Engine','float dt=Clamp(interval,0,0.1f);','float dt=interval;'),
('paused-animation','Engine','if(s.animatePaused&&paused&&!prompt&&dt>0&&!souls)','if(s.animatePaused&&!prompt&&dt>0&&!souls)'),
('animation-receiver','Engine','(0x00734E80)(player,dt);','(0x00734E80)(camera,dt);'),
('face-weights','Engine','memcpy(key.values,faceSnapshot.values+offset,key.count*sizeof(float));',''),
('face-override','Engine','data->overrideFlag=faceSnapshot.overrideFlag;','data->overrideFlag=1;'),
('face-unlock','Engine','}}__finally{lock->Release();}','}}__finally{}'),
('comma-decimal','Settings',"(*text=='.'||*text==',')","(*text=='.')"),
('projection-rtti','Engine','if(memcmp','if(memcmp')
]
# A replacement at the acceptance point bypasses the complete RTTI chain.
cases[-1]=('projection-rtti','Engine','if(Readable(type->name,9,false)&&memcmp(type->name,"NiCamera",9)==0)','if(true)')
cases += [
 ('queued-reset','Engine','LONG reset=InterlockedExchange(&resetRequest,0);if(reset)ApplyReset((reset&2)!=0,(reset&4)!=0);',''),
 ('foreign-reset','Engine','if(owner&&owner==GetCurrentThreadId())','if(true)'),
 ('idle-work','Engine','if(!wanted&&!active&&!pendingPOV&&!heldControls&&!projectionRef.object&&!previewCount&&!faceSnapshot.data&&!blur[0].menu&&!blur[1].menu&&!blur[2].menu&&!blur[3].menu&&!blur[4].menu)return;',''),
 ('axis-discontinuity','Engine','return (v<0?-1.0f:1.0f)*static_cast<float>(magnitude-7000)/(v<0?25768.0f:25767.0f);','return static_cast<float>(v)/32768;'),
 ('controller-dirty','Engine','return dt>0&&(rx!=0||ry!=0||lx!=0||ly!=0||rt!=lt);','return true;'),
 ('settings-debounce','Settings','if(!force&&GetTickCount()-last<400)return;',''),
 ('camera-child-search','Engine','for(unsigned i=0;i<count;++i){','for(unsigned i=0;i<1;++i){'),
 ('panel-close-save','UI','__try{FlushSettingsSave(true);}','__try{}'),
 ('panel-stale-input','UI','messageCount=0;ReleaseSRWLockExclusive(&messageLock);','ReleaseSRWLockExclusive(&messageLock);'),
]
cases += [('load-discard','Engine','if(reset)ApplyReset((reset&2)!=0,(reset&4)!=0);','if(reset)ApplyReset((reset&2)!=0,false);'),('load-discard-sticky','Engine','(before&4)|(mayTeardown?4:0)','(mayTeardown?4:0)')]
cases += [
 ('legacy-preview-default','Settings','if(settingsVersion<3)s.hidePreviews=true;','if(settingsVersion<3)s.hidePreviews=false;'),
 ('explicit-preview-preference','Settings','if(settingsVersion<3)s.hidePreviews=true;','if(true)s.hidePreviews=true;'),
 ('prompt-preview-visibility','Engine','Preview(s.hidePreviews);','Preview(s.hidePreviews&&!prompt);'),
 ('all-preview-models','Engine','for(unsigned i=0;i<nodeCount;++i){OwnedPreview* found','for(unsigned i=nodeCount?nodeCount-1:0;i<nodeCount;++i){OwnedPreview* found'),
 ('preview-release','Engine','own.restoreVisible=false;own.ref.Release();','own.restoreVisible=false;'),
 ('preexisting-preview-cull','Engine','own.restoreVisible=(nodes[i]->m_flags&1)==0;','own.restoreVisible=true;'),
]
cases += [
 ('background-parent-gate','Engine','if(!active){if(!s.remember){','if(Open(menus,4))return;if(!active){if(!s.remember){'),
 ('front-orbit-reset','Engine','opening.yaw=0;',''),
 ('respect-orbit-preference','Engine','opening.turnToFront&&opening.resetOrbitOnOpen','opening.turnToFront'),
 ('native-camera-reapply','Engine','if(I==1)KeepCameraAfterNativeUpdate(state,next);',''),
 ('pending-camera-transition','Engine','if(!Readable(next,sizeof(void*),false)||*static_cast<void**>(next))return;',''),
 ('paused-scene-propagation','Engine','NodeUpdate(body);++statistics.poseUpdates;',''),
 ('paused-scene-disabled','Engine','if(s.animatePaused&&paused&&!prompt&&!souls){\n  auto* loaded=','if(paused&&!prompt&&!souls){\n  auto* loaded='),
 ('model-heading','Engine','float heading=BodyHeading(player,body)','float heading=Field<NiPoint3>(player,0x28).z'),
 ('late-first-person-repair','Engine','if(State(camera)==0||State(camera)==7){Third(camera);if(State(camera)!=9&&State(camera)!=8){Stop(true);return;}}',''),
 ('camera-recursion-release','Engine','__finally{applyingCamera=false;}','__finally{}'),
 ('projection-transaction','Engine','float htan=std::tan(s.fov*Pi/360),vtan=htan/aspect;','float htan=std::tan(s.fov*Pi/360),vtan=htan/aspect;Field<float>(camera,0xAC)=s.fov;'),
]
cases += [
 ('physical-pan-profile','Engine','// Shift the projection rather than moving the eye sideways: the model stays frontal.','position=Add(position,Mul(right,-distance*htan*(2*s.screenX-1)));position=Add(position,Mul(up,-distance*vtan*(1-2*s.screenY)));'),
 ('off-axis-framing','Engine','const float centerX=htan*(1-2*s.screenX),centerY=vtan*(2*s.screenY-1);','const float centerX=0,centerY=0;'),
]
cases += [
 ('background-camera-reapply','Engine','if(!menus||!Open(menus,sessionMenu)||Blocked(menus))return;','if(!menus||!Open(menus,sessionMenu)||Open(menus,4)||Blocked(menus))return;'),
 ('late-tween-return','Engine','if(State(camera)==0||State(camera)==7){Third(camera);','if(State(camera)==0){Third(camera);'),
 ('tween-fov-baseline','Engine','savedFOV=restoredFOV;',''),
 ('tween-lens-baseline','Engine','projectionRef.Set(after);savedFrustum=after->m_frustum;','projectionRef.Set(after);'),
 ('parent-blur-hide','Engine','Blur(Open(menus,4)?Menu(menus,4):nullptr,4,s.clearBlur&&Open(menus,4));',''),
 ('parent-blur-before-message','Engine','if(borrowed)RestoreBlur(4);',''),
 ('parent-blur-after-message','Engine','Blur(menu,4,GetSettings().clearBlur);',';'),
 ('parent-destructor-hook','Engine','ok=ok&&PatchSlot(0x010E8140,0,reinterpret_cast<void*>(&DtorHook<4>),reinterpret_cast<void**>(&oldDtor[4]));',''),
 ('parent-camera-return','Engine','if(parent)ResumeParentTween(camera);','if(parent){}'),
]
cases += [('native-fov-range','Engine','restoredFOV>0&&restoredFOV<180','restoredFOV>=20&&restoredFOV<=150')]
current_owner='if(!owner||owner!=thread){++statistics.wrongThreads;if(statistics.wrongThreads==1)Log("Camera deferred: UI thread=%lu owner=%lu",thread,owner);return;}\n if(uiThread!=thread){uiThread=thread;Log("Accepted UI/camera owner thread=%lu",thread);}'
old_owner='if(!uiThread)uiThread=thread;\n if(!owner||owner!=thread){++statistics.wrongThreads;return;}'
cases.append(('early-ui-owner','Engine',current_owner,old_owner))
cases += [
 ('camera-stable-shortcut','Engine','if(cameraCommitted&&Field<float>','if(false&&cameraCommitted&&Field<float>'),
 ('camera-fov-invalidates','Engine','Field<float>(camera,0xAC)==s.fov&&',''),
 ('camera-world-invalidates','Engine','memcmp(&root->m_worldTransform,&transform,sizeof(NiTransform))==0&&',''),
 ('camera-local-invalidates','Engine','memcmp(&root->m_localTransform,&result,sizeof(NiTransform))==0&&',''),
 ('camera-tail-invalidates','Engine','memcmp(reinterpret_cast<char*>(projection)+sizeof(NiAVObject),committedCameraData,sizeof(committedCameraData))==0&&',''),
 ('camera-child-local-invalidates','Engine','memcmp(&projection->m_localTransform,&committedLocal,sizeof(NiTransform))==0&&',''),
 ('camera-child-world-invalidates','Engine','memcmp(&projection->m_worldTransform,&committedWorld,sizeof(NiTransform))==0','true'),
 ('camera-framing-invalidates','Engine','projection->m_frustum.m_fLeft==centerX-htan&&projection->m_frustum.m_fRight==centerX+htan&&\n  projection->m_frustum.m_fTop==centerY+vtan&&projection->m_frustum.m_fBottom==centerY-vtan&&',''),
 ('inactive-wheel-work','Engine','return active&&!Disabled()&&InterlockedCompareExchange(&ready,0,0)!=0;','return true;'),
 ('profile-frame-bounded','Engine','sessionFrames++&63','sessionFrames++&0'),
 ('profile-camera-bounded','Engine','cameraSamples++&63','cameraSamples++&0'),
 ('profile-disabled','Engine','&&LoggingEnabled()&&','&&'),
]
cases += [
 ('final-projection-hook','Engine','ok=ok&&PatchSlot(0x01117CFC,0x2D,reinterpret_cast<void*>(&ProjectionUpdateHook),reinterpret_cast<void**>(&oldProjectionUpdate));',''),
 ('final-projection-repair','Engine','KeepFinalProjection(projection);',''),
 ('final-projection-order','Engine','oldProjectionUpdate(projection,context);','oldProjectionUpdate(projection,context);'),
 ('final-projection-identity','Engine','projection!=projectionRef.object||',''),
 ('final-projection-thread','Engine','uiThread!=GetCurrentThreadId()||!InterlockedCompareExchange(&ready,0,0)||InterlockedCompareExchange(&resetRequest,0,0))return;\n auto* camera=Camera();auto* menus=Menus();','!InterlockedCompareExchange(&ready,0,0)||InterlockedCompareExchange(&resetRequest,0,0))return;\n auto* camera=Camera();auto* menus=Menus();'),
 ('final-projection-blocked','Engine','||Blocked(menus)||!Readable(projection,sizeof(NiCamera))','||!Readable(projection,sizeof(NiCamera))'),
 ('final-projection-state','Engine','||(State(camera)!=8&&State(camera)!=9)||!menus','||!menus'),
 ('final-projection-disabled','Engine','const Settings s=GetSettings();if(!s.enabled)return;','const Settings s=GetSettings();'),
 ('final-projection-ortho','Engine','f.m_bOrtho||!std::isfinite(width)','!std::isfinite(width)'),
 ('final-projection-x','Engine','width*(0.5f-s.screenX)','width*(0.5f-0.5f)'),
 ('final-projection-y','Engine','height*(s.screenY-0.5f)','height*(0.5f-0.5f)'),
]
# Model the original ordering defect explicitly: repair after matrix consumption.
cases=[c for c in cases if c[0]!='final-projection-order']
cases.append(('final-projection-order','Engine','KeepFinalProjection(projection);',';'))
ui=(root/'src/UI.cpp').read_text(encoding='utf8')
report=[]
for name,kind,old,new in cases:
 source={'Engine':engine,'Settings':settings,'UI':ui}[kind]
 if old==new or old not in source:raise SystemExit('Missing mutation anchor: '+name)
 folder=out/name;folder.mkdir(exist_ok=True);shadow=folder/(kind+'.cpp');mutated=source.replace(old,new)
 if name=='final-projection-order':mutated=mutated.replace('oldProjectionUpdate(projection,context);','oldProjectionUpdate(projection,context);KeepFinalProjection(projection);')
 shadow.write_text(mutated)
 cmd=['python',str(root/'build.py'),'--tests','--sdk',a.sdk,'--msbuild',a.msbuild,'--out',str(folder),'--'+kind.lower()+'-source',str(shadow)]
 built=subprocess.run(cmd,capture_output=True,text=True,timeout=90);(folder/'build.log').write_text(built.stdout+built.stderr)
 if built.returncode:raise SystemExit('Mutation failed to compile: '+name)
 ran=subprocess.run([str(folder/'SPIIHarness.exe')],capture_output=True,timeout=40)
 log=(ran.stdout+ran.stderr).decode('utf8',errors='replace');(folder/'run.log').write_text(log)
 rejected=ran.returncode==1 and 'ASSERTION FAILED' in log and 'Harness initialized' in log
 report.append({'case':name,'rejected_by_assertion':rejected,'exit':ran.returncode})
 print(name, 'DETECTED' if rejected else 'FAILED',flush=True)
 if not rejected:raise SystemExit('Mutation did not produce an ordinary assertion failure: '+name)
(out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
assert (root/'src/Engine.cpp').read_text(encoding='utf8')==engine and (root/'src/Settings.cpp').read_text(encoding='utf8')==settings
assert (root/'src/UI.cpp').read_text(encoding='utf8')==ui
print(len(report),'compiled defects detected; production sources unchanged')
