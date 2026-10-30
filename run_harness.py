from pathlib import Path
import subprocess,sys,json,time
root=Path(__file__).resolve().parent
exe=Path(sys.argv[1]).resolve() if len(sys.argv)>1 and not sys.argv[1].startswith('--') else root/'build/tests/SPIIHarness.exe'
sources=list((root/'src').glob('*.cpp'))+list((root/'src').glob('*.h'))+list((root/'tests').glob('*.cpp'))+[root/'Harness.vcxproj']
if exe.name=='SPIIHarness.exe' and exe.parent==root/'build/tests' and exe.stat().st_mtime_ns<max(p.stat().st_mtime_ns for p in sources):
 raise SystemExit('REFUSED stale harness: rebuild after source/project changes')
start=time.perf_counter()
if '--prove-timeout' in sys.argv:
 result=subprocess.run([str(exe),'--stall'],cwd=root,timeout=5,capture_output=True,creationflags=0x08000000)
 if result.returncode==124:print('PASS: deliberately stalled owned child terminated by native supervisor');raise SystemExit(0)
 raise SystemExit('FAIL: native stalled-child supervisor')
try:result=subprocess.run([str(exe)],cwd=root,timeout=45,capture_output=True,creationflags=0x08000000)
except subprocess.TimeoutExpired:print('TIMEOUT: owned harness child terminated');raise SystemExit(124)
text=result.stdout.decode('utf8',errors='replace')+result.stderr.decode('utf8',errors='replace');print(text,end='')
if result.returncode:print('Child exit:',hex(result.returncode&0xffffffff))
if not (result.returncode==0 and '0 failures' in text and 'PASS late callbacks' in text):raise SystemExit(1)
print('PASS supervised actual-source harness in',round(time.perf_counter()-start,3),'seconds')
