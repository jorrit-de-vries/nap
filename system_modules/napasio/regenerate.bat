@echo off
set PYTHONPATH=
set PYTHONHOME=
set python=%~dp0\..\..\thirdparty\python\python.exe
if not exist %python% (
    set python=%~dp0\..\..\..\thirdparty\python\msvc\x86_64\python
)
%python% %~dp0\..\..\tools\buildsystem\common\regenerate_module_by_dir.py %~dp0 %*
