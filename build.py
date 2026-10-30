from pathlib import Path
import argparse,subprocess,shutil,os,json,hashlib
root=Path(__file__).resolve().parent
p=argparse.ArgumentParser();p.add_argument('--sdk',type=Path);p.add_argument('--msbuild');p.add_argument('--tests',action='store_true');p.add_argument('--engine-source',type=Path);p.add_argument('--settings-source',type=Path);p.add_argument('--ui-source',type=Path);p.add_argument('--out',type=Path);a=p.parse_args()
sdk=(a.sdk or Path(os.environ.get('SKSE_LE_SDK',root/'sdk'))).resolve()
if not (sdk/'skse/skse/PluginAPI.h').is_file():raise SystemExit('Provide --sdk <SKSE LE1.7.3 source root> or SKSE_LE_SDK; nothing is downloaded.')
msbuild=a.msbuild or os.environ.get('MSBUILD_EXE') or shutil.which('MSBuild')
if not msbuild:raise SystemExit('Provide --msbuild <MSBuild.exe> or run in a VS2022 developer prompt.')
project=root/('Harness.vcxproj' if a.tests else 'ShowPlayerInInventoryLE.vcxproj')
args=[msbuild,str(project),'/p:Configuration=Release','/p:Platform=Win32','/p:SdkRoot='+str(sdk),'/nologo','/v:minimal']
if a.ui_source:args+=['/p:UISource='+str(a.ui_source.resolve())]
if a.settings_source:args+=['/p:SettingsSource='+str(a.settings_source.resolve())]
if a.engine_source:args+=['/p:EngineSource='+str(a.engine_source.resolve())]
out=a.out.resolve() if a.out else root/('build/tests' if a.tests else 'build')
if a.out:args+=['/p:OutDir='+str(out)+os.sep]
subprocess.run(args,check=True,cwd=root)
binary=out/('SPIIHarness.exe' if a.tests else 'ShowPlayerInInventoryLE.dll')
files=list((root/'src').glob('*'))+list((root/'ThirdParty/imgui').rglob('*.h'))+list((root/'ThirdParty/imgui').rglob('*.cpp'))+[project]
if a.tests:files+=list((root/'tests').glob('*.cpp'))
if a.engine_source:files=[f for f in files if f!=root/'src/Engine.cpp']+[a.engine_source.resolve()]
if a.settings_source:files=[f for f in files if f!=root/'src/Settings.cpp']+[a.settings_source.resolve()]
if a.ui_source:files=[f for f in files if f!=root/'src/UI.cpp']+[a.ui_source.resolve()]
manifest={'binary_sha256':hashlib.sha256(binary.read_bytes()).hexdigest(),'sources':{str(f.relative_to(root)) if f.is_relative_to(root) else f.name:hashlib.sha256(f.read_bytes()).hexdigest() for f in files if f.is_file()}}
(out/'build-inputs.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf8')
print('Build and source manifest:',binary)
