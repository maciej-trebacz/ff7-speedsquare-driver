@echo off

REM Replace the version number here
cmake --preset "x86-Release" -D_DLL_VERSION="1.1"

cmake --build --preset "x86-Release"
IF NOT EXIST .dist mkdir .dist
copy /y .build\bin\FFNx.dll .dist\AF3DN.P