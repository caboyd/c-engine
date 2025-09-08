REM run the exe after building in the data directory

zig build build -Doptimize=ReleaseFast

pushd data && "../zig-out/bin/win32-cengine.exe" && popd

