REM run the exe after building in the data directory

zig build build

pushd data
start "" "../zig-out/bin/win32-cengine-dev.exe"
popd

zig build dll --watch
