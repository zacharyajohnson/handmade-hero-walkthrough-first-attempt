#!/bin/bash

# Bash halts execution of the file if you try to create a directory and it already exists so we need this check.
if [-d "../build" ]
then
    echo "Directory already exists. Continuing execution"
else
    mkdir ../build
fi

cd ../build

# We are in the build folder right now so we need to go up a level to get to 
# the code folder
# the -l keyword is used to link external libraries to code. 
# Gdi32 had to be linked for the PatBlt method from Day 2
gcc ../code/win32_handmade.cpp -lGdi32

# Optinally runt the exe after it is compiled successfully
# ./a.exe

cd ../code