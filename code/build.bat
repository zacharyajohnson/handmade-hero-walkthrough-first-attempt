@echo off

mkdir ..\build

pushd ..\build

REM We are in the build folder right now so we need to go up a level to get to 
REM the code folder
REM the -l keyword is used to link external libraries to code. 
REM Gdi32 had to be linked for the PatBlt method from Day 2
gcc ..\code\win32_handmade.cpp -lGdi32

REM Optinally run the exe after it is compiled successfully
REM a.exe

popd
