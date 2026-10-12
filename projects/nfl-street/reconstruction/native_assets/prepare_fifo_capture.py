"""Prepare an isolated Windows FIFO capture runner from the existing private build.

Writes copied host source, response files and a build script on W:. Does not
edit or overwrite the original runner source, module or executable.
"""
import argparse
import hashlib
import json
from pathlib import Path

def win(path):return 'W:\\'+str(path.resolve().relative_to('/mnt/w')).replace('/','\\')
def block(text,marker):
    start=text.index(marker);end=text.find('\n\n',start)
    return text[start:end]
def fields(text):return dict(line.strip().split(' = ',1) for line in text.splitlines()[1:] if ' = ' in line)

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root',type=Path,default=Path('/mnt/w/Daywater/tools/ModernGekko-Template/lib/ModernGekko'))
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--inspector',action='store_true');a=p.parse_args()
    a.output.resolve().relative_to(Path('/mnt/w/Daywater').resolve());a.output.mkdir(parents=True,exist_ok=True)
    build=a.root/'build-win64';ninja=(build/'build.ninja').read_text()
    compiler=fields(block(ninja,'build CMakeFiles\\moderngekko-run.dir\\tools\\moderngekko_run.cpp.obj:'))
    link_block=block(ninja,'build moderngekko-run.exe:');link=fields(link_block)
    objects=link_block.splitlines()[0].split('_Release ',1)[1].split(' | ',1)[0]
    source=(a.root/'tools/moderngekko_run.cpp').read_text()
    includes='\n'.join('#include "'+name+'"' for name in ['Core/Core.h','Core/System.h','Core/State.h','Core/FifoPlayer/FifoRecorder.h','VideoCommon/FrameDumper.h'])
    snippet=Path(__file__).with_name('fifo_capture_runner.inc').read_text()
    source=source.replace('namespace {',includes+'\n\n'+snippet+'\nnamespace {',1)
    marker='  const moderngekko::RuntimeRunResult result = created.runtime->Run();'
    assert source.count(marker)==1
    source=source.replace(marker,'  auto fifo_capture = StartResearchFifoCapture(*created.runtime);\n'+marker+'\n  fifo_capture.request_stop();')
    if a.inspector:source=Path(__file__).with_name('fifo_inspect.cpp').read_text()
    cpp=a.output/'fifo_capture_runner.cpp';cpp.write_text(source)
    obj=a.output/'fifo_capture_runner.obj';exe=a.output/('nflstreet-fifo-inspect.exe' if a.inspector else 'nflstreet-fifo-capture.exe')
    compile_rsp=a.output/'compile.rsp';compile_rsp.write_text(compiler['DEFINES']+' '+compiler['FLAGS']+' '+compiler['INCLUDES']+' /nologo /c /Fo"'+win(obj)+'" "'+win(cpp)+'"\n')
    objects=objects.replace('CMakeFiles\\moderngekko-run.dir\\tools\\moderngekko_run.cpp.obj','"'+win(obj)+'"')
    rsp=a.output/'link.rsp';rsp.write_text(objects+' '+link['LINK_LIBRARIES']+' '+link['LINK_FLAGS']+' /nologo /MANIFEST:EMBED /out:"'+win(exe)+'" /implib:"'+win(a.output/'capture.lib')+'" /pdb:"'+win(a.output/'capture.pdb')+'"\n')
    script='@echo off\r\ncall "C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\BuildTools\\Common7\\Tools\\VsDevCmd.bat" -arch=amd64 -host_arch=amd64\r\nif errorlevel 1 exit /b 1\r\ncd /d "'+win(build)+'"\r\ncl.exe @"'+win(compile_rsp)+'"\r\nif errorlevel 1 exit /b 1\r\nlink.exe @"'+win(rsp)+'"\r\nexit /b %ERRORLEVEL%\r\n'
    (a.output/'build.cmd').write_bytes(script.encode())
    (a.output/'build-inputs.json').write_text(json.dumps(dict(source_sha256=hashlib.sha256((a.root/'tools/moderngekko_run.cpp').read_bytes()).hexdigest(),build_ninja_sha256=hashlib.sha256(ninja.encode()).hexdigest(),baseline_modified=False),indent=2)+'\n')
    print(win(a.output/'build.cmd'))

if __name__=='__main__':main()
