@echo off
REM check if caller supplied output name
if "%~1"=="" (
    set "name=hot.txt"
) else (
    REM user supplied something
    set "name=%~1"
    if /i not "%~x1"==".txt" set "name=%~1.txt"
)

REM compile file to analyze
@echo on
zig c++ -std=c++11 -O3 -ffast-math -fno-math-errno -DCENGINE_SLOW=1 -DCENGINE_INTERNAL=1 -DDEBUG=1 -S -masm=intel -march=znver4 src\hot.cpp -o zig-out\hot.s
@echo off
REM Analyze and output file
REM llvm-mca.exe exists in the llvm release tar.gz and can be extracted anywhere
REM https://github.com/llvm/llvm-project/releases/tag/llvmorg-21.1.0
REM clang+llvm-21.1.0-x86_64-pc-windows-msvc.tar.xz
REM https://llvm.org/docs/CommandGuide/llvm-mca.html
@echo on
llvm-mca -mcpu=znver4 zig-out\hot.s > "zig-out\%name%"

