@echo off
mkdir msvc64
cmake -H. -Bmsvc64  -G "Visual Studio 14 2015 Win64" -DCMAKE_WINDOWS_EXPORT_ALL_SYMBOLS=TRUE
