@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\Common7\Tools\vsvars32.bat"
set PATH=%PATH%;C:\Soft\cmake-3.11.1-win64-x64\bin;c:\soft\ninja;C:\Soft\Git\bin
cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release
