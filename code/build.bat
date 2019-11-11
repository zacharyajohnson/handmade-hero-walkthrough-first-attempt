@echo off

mkdir ..\build

pushd ..\build

REM We are in the build folder right now so we need to go up a level to get to 
REM the code folder
gcc ..\code\win32_handmade.cpp
popd
