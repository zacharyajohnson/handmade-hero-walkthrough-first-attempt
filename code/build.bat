@echo off

mkdir ..\..\build

pushd ..\..\build

REM We are in the build directory due to the pushd command so we need to go up a level to the correct directory containing our project
gcc ..\handmade\code\win32_handmade.cpp
popd